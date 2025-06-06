# dwm version
VERSION = 6.5

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

MIMALLOC_INC = ./include/mimalloc/include
MIMALLOC_LIB = ./include/mimalloc/out/release





JEMALLOC_LIB = ./include/jemalloc/lib/libjemalloc.so
JEMALLOC_INC = ./include/jemalloc/include


# Xinerama, comment if you don't want it
XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA


MIMALLOCFLAGS = -DMI_SECURE=0 \
                -DMI_DEBUG=0 \
                -DMI_STAT=0 \
                -DMI_TRACK=0 \
                -DMI_PURGE_DECOMMITS=1 \
                -DMI_LOCAL_DYNAMIC_TLS=0 \
                -DMI_USE_ENVIRON=0 \
                -DMI_NO_THP=1 \
                -DMI_XMALLOC=0 \
                -DMI_ASAN=0 \
                -DMI_TRACK_VALID=0 \
                -DMI_VALGRIND=0 \
                -DMIMALLOC_EAGER_COMMIT=0

                
# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2
# OpenBSD (uncomment)
#FREETYPEINC = ${X11INC}/freetype2
#MANPREFIX = ${PREFIX}/man

# includes and libs
INCS = -I${X11INC} -I${FREETYPEINC} -I${JEMALLOC_INC}
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS} ${FREETYPELIBS}  -L${JEMALLOC_LIB} -ljemalloc
# -L${MIMALLOC_LIB} -lmimalloc

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700L -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
#mimmallocflags
#CFLAGS   = -g -std=c99 -pedantic -Wall -O0 ${INCS} ${CPPFLAGS}
CFLAGS   = -std=c99   -march=native -O2 -pedantic -Wall   ${INCS} ${CPPFLAGS}  -finline-functions -finline-small-functions
LDFLAGS  = ${LIBS}
# -rpath=${MIMALLOC_LIB}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}

# compiler and linker
CC = cc
