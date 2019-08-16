javalibdir = $(pkglibdir)
javalib_DATA = netxms-java-bridge.jar

netxms-java-bridge.jar: pom.xml
	mvn clean
	mvn install
	mv target/netxms-java-bridge.jar .

clean-local:
	rm -rf netxms-java-bridge.jar

EXTRA_DIST = Makefile.w32 pom.xml netxms-java-bridge.jar
