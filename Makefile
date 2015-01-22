CC ?= cc
CFLAGS ?= -O2
LDFLAGS ?=
DESTDIR ?=
SBIN_DIR ?= /sbin
DOC_DIR ?= /usr/share/doc/nopdev
MAN_DIR ?= /usr/share/man

CFLAGS += -Wall -pedantic

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)

all: nopdev start-nopdev

config.h: config.def.h
	$(warning Using the default configuration)
	cp config.def.h config.h

%.o: %.c config.h
	$(CC) -c -o $@ $< $(CFLAGS)

nopdev: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

start-nopdev: start-nopdev.in
	sed s~@SBIN_DIR@~$(SBIN_DIR)~ $^ > $@

clean:
	rm -f start-nopdev nopdev $(OBJECTS) config.h

install: all
	install -D -m 755 nopdev $(DESTDIR)$(SBIN_DIR)/nopdev
	install -m 755 start-nopdev $(DESTDIR)$(SBIN_DIR)/start-nopdev
	install -D -m 644 nopdev.8 $(DESTDIR)$(MAN_DIR)/man8/nopdev.8
	install -m 644 start-nopdev.8 $(DESTDIR)$(MAN_DIR)/man8/start-nopdev.8
	install -D -m 644 README $(DESTDIR)$(DOC_DIR)/README
	install -m 644 AUTHORS $(DESTDIR)$(DOC_DIR)/AUTHORS
	install -m 644 COPYING $(DESTDIR)$(DOC_DIR)/COPYING
