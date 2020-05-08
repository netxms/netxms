JAR = netxms-client.jar
javalibdir = $(pkglibdir)
javalib_DATA = $(JAR)

$(JAR): pom.xml
	mvn clean
	mvn -Dmaven.test.skip=true install -Ppkg-build
	mv target/$(JAR) .

EXTRA_DIST = $(JAR) pom.xml

clean-local:
	rm -f $(JAR)
