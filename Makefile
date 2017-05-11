
# Uncomment following line to use fastest CFLAGS for gcc.
#   program will only run on hardware that is equal or better to
#   the current host.
#CFLAGS ?= -O3 -march=native -mtune=native -flto -fuse-linker-plugin

# Default CFLAGS
CFLAGS ?= -O3 -flto -fuse-linker-plugin -s

LDFLAGS ?= -flto -fuse-linker-plugin -Wl,-O1,--sort-common,--as-needed,-z,relro

DEBUG_CFLAGS = -Og -ggdb3
DEBUG_LDFLAGS = -Wl,-Og -ggdb3

C_STANDARD=$(shell test -f .use_c_std && cat .use_c_std || (echo 'int main(int argc, char *argv[]) { return 0; }' > .uc.c; ${CC} -std=gnu99 .uc.c >/dev/null 2>&1 && (echo 'gnu99' > .use_c_std; echo 'gnu99'; rm -f .uc.c) || ( echo 'c99' > .use_c_std; echo 'c99'; rm -f .uc.c ) ))

# Actual flags to use.
USE_CFLAGS = ${CFLAGS} -Wall -Wno-unused-function -pipe -std=${C_STANDARD}

USE_LDFLAGS = ${LDFLAGS}

# Cause everything to recompile when CFLAGS changes, unless user is root (to support "sudo make install")
WHOAMI=$(shell whoami)
CFLAGS_HASH=$(shell echo "CFLAGS=${USE_CFLAGS} .. LDFLAGS=${USE_LDFLAGS}" | md5sum | tr ' ' '\n' | head -n1)
CFLAGS_HASH_FILE=$(shell test "${WHOAMI}" != "root" && echo .cflags.${CFLAGS_HASH} || echo .cflags.*)

PREFIX ?= $(shell test -w "/usr/bin" && echo "/usr" || echo "${HOME}")

DESTDIR ?= ${PREFIX}

DEPS = bin/.created ${CFLAGS_HASH_FILE} pid_tools.h pid_utils.h

ALL_FILES = bin/getppid \
	bin/getcpids \
	bin/isaparentof \
	bin/isachildof \
	bin/getpcmd \
	bin/waitpid


all: ${DEPS} ${ALL_FILES}
#	@ /bin/true

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


static:
	CFLAGS="${CFLAGS} -static" make

debug:
	CFLAGS="${DEBUG_CFLAGS}" LDFLAGS="${DEBUG_LDFLAGS}" make


bin/.created:
	mkdir -p bin
	touch bin/.created

getppid.o : ${DEPS} getppid.c ppid.c
	gcc ${USE_CFLAGS} getppid.c -c -o getppid.o

getcpids.o : ${DEPS} getcpids.c ppid.c
	gcc ${USE_CFLAGS} getcpids.c -c -o getcpids.o

isaparentof.o : ${DEPS} isaparentof.c ppid.c
	gcc ${USE_CFLAGS} isaparentof.c -c -o isaparentof.o

isachildof.o : ${DEPS} isachildof.c ppid.c
	gcc ${USE_CFLAGS} isachildof.c -c -o isachildof.o

getpcmd.o : ${DEPS} getpcmd.c
	gcc ${USE_CFLAGS} getpcmd.c -c -o getpcmd.o

bin/isaparentof : ${DEPS} isaparentof.o
	gcc ${USE_CFLAGS} ${USE_LDFLAGS} isaparentof.o -o bin/isaparentof

bin/isachildof : ${DEPS} isachildof.o
	gcc ${USE_CFLAGS} ${USE_LDFLAGS} isachildof.o -o bin/isachildof

bin/getppid : ${DEPS}  getppid.o
	gcc ${USE_CFLAGS} ${USE_LDFLAGS} getppid.o -o bin/getppid

bin/getcpids : ${DEPS} getcpids.o
	gcc ${USE_CFLAGS} ${USE_LDFLAGS} getcpids.o -o bin/getcpids

bin/getpcmd : ${DEPS} getpcmd.o
	gcc ${USE_CFLAGS} ${USE_LDFLAGS} getpcmd.o -o bin/getpcmd

waitpid.o : waitpid.c
	gcc ${USE_CFLAGS} waitpid.c -c -o waitpid.o

bin/waitpid: waitpid.o
	gcc ${USE_CFLAGS} waitpid.o -o bin/waitpid


remake:
	make clean
	make all
