CFLAGS:=-Wall -pedantic -std=gnu11 -g $(shell pkg-config fuse --cflags)
LDFLAGS:=$(shell pkg-config fuse --libs) $(shell pkg-config libcrypto --libs)

all: realmain

realmain: realmain.c path_util.o ssl-crypt.o
	gcc $^ -o $@ $(CFLAGS) $(LDFLAGS)

path_util.o: path_util.c ssl-crypt.c ssl-crypt.h
	gcc $< -c -o $@ $(CFLAGS) $(LDFLAGS)

ssl-crypt.o: ssl-crypt.c ssl-crypt.h
	gcc $< -c -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) realmain *.o