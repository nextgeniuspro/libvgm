// String Utilities: Character Codepage Conversion
// ----------------

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#ifdef _MSC_VER
#define strdup		_strdup
#define stricmp		_stricmp
#define strnicmp	_strnicmp
#else
#ifdef _POSIX_C_SOURCE
#include <strings.h>
#endif
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#include <errno.h>

#include "../stdtype.h"
#include "StrUtils.h"



static size_t GetEncodingCharSize(const char* encoding)
{
	return sizeof(char);
}

static size_t GetStrSize(const char* str, size_t charBytes)
{
	return strlen(str) * charBytes;
}

UINT8 CPConv_Init(CPCONV** retCPC, const char* cpFrom, const char* cpTo)
{
	return 0x00;
}

void CPConv_Deinit(CPCONV* cpc)
{	
	return;
}

UINT8 CPConv_StrConvert(CPCONV* cpc, size_t* outSize, char** outStr, size_t inSize, const char* inStr)
{
	// Copy the input string to the output string
	if (*outStr == NULL)
	{
		*outStr = (char*)malloc(inSize + 1);
	}
	memcpy(*outStr, inStr, inSize);
	(*outStr)[inSize] = '\0';
	*outSize = inSize;

	return 0x00;
}
