# list out the driver program source
# TEST_SRC = $(wildcard tests/*.c)
# PROGS := tests/1-i2s-test.c
# PROGS := tests/2-i2s-dump.c
# PROGS := tests/3-i2s-audiovis.c
# PROGS := tests/4-fft-test.c
# PROGS := tests/5-fft-time.c
PROGS := tests/6-fft-freqout.c
# PROGS := tests/7-fft-freqviz.c
# PROGS := tests/8-fft-buckets.c
# PROGS := tests/9-bitbang-write.c
# PROGS := tests/9-jingle.c
# PROGS := play-mp3.c
# PROGS := wav_to_file.c

# OPT_LEVEL = -O3

STAFF_OBJS += external/staff-mbr.o

#CFLAGS_EXTRA  = -Iexternal-code

# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
SRC = i2s.c fft.c external/pi-sd.c external/emmc.c external/mbox.c external/fat32.c external/mbr-helpers.c external/fat32-helpers.c external/fat32-lfn-helpers.c external/unicode-utf8.c

# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = my-install

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

DEPS = ./Makefile
COMMON_OBJS := $(SRC:.c=.o)
include $(CS140E_2024_PATH)/libpi/mk/Makefile.template-v3