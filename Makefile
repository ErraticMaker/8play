VERSION = 1

PREFIX ?= /usr/local
MANPREFIX ?= ${PREFIX}/share/man

CC ?= cc
CFLAGS += -Wall -Wextra
CFLAGS += -std=c99 -pedantic -O2
CFLAGS += -D_XOPEN_SOURCE=700
CFLAGS += `pkg-config --cflags json-c \
		libavcodec \
		libavformat \
		libavresample \
		libavutil \
		libcurl \
		sdl`
LDFLAGS += -s -lpthread
LDFLAGS += -Ilibplayer
LDFLAGS += `pkg-config --libs json-c \
		libavcodec \
		libavformat \
		libavresample \
		libavutil \
		libcurl \
		sdl`

SRC = 8tracks.c curl.c main.c
OBJ = ${SRC:.c=.o}

all: 8play

.c.o:
	${CC} -c ${CFLAGS} $<

8play: libplayer/player.o ${OBJ}
	${CC} -o $@ ${OBJ} libplayer/player.o ${LDFLAGS}

libplayer/player.o:
	cd libplayer; ${CC} -c ${CFLAGS} player.c

.PHONY: clean install uninstall

clean:
	rm -f 8play ${OBJ} 8play.1.gz libplayer/player.o

dist:
	@echo creating tarball
	mkdir -p 8play-${VERSION}/libplayer
	cp *.1 *.c *.h Makefile README.md 8play-${VERSION}
	cp libplayer/player.c libplayer/player.h libplayer/README.md \
	    8play-${VERSION}/libplayer
	tar -cf 8play-${VERSION}.tar 8play-${VERSION}
	gzip 8play-${VERSION}.tar
	rm -rf 8play-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f 8play ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/8play
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	gzip -9 < 8play.1 > 8play.1.gz
	chmod 644 8play.1.gz
	cp -f 8play.1.gz ${DESTDIR}${MANPREFIX}/man1

uninstall:
	rm ${DESTDIR}${PREFIX}/bin/8play
	rm ${DESTDIR}${MANPREFIX}/man1/8play.1.gz

