javalibdir = $(pkglibdir)
javalib_DATA = netxms-server.jar

netxms-server.jar: pom.xml
	mvn clean
	mvn install
	mv target/netxms-server.jar .

clean-local:
	rm -rf netxms-server.jar

EXTRA_DIST = Makefile.w32 pom.xml netxms-server.jar
