# tlock version
VERSION = 0.1

# Name of the output file.
NAME = tlock

# Path prefix.
PREFIX = /usr/local

# Directory to write the output file to.
BINDIR = /bin

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# Imports and libraries.
INCS = -I. -I/usr/include -I${X11INC}
LIBS = -L/usr/lib -lc -lcrypt -L${X11LIB} -lX11 -lXext

# Flags.
CPPFLAGS = -DVERSION=\"${VERSION}\" -DHAVE_SHADOW_H -DCOLOR1=\"black\" -DCOLOR2=\"\#005577\"
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -s ${LIBS}

# On *BSD remove -DHAVE_SHADOW_H from CPPFLAGS and add 
# -DHAVE_BSD_AUTH
# On OpenBSD and Darwin remove -lcrypt from LIBS

# Compiler and linker.
CC = cc

# Install mode. On BSD systems MODE=2755 and GROUP=auth
# On others MODE=4755 and GROUP=root
#MODE=2755
#GROUP=auth
