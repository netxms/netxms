CFLAGS = -O2 -I../../../include -I../include -I/usr/kerberos/include
LFLAGS = -L../../libnetxms -lstdc++ -lnetxms -lpthread -lsnmp -lssl
OBJECTS = messages.o acl.o actions.o admin.o agent.o client.o config.o \
	  datacoll.o db.o dbwrite.o dcitem.o dcithreshold.o debug.o \
	  discovery.o dload.o entirenet.o epp.o events.o evproc.o hk.o id.o \
	  interface.o locks.o log.o main.o netobj.o node.o np.o objects.o \
	  session.o snmp.o status.o subnet.o syncer.o tools.o users.o \
	  watchdog.o winsrv.o
EXEC = netxmsd

.cpp.o:
	gcc $(CFLAGS) -c -o $@ $<

.c.o:
	gcc $(CFLAGS) -c -o $@ $<

all: $(EXEC)

$(EXEC): $(OBJECTS)
	gcc $(LFLAGS) -o $(EXEC) $(OBJECTS)

messages.c: messages.mc
	perl ../../../tools/mc.pl $<

clean:
	rm -f *.o $(EXEC) messages.c messages.h
