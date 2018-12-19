CFLAGS:=-Wall -pedantic -std=gnu11 -g $(shell pkg-config fuse --cflags)
LDFLAGS:=$(shell pkg-config fuse --libs) $(shell pkg-config libcrypto --libs)

all: realmain

realmain: realmain.c ssl-crypt.o tiny-AES-c/aes.o

ssl-crypt.o:

clean:
	$(RM) realmain *.o