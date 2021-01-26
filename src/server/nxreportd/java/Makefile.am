javalibdir = $(pkglibdir)
javalib_DATA = nxreportd.jar

nxreportd.jar: pom.xml
	mvn clean
	mvn package -Plauncher
	mv target/nxreportd.jar .

EXTRA_DIST = pom.xml nxreportd.jar

clean-local:
	rm -rf nxreportd.jar target
