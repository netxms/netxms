SRC=cscp.cpp main.cpp queue.cpp tools.cpp
OBJ=$(SRC:C/.cpp/.o/)
DST=libnmsutil.a

CFLAGS=-I../../include -Wall

all: $(DST)

$(DST): $(OBJ)
	@rm -f $(DST)
	@$(AR) q $(DST) $(OBJ)

.SUFFIXES: .cpp .o
.cpp.o: $(SRC)
	$(CC) -c -o $@ $(CFLAGS) $<

clean:
	@rm -f $(OBJ) $(DST)
