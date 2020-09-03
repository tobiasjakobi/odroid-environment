compiler := gcc
cflags   := -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard
ldflags  :=
destdir  := /usr/local

ifndef platform
platform := $(shell $(compiler) -dumpmachine)
endif

ifeq (release-lto,$(build))
cflags += -O2 -flto=4 -fuse-linker-plugin -DNDEBUG
ldflags += -O2 -flto=4 -fuse-linker-plugin
endif

ifeq (release,$(build))
cflags += -O2 -DNDEBUG
endif

ifeq (debug,$(build))
cflags += -O0 -g
endif

ifneq (,$(DESTDIR))
destdir := $(DESTDIR)
endif

objects := nv12_test libnv12mt.so

all: $(objects)

%.o: %.S
	$(compiler) -c -o $@ $(cflags) $<

%.o: %.c
	$(compiler) -std=c99 -c -o $@ $(cflags) $<

nv12_lib.o: nv12_test.c
	$(compiler) -std=c99 -fPIC -DNV12_LIB -c -o $@ $(cflags) $<

nv12_test: nv12_test.o nv12mt_to_yuv420m.neon.o
	$(compiler) -o $@ $^ $(ldflags)

libnv12mt.so: nv12_lib.o
	$(compiler) -shared -o $@ $^ $(ldflags)

clean:
	rm -f *.o
	rm -f $(objects)

strip:
	strip -s $(objects)
