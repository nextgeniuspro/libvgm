// Audio Stream - KallistiOS [Dreamcast] driver
#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// for memcpy() etc.

#include "../common_def.h"	// stdtype.h, INLINE

#include "AudioStream.h"

#include <kos.h>
#include <kos/thread.h>
#include <dc/sound/stream.h>

#ifdef _MSC_VER
#define	strdup	_strdup
#endif

#pragma pack(1)
typedef struct
{
	UINT16 wFormatTag;
	UINT16 nChannels;
	UINT32 nSamplesPerSec;
	UINT32 nAvgBytesPerSec;
	UINT16 nBlockAlign;
	UINT16 wBitsPerSample;
} WAVEFORMAT;	// from MSDN Help
#pragma pack()

uint8_t __attribute__((aligned(32))) pcm_buffer[SND_STREAM_BUFFER_MAX+16384]; /* complete buffer + 16KB safety */

typedef struct _kos_wave_writer_driver
{
	void* audDrvPtr;
	UINT8 devState;	// 0 - not running, 1 - running, 2 - terminating
	UINT8 playing;	// 0 - not; 1 - yes
	
	WAVEFORMAT waveFmt; // TODO: Unused
	AUDFUNC_FILLBUF FillBuffer;
	void* userParam;
} DRV_KOS;
static DRV_KOS* g_drv;


UINT8 KosDrv_IsAvailable(void);
UINT8 KosDrv_Init(void);
UINT8 KosDrv_Deinit(void);
const AUDIO_DEV_LIST* KosDrv_GetDeviceList(void);
AUDIO_OPTS* KosDrv_GetDefaultOpts(void);

UINT8 KosDrv_Create(void** retDrvObj);
UINT8 KosDrv_Destroy(void* drvObj);

UINT8 KosDrv_Start(void* drvObj, UINT32 deviceID, AUDIO_OPTS* options, void* audDrvParam);
UINT8 KosDrv_Stop(void* drvObj);
UINT8 KosDrv_PauseResume(void* drvObj);

UINT8 KosDrv_SetCallback(void* drvObj, AUDFUNC_FILLBUF FillBufCallback, void* userParam);
UINT32 KosDrv_GetBufferSize(void* drvObj);
UINT8 KosDrv_IsBusy(void* drvObj);
UINT8 KosDrv_WriteData(void* drvObj, UINT32 dataSize, void* data);
UINT32 KosDrv_GetLatency(void* drvObj);


AUDIO_DRV audDrv_Kos =
{
	{ADRVTYPE_OUT, ADRVSIG_LIBKOS, "KosDrv"},
	
	KosDrv_IsAvailable,
	KosDrv_Init, KosDrv_Deinit,
	KosDrv_GetDeviceList, KosDrv_GetDefaultOpts,
	
	KosDrv_Create, KosDrv_Destroy,
	KosDrv_Start, KosDrv_Stop,
	KosDrv_PauseResume, KosDrv_PauseResume,
	
	KosDrv_SetCallback, KosDrv_GetBufferSize,
	KosDrv_IsBusy, KosDrv_WriteData,
	
	KosDrv_GetLatency,
};

static char* kosOutDevNames[1] = {"KallistiOS Driver"};
static AUDIO_OPTS defOptions;
static AUDIO_DEV_LIST deviceList;

static UINT8 isInit = 0;
static UINT32 activeDrivers;
kthread_t * snddrv_thd;

/* Signal how many samples the AICA needs, then wait for the deocder to produce them */
static void *KosDrv_Callback(snd_stream_hnd_t hnd, int len, int *actual) 
{   
	if (g_drv->FillBuffer == NULL) {
		*actual = 0;
		return NULL;
	}

	/* Check if the callback requests more data than our buffer can hold */
	if (len > SND_STREAM_BUFFER_MAX) {
		len = SND_STREAM_BUFFER_MAX;
	}
	    
	g_drv->FillBuffer(g_drv->audDrvPtr, g_drv->userParam, len, pcm_buffer);

	*actual = len;
	return pcm_buffer;
}

static void* KosDrv_Thread(void* drvObj) 
{	   
	snd_stream_hnd_t shnd = snd_stream_alloc(KosDrv_Callback, SND_STREAM_BUFFER_MAX);
	snd_stream_start(shnd, defOptions.sampleRate, defOptions.numChannels - 1);
	
	DRV_KOS* drv = (DRV_KOS*)drvObj;
	drv->devState = 1;
	while( drv->devState == 1 ) {
		snd_stream_poll(shnd);		
		timer_spin_sleep(10);
	}

    snd_stream_destroy(shnd);
	snd_stream_shutdown();

	return NULL;
}


UINT8 KosDrv_IsAvailable(void)
{
	return 1;
}

UINT8 KosDrv_Init(void)
{
	if (isInit)
		return AERR_WASDONE;
	
	deviceList.devCount = 1;
	deviceList.devNames = kosOutDevNames;
	
	memset(&defOptions, 0x00, sizeof(AUDIO_OPTS));
	defOptions.sampleRate = 44100;
	defOptions.numChannels = 2;
	defOptions.numBitsPerSmpl = 16;
	defOptions.usecPerBuf = 10000;
	defOptions.numBuffers = 1;
	
	snd_stream_init();
	
	activeDrivers = 0;
	isInit = 1;
	
	return AERR_OK;
}

UINT8 KosDrv_Deinit(void)
{
	if (! isInit)
		return AERR_WASDONE;
	
	deviceList.devCount = 0;
	deviceList.devNames = NULL;
	
	isInit = 0;
	
	return AERR_OK;
}

const AUDIO_DEV_LIST* KosDrv_GetDeviceList(void)
{
	return &deviceList;
}

AUDIO_OPTS* KosDrv_GetDefaultOpts(void)
{
	return &defOptions;
}


UINT8 KosDrv_Create(void** retDrvObj)
{	
	g_drv = (DRV_KOS*)malloc(sizeof(DRV_KOS));
	g_drv->devState = 0;

	if((uintptr_t)pcm_buffer & 31) {
		printf("Buffer not aligned to 32 bytes\n");
	}

	snddrv_thd = thd_create( true, KosDrv_Thread, g_drv );
	
	activeDrivers ++;
	*retDrvObj = g_drv;
	
	return AERR_OK;
}

UINT8 KosDrv_Destroy(void* drvObj)
{
	DRV_KOS* drv = (DRV_KOS*)drvObj;
	
	if (drv->devState != 0)
		KosDrv_Stop(drvObj);
	
	activeDrivers --;
		
	memset( pcm_buffer, 0, SND_STREAM_BUFFER_MAX);
		
	free(drv);
	
	return AERR_OK;
}

UINT8 KosDrv_Start(void* drvObj, UINT32 deviceID, AUDIO_OPTS* options, void* audDrvParam)
{
	DRV_KOS* drv = (DRV_KOS*)drvObj;
	drv->playing = 1;

	return AERR_OK;
}

UINT8 KosDrv_Stop(void* drvObj)
{
	DRV_KOS* drv = (DRV_KOS*)drvObj;
	drv->playing = 0;
	
	if (drv->devState != 1)
		return AERR_NOT_OPEN;
	
	drv->devState = 2;
	
	return AERR_OK;
}

UINT8 KosDrv_PauseResume(void* drvObj)
{
	DRV_KOS* drv = (DRV_KOS*)drvObj;
	drv->playing = !drv->playing;

	return AERR_OK;
}


UINT8 KosDrv_SetCallback(void* drvObj, AUDFUNC_FILLBUF FillBufCallback, void* userParam)
{
	DRV_KOS* drv = (DRV_KOS*)drvObj;
	drv->FillBuffer = FillBufCallback;
	drv->userParam = userParam;

	return AERR_OK;
}

UINT32 KosDrv_GetBufferSize(void* drvObj)
{
	return SND_STREAM_BUFFER_MAX;
}

UINT8 KosDrv_IsBusy(void* drvObj)
{
	DRV_KOS* drv = (DRV_KOS*)drvObj;
	
	if (drv->FillBuffer != NULL)
		return AERR_BAD_MODE;
	
	return AERR_OK;
}

UINT8 KosDrv_WriteData(void* drvObj, UINT32 dataSize, void* data)
{
	DRV_KOS* drv = (DRV_KOS*)drvObj;
	UINT32 wrtBytes;

	// TODO: Implement this
	
	return AERR_OK;
}

UINT32 KosDrv_GetLatency(void* drvObj)
{
	return 0;
}
