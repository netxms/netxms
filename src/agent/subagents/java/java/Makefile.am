javalibdir = $(pkglibdir)
javalib_DATA = netxms-agent.jar

netxms-agent.jar: pom.xml
	mvn clean
	mvn install
	mv target/netxms-agent.jar .

SUBDIRS = src

EXTRA_DIST = pom.xml netxms-agent.jar
