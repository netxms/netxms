javalibdir = $(pkglibdir)
javalib_DATA = netxms-agent.jar

netxms-agent.jar: pom.xml
	mvn clean
	mvn install
	mv target/netxms-agent.jar .

clean-local:
	rm -rf netxms-agent.jar

EXTRA_DIST = Makefile.w32 pom.xml netxms-agent.jar
