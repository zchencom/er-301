# Suns for disting NT

Display-only algorithm: three suns and their merged light on the floor.

Build (needs `gcc-arm-none-eabi` and the distingNT_API headers):

    arm-none-eabi-c++ -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
      -mthumb -fno-rtti -fno-exceptions -Os -fPIC -Wall \
      -I distingNT_API/include -c -o suns_nt.o suns_nt.cpp

Install: copy `suns_nt.o` to the `programs/plug-ins` folder on the NT's SD card,
then add the algorithm **Suns** (category Utility) to a preset.
