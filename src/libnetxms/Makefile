SRC=cscp.cpp main.cpp queue.cpp tools.cpp hash.cpp inline.cpp icmp.cpp
#OBJ=$(SRC:C/.cpp/.o/)
OBJ=cscp.o main.o queue.o tools.o hash.o inline.o icmp.o
#DST=libnetxms.a
DST=libnetxms.so

CFLAGS=-I../../include -Wall

all: $(DST)

$(DST): $(OBJ)
	rm -f $(DST)
#	$(AR) q $(DST) $(OBJ)
	gcc -shared -o $(DST) $(OBJ)

.SUFFIXES: .cpp .o
.cpp.o: $(SRC)
	$(CC) -c -o $@ $(CFLAGS) $<

clean:
	rm -f $(OBJ) $(DST)
