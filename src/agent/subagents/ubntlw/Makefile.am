javalibdir = $(pkglibdir)
javalib_DATA = ubntlw.jar

ubntlw.jar: pom.xml
	mvn clean
	mvn package
	mv target/ubntlw.jar .

EXTRA_DIST = ubntlw.jar
