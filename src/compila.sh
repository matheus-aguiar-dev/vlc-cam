gcc $(pkg-config --cflags gtk+-3.0 libvlc) -c start.c -o start.o -lX11
gcc start.o -o start $(pkg-config --libs gtk+-3.0 libvlc) -lX11
