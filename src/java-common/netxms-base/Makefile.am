JAR = netxms-base.jar
javalibdir = $(pkglibdir)
javalib_DATA = $(JAR)

$(JAR): pom.xml
	mvn clean
	cp ../../../build/netxms-build-tag.properties ./src/main/resources/
	mvn -Dmaven.test.skip=true install -Ppkg-build
	mv target/$(JAR) .

EXTRA_DIST = $(JAR) pom.xml 

clean-local:
	rm -f $(JAR)
