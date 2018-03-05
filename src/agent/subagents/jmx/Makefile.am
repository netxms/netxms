javalibdir = $(pkglibdir)
javalib_DATA = jmx.jar

jmx.jar: pom.xml
	mvn clean
	mvn package
	mv target/jmx.jar .

EXTRA_DIST = pom.xml jmx.jar
