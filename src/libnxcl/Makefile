OBJ=main.o misc.o comm.o msgwq.o objects.o request.o events.o inline.o eventdb.o
DST=libnxcl.so

CFLAGS = -I../../include -I/usr/kerberos/include -Wall
LFLAGS = -shared -L../libnetxms -lnetxms

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
