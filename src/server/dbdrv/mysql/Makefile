OBJ = mysql.o
#DST=libnetxms.a
DST = mysql.ddr

CFLAGS = -I../../include -I../../../../include -Wall
LFLAGS = -shared -lmysql

all: $(DST)

$(DST): $(OBJ)
	rm -f $(DST)
#	$(AR) q $(DST) $(OBJ)
	gcc $(LFLAGS) -o $(DST) $(OBJ)

.SUFFIXES: .cpp .o
.cpp.o: $(SRC)
	$(CC) -c -o $@ $(CFLAGS) $<

clean:
	rm -f $(OBJ) $(DST)
