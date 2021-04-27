JAVAOBJS  = java.o  util.o class.o file.o memory.o native.o
JAVAPOBJS = javap.o util.o class.o file.o

LIBS = -lm
INCS =
CPPFLAGS = -D_POSIX_C_SOURCE=200809L
CFLAGS = -g -O0 -std=c99 -Wall -Wextra ${INCS} ${CPPFLAGS}
LDFLAGS = ${LIBS}
LINT = splint
LINTFLAGS = -nullret -predboolint

all: java javap

java: ${JAVAOBJS}
	${CC} -o $@ ${JAVAOBJS} ${LDFLAGS}

javap: ${JAVAPOBJS}
	${CC} -o $@ ${JAVAPOBJS} ${LDFLAGS}

java.o:   class.h util.h file.h memory.h native.h
javap.o:  class.h util.h file.h
file.o:   class.h util.h
native.o: class.h memory.h native.h
memory.o: class.h memory.h
class.o:  class.h util.h

lint:
	-${LINT} ${CPPFLAGS} ${LINTFLAGS} javap.c util.c class.c file.c

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	-rm java javap *.o

.PHONY: all clean lint
