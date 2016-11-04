
# Uncomment following line to use fastest CFLAGS for gcc.
#   program will only run on hardware that is equal or better to
#   the current host.
#CFLAGS ?= -O3 -march=native -mtune=native -flto -Bdirect -Wall

# Default lame CFLAGS.
CFLAGS ?= -O3 -Wall

# Cause everything to recompile when CFLAGS changes, unless user is root (to support "sudo make install")
WHOAMI=$(shell whoami)
CFLAGS_HASH=$(shell echo "${CFLAGS}" | md5sum | tr ' ' '\n' | head -n1)
CFLAGS_HASH_FILE=$(shell test "${WHOAMI}" != "root" && echo .cflags.${CFLAGS_HASH} || echo .cflags.*)

PREFIX ?= $(shell test -w "/usr/bin" && echo "/usr" || echo "${HOME}")

DESTDIR ?= ${PREFIX}

DEPS = bin ${CFLAGS_HASH_FILE}

ALL_FILES = bin/getppid \
	bin/getcpids


all: ${ALL_FILES}
	true

clean:
	rm -Rf bin
	rm -f *.o
	rm -f .cflags.*

install: ${ALL_FILES}
	mkdir -p "${DESTDIR}/bin"
	install -m 775 ${ALL_FILES} "${DESTDIR}/bin"


# When hash of CFLAGS changes, this unit causes all compiles to become invalidated
${CFLAGS_HASH_FILE}:
	test "${WHOAMI}" != "root" -a ! -e "${CFLAGS_HASH_FILE}" && rm -f .cflags.* || true
	touch "${CFLAGS_HASH_FILE}"


bin:
	mkdir -p bin

getppid.o : getppid.c
	gcc ${CFLAGS} getppid.c -c -o getppid.o

bin/getppid : ${DEPS}  getppid.o
	gcc ${CFLAGS} getppid.o -o bin/getppid

getcpids.o : getcpids.c
	gcc ${CFLAGS} getcpids.c -c -o getcpids.o

bin/getcpids : ${DEPS} getcpids.o
	gcc ${CFLAGS} getcpids.o -o bin/getcpids


