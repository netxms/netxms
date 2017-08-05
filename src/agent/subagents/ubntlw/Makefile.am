javalibdir = $(pkglibdir)
javalib_DATA = ubntlw.jar

ubntlw.jar: pom.xml
	mvn clean
	mvn package
	mv target/ubntlw.jar .

clean-local:
	rm -rf target ubntlw.jar

EXTRA_DIST = Makefile.w32 pom.xml ubntlw.jar LICENSE README
