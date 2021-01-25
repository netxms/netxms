javalibdir = $(pkglibdir)
javalib_DATA = nxshell.jar

nxshell.jar: pom.xml
	mvn clean
	mvn package -Plauncher
	mv target/nxshell.jar .

EXTRA_DIST = pom.xml nxshell.jar

clean-local:
	rm -f nxshell.jar
