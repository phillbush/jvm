CLASS = HelloWorld.class \
        Double.class \
        Echo.class \
        Int.class \
        Multi.class \
        TableSwitch.class \
        Vector1.class \
        Vector2.class

.SUFFIXES: .java .class

all: ${CLASS}

.java.class:
	javac $<

clean:
	-rm ${CLASS}

.PHONY: all clean
