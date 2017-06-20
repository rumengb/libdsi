all:
	gcc -g -o dsitest dsitest.c libdsi.c -I. `pkg-config --libs --cflags libusb-1.0` -lm
