CFLAGS:=-Wall -pedantic -std=gnu11 -g $(shell pkg-config fuse --cflags)
LDFLAGS:=$(shell pkg-config fuse --libs) $(shell pkg-config libcrypto --libs)

all: realmain

realmain: realmain.c path_util.o ssl-crypt.o
	gcc $^ -o $@ $(CFLAGS) $(LDFLAGS)

path_util.o: path_util.c ssl-crypt.c ssl-crypt.h
	gcc $< -c -o $@ $(CFLAGS) $(LDFLAGS)

ssl-crypt.o: ssl-crypt.c ssl-crypt.h
	gcc $< -c -o $@ $(CFLAGS) $(LDFLAGS)

path_util_test: path_util_test.c path_util.o ssl-crypt.o
	gcc $^ -o $@ $(CFLAGS) $(LDFLAGS)

ssl-test-main1: ssl-test-main.c ssl-crypt.o
	gcc $^ -DTEST1 -o $@ $(CFLAGS) $(LDFLAGS)
ssl-test-main2: ssl-test-main.c ssl-crypt.o
	gcc $^ -DTEST2 -o $@ $(CFLAGS) $(LDFLAGS)
ssl-test-main12: ssl-test-main.c ssl-crypt.o
	gcc $^ -DTEST1 -DTEST2 -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) realmain path_util_test ssl-test-main1 ssl-test-main2 ssl-test-main12 *.o