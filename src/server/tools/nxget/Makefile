CFLAGS = -O2 -I../../../../include
LFLAGS = -L../../../libnetxms -lstdc++ -lnetxms
OBJECTS = nxget.o
EXEC = nxget

.cpp.o:
	gcc $(CFLAGS) -c -o $@ $<

all: $(EXEC)

$(EXEC): $(OBJECTS)
	gcc $(LFLAGS) -o $(EXEC) $(OBJECTS)

clean:
	rm -f *.o $(EXEC)
