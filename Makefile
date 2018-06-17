
# Targets:
#
#   all (default) - Make all using env CFLAGS and LDFLAGS if present,
#      otherwise optimized defaults
#
#   debug - Make using CFLAGS and LDFLAGS intended for debugging with gdb
#
#   static - Compile static versions of executables, using env CFLAGS / LDFLAGS
#     otherwise optimized defaults
#
#   native - Compile executeables using super-optimized CFLAGS / LDFLAGS
#     which are optimized (and will only run) on the current processor, or better.
#
#   native-static - Compile static versions of executables, using cflags from "native" targets.
#
#   clean - Clean all compiled files
#
#   distclean - Alias for clean ( Not automake generated, so "dist" prefix has no distinct meaning here)
#
#   remake - Cleans and recompiles
#
#   install - Installs executables into $DESTDIR/$PREFIX/bin , or $PREFIX/bin if DESTDIR is not defined ,
#      if neither are defined, detects if /usr/bin is writeable and if so installs there,
#      otherwise installs to $HOME/bin

#  NOTES: Changing CFLAGS or LDFLAGS will cause everything to be recompiled.

USER_CFLAGS = $(shell env | grep ^CFLAGS= | sed 's/^CFLAGS=//g')

# Default CFLAGS
CFLAGS ?= -O3 -flto -fuse-linker-plugin -s

# Default LDFLAGS
LDFLAGS ?= -flto -fuse-linker-plugin -Wl,-O1,--sort-common,--as-needed,-z,relro

# Debug CFLAGS
DEBUG_CFLAGS = -Og -ggdb3

# Debug LDFLAGS
DEBUG_LDFLAGS = -Wl,-Og -ggdb3

# Native CFLAGS
NATIVE_CFLAGS = -O3 -flto -fuse-linker-plugin -march=native -mtune=native -s

# Native LDFLAGS
NATIVE_LDFLAGS = -flto -fuse-linker-plugin -Wl,-O1,--sort-common,--as-needed,-z,relro -s

# CFLAG to trigger static build
STATIC_CFLAG = -static

# LDFLAG to trigger static build
STATIC_LDFLAG = -static

# Determine if gnu99 is supported C mode, otherwise use c99
C_STANDARD=$(shell test -f .use_c_std && cat .use_c_std || (echo 'int main(int argc, char *argv[]) { return 0; }' > .uc.c; ${CC} -std=gnu99 .uc.c >/dev/null 2>&1 && (echo 'gnu99' > .use_c_std; echo 'gnu99'; rm -f .uc.c) || ( echo 'c99' > .use_c_std; echo 'c99'; rm -f .uc.c ) ))

# Actual CFLAGS to use
USE_CFLAGS = ${CFLAGS} -Wall -pipe -std=${C_STANDARD}

# Actual LDFLAGS to use
USE_LDFLAGS = ${LDFLAGS}

# Cause everything to recompile when CFLAGS changes, unless user is root (to support "sudo make install")
WHOAMI=$(shell whoami)
CFLAGS_HASH=$(shell echo "CFLAGS=${USE_CFLAGS} .. LDFLAGS=${USE_LDFLAGS}" | md5sum | tr ' ' '\n' | head -n1)
CFLAGS_HASH_FILE=$(shell test "${WHOAMI}" != "root" && echo .cflags.${CFLAGS_HASH} || echo .cflags.*)
_X=$(shell printf "%s" "${CFLAGS}" > .last_cflags && printf "%s" "${USE_LDFLAGS}" > .last_ldflags)

LAST_CFLAGS=$(shell cat .last_cflags)
LAST_LDFLAGS=$(shell cat .last_ldflags)



# DESTDIR - actual destination used, same as prefix. Standard param
#   when make instaling to a package dir (like PKGBUILD, or RPM spec)
DESTDIR ?= /

# Guess prefix based on if /usr/bin is writeable, otherwise use $HOME
PREFIX ?= $(shell test -w "${DESTDIR}" && echo "/usr" || echo "${HOME}")

INSTALLDIR = "${DESTDIR}/${PREFIX}"

# DEPS common for all units
#   * will recompile if CFLAGS changes,
#   * Ensures bin dir is created
#   * Will recompile if headers change
DEPS = bin/.created ${CFLAGS_HASH_FILE} pid_tools.h pid_utils.h

INODE_UTILS_DEPS = pid_inode_utils.h

# All output executables
ALL_FILES = bin/getppid \
	bin/getcpids \
	bin/isaparentof \
	bin/isachildof \
	bin/getpcmd \
	bin/waitpid \
	bin/getpenv


# TARGET all - Default target
all: ${DEPS} ${ALL_FILES}
	@ echo ${_X} >/dev/null 2>&1
#	@ /bin/true

# TARGET clean - Clean target
clean:
	rm -Rf bin
	rm -f *.o
	rm -f .cflags.*
	rm -f .last_cflags
	rm -f .last_ldflags

# TARGET distclean - Clean target
distclean:
	@ make clean

# TARGET install - Install stuff to destdir
install:
	[ -f ".last_cflags" -a -z "${USER_CFLAGS}" ] && (export CFLAGS="${LAST_CFLAGS}" && export LDFLAGS="${LAST_LDFLAGS}" && make _install DESTDIR="${DESTDIR}" PREFIX="${PREFIX}") || make all _install DESTDIR="${DESTDIR}" PREFIX="${PREFIX}"


_install: ${ALL_FILES}
	mkdir -p "${INSTALLDIR}/bin"
	install -m 775 ${ALL_FILES} "${INSTALLDIR}/bin"


# When hash of CFLAGS changes, this unit causes all compiles to become invalidated
${CFLAGS_HASH_FILE}:
	test "${WHOAMI}" != "root" -a ! -e "${CFLAGS_HASH_FILE}" && rm -f .cflags.* || true
	touch "${CFLAGS_HASH_FILE}"

# TARGET - static
static:
	CFLAGS="${CFLAGS} ${STATIC_CFLAG}" make

# TARGET - debug
debug:
	CFLAGS="${DEBUG_CFLAGS}" LDFLAGS="${DEBUG_LDFLAGS}" make

# TARGET - native
native:
	CFLAGS="${NATIVE_CFLAGS}" LDFLAGS="${NATIVE_LDFLAGS}" make

# TARGET - static-native
static-native:
	CFLAGS="${NATIVE_CFLAGS} ${STATIC_CFLAG}" LDFLAGS="${NATIVE_LDFLAGS} ${STATIC_LDFLAG}" make

# TARGET - remake
remake:
	make clean
	make all

native-static:
	@ echo "No target native-static. I think you mean 'static-native'" >&2
	@ false

# Ensure bin dir is created. Seems to be a bug in some (all?) versions of Make
#   where just listing "bin" as the target causes running make to keep remaking,
#   as the mtime changes in the dir as files inside are created
bin/.created:
	mkdir -p bin
	touch bin/.created

########
#  OBJECTS
##############

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

waitpid.o : ${DEPS} waitpid.c
	gcc ${USE_CFLAGS} waitpid.c -c -o waitpid.o

getpenv.o : ${DEPS} getpenv.c
	gcc ${USE_CFLAGS} getpenv.c -c -o getpenv.o


########
#  EXECUTABLES
##################

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

bin/getpenv : ${DEPS} getpenv.o
	gcc ${USE_CFLAGS} ${USE_LDFLAGS} getpenv.o -o bin/getpenv
	

bin/waitpid: ${DEPS} waitpid.o
	gcc ${USE_CFLAGS} waitpid.o -o bin/waitpid


