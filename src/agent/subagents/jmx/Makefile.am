javalibdir = $(pkglibdir)
javalib_DATA = jmx.jar

jmx.jar: pom.xml
	mvn clean
	mvn package
	mv target/jmx.jar .

clean-local:
	rm -rf target $(javalib_DATA)

EXTRA_DIST = pom.xml jmx.jar
