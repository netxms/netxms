javalibdir = $(pkglibdir)
javalib_DATA = bind9.jar

bind9.jar: pom.xml
	mvn clean
	mvn package
	mv target/bind9.jar .

EXTRA_DIST = pom.xml bind9.jar
