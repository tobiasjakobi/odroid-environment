compiler := gcc
cflags   := -march=armv7-a -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard
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

objects := neon_test thread_test test_memcmp

all: $(objects)

%.o: %.S
	$(compiler) -c -o $@ $(cflags) $<

%.o: %.c
	$(compiler) -c -o $@ $(cflags) $<

thread_test: thread_test.o; $(compiler) -o $@ $^ $(ldflags) -lpthread

neon_test: neon_test.o put_glyph_rgba4444_neon.o; $(compiler) -o $@ $^ $(ldflags)

test_memcmp: test_memcmp.o memcmp-neon.o; $(compiler) -o $@ $^ $(ldflags)

clean:
	rm -f *.o
	rm -f $(objects)

strip:
	strip -s $(objects)

