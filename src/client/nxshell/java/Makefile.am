javalibdir = $(pkglibdir)
javalib_DATA = nxshell.jar

nxshell.jar: pom.xml
	mvn clean
	mvn package -Pnxshell-launcher
	mv target/nxshell.jar .
