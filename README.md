# libvgm

This is forked version of libvgm with fixes for Dreamcast (KallistiOS) platform. Unfortunately, performance on Dreamcast is poor and only simple VGM files can be played.

## How to build for Dreamcast

You need to have KallistiOS and dc-toolchain installed on your system. You can find instructions on how to install it [here](https://github.com/KallistiOS/KallistiOS)

1. Clone the repository
```bash
git clone https://github.com/nextgeniuspro/libvgm.git
```

2. Import KallistiOS environment variables
```bash
source <path-to-kos>/environ.sh
```

3. Generate build files
```bash
cmake -S . -B builddc -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=${KOS_CMAKE_TOOLCHAIN} -DOS_KOS:STRING=1
```

4. Build the library
```bash
cmake --build builddc -j8
```

5. Launch in the emulator
```bash
Flycast builddc/bin/player.elf
```

To update vgm file you can put it to romdisk folder as `sample.vgm` and rebuild the project.

## How to build for macOS

1. Clone the repository
```bash
git clone https://github.com/nextgeniuspro/libvgm.git
```

2. Generate build files
```bash
cmake -S . -B buildmac -G "Xcode"
```

3. Open the project in Xcode and build it

4. Run the player
```bash
./buildmac/Debug/player <path-to-vgm-file>
```