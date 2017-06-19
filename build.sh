#!/bin/bash
gcc -std=c99 -g -o dsitest dsitest.c libdsi.c libdsifw.c -I. `pkg-config --libs --cflags libusb-1.0` -lm
