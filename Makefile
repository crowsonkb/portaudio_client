# Config options peculiar to my system (OS X, portaudio in /usr/local)
CC = clang
CFLAGS = -Wall -std=gnu99
CPATH = -I/usr/local/include
LDFLAGS = -Wl,-L/usr/local/lib,-lportaudio

read: read.c
	$(CC) $(CFLAGS) $(CPATH) $(LDFLAGS) read.c -o read

clean:
	rm read
