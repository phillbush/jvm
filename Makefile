JAVAOBJS  := java.o  util.o class.o file.o memory.o native.o
JAVAPOBJS := javap.o util.o class.o file.o
OBJS      := java.o  util.o class.o file.o memory.o native.o
SRCS      := ${OBJS:.o=.c}

JAVAP := javap
JAVA  := java

CLASSES := tests/HelloWorld.class \
           tests/Double.class \
           tests/Echo.class \
           tests/Int.class \
           tests/Multi.class \
           tests/TableSwitch.class \
           tests/Vector1.class \
           tests/Vector2.class
TESTP := ${CLASSES:.class=.p}
TESTJ := ${CLASSES:.class=.j}

LIBS = -lm
INCS =
CPPFLAGS = -D_POSIX_C_SOURCE=200809L
CFLAGS = -g -O0 -std=c99 -Wall -Wextra ${INCS} ${CPPFLAGS}
LDFLAGS = ${LIBS}
LINT = splint
LINTFLAGS = -nullret -predboolint
JAVAPFLAGS = -vp

# .p and .j are dummy suffixes we use for testing
.SUFFIXES: .p .j .java .class

# main targets
all: ${JAVA} ${JAVAP}

classes: ${CLASSES}

testp: ${TESTP}
testj: ${TESTJ}

lint:
	-${LINT} ${CPPFLAGS} ${LINTFLAGS} ${SRCS}

${JAVA}: ${JAVAOBJS}
	${CC} -o $@ ${JAVAOBJS} ${LDFLAGS}

${JAVAP}: ${JAVAPOBJS}
	${CC} -o $@ ${JAVAPOBJS} ${LDFLAGS}

java.o:   class.h util.h file.h memory.h native.h
javap.o:  class.h util.h file.h
file.o:   class.h util.h
native.o: class.h memory.h native.h
memory.o: class.h memory.h
class.o:  class.h util.h

.c.o:
	${CC} ${CFLAGS} -c $<

${TESTP}: javap
${TESTJ}: java

# test the disassembler (javap) on the test classes
.class.p:
	@echo
	@echo "========== Disassembling $<"
	@./${JAVAP} ${JAVAPFLAGS} $<

# test the jvm (java) on the test classes
.class.j:
	@echo
	@echo "========== Running $<"
	@./${JAVA} -cp "$$(echo $< | sed 's,/[^/]*,,')" ${JAVAFLAGS} "$$(echo $< | sed 's,.*/,,; s,.class,,')"

# compile the test classes
.java.class:
	javac $<

clean:
	-rm ${JAVA} ${JAVAP} ${OBJS} ${CLASSES} 2>/dev/null

.PHONY: all clean lint testp testj
