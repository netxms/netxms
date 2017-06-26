javalibdir = $(pkglibdir)
javalib_DATA = nxshell.jar

nxshell.jar: pom.xml
	mvn clean
	MAVEN_OPTS=-Xmx512m mvn package -Pnxshell-launcher
	mv target/nxshell.jar .

EXTRA_DIST = pom.xml nxshell.jar Makefile.w32

clean-local:
	rm -f nxshell.jar
