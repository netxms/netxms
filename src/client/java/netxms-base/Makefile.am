JAR = netxms-base.jar
javalibdir = $(pkglibdir)
javalib_DATA = $(JAR)

$(JAR): pom.xml
	mvn clean
	MAVEN_OPTS=-Xmx512m mvn -Dmaven.test.skip=true install -Ppkg-build
	mv target/$(JAR) .

EXTRA_DIST = $(JAR) pom.xml Makefile.w32

clean-local:
	rm -f $(JAR)
