CC=gcc
CFLAGS=-g
all : oss user

oss: oss.c shared.h
	$(CC) -o $@ $^

user: user.c shared.h
	$(CC) -o $@ $^

clean:
	rm logfile user oss