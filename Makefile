# Config options peculiar to my system (OS X, portaudio in /usr/local)
CC = /usr/bin/clang
CFLAGS = -Wall -std=gnu11 -O3
CPATH = -I/usr/local/include
LDFLAGS = -Wl,-L/usr/local/lib,-lportaudio
CCFULL = $(CC) $(CFLAGS) $(CPATH) $(LDFLAGS)

read_samples: read_samples.c
	$(CCFULL) read_samples.c -o read_samples

clean:
	rm read_samples
