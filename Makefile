# ${NAME} - tightened screen locker
# See LICENSE file for copyright and license details.

include config.mk

SRC = ${NAME}.c
OBJ = ${SRC:.c=.o}

all: options ${NAME}

options:
	@echo ${NAME} build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

${NAME}: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f ${NAME} ${OBJ} ${NAME}-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p ${NAME}-${VERSION}
	@cp -R LICENSE Makefile README config.mk ${SRC} ${NAME}-${VERSION}
	@tar -cf ${NAME}-${VERSION}.tar ${NAME}-${VERSION}
	@gzip ${NAME}-${VERSION}.tar
	@rm -rf ${NAME}-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}${BINDIR}
	@mkdir -p ${DESTDIR}${PREFIX}${BINDIR}
	@cp -f ${NAME} ${DESTDIR}${PREFIX}${BINDIR}
	@chmod 755 ${DESTDIR}${PREFIX}${BINDIR}/${NAME}
	@chmod u+s ${DESTDIR}${PREFIX}${BINDIR}/${NAME}

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}${BINDIR}
	@rm -f ${DESTDIR}${PREFIX}${BINDIR}/${NAME}

.PHONY: all options clean dist install uninstall
