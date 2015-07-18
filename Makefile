CFLAGS=-Wall -O2 -g -ffreestanding
V6CFLAGS=
V7CFLAGS=-march=armv7-a

BCM2835_OBJS=\
     uart-bcm2835.o \
     system-bcm2835.o

BCM2836_OBJS=\
     uart-bcm2836.o \
     system-bcm2836.o

LIBS=libuart-bcm2835.a \
     libuart-bcm2836.a

all: $(LIBS)

libuart-bcm2835.a: $(BCM2835_OBJS)
	ar cr $@ $^

libuart-bcm2836.a: $(BCM2836_OBJS)
	ar cr $@ $^

clean:
	rm -f $(LIBS) $(BCM2835_OBJS) $(BCM2836_OBJS)

%-bcm2835.o: %.c
	$(CC) $(CFLAGS) $(V6CFLAGS) -DPLAT_PI_1_B_PLUS -c -o $@ $<

%-bcm2836.o: %.c
	$(CC) $(CFLAGS) $(V7CFLAGS) -DPLAT_PI_2_B -c -o $@ $<
