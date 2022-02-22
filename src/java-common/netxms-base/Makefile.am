JAR_BASE_NAME=netxms-base

install-exec-hook:
	cp @top_srcdir@/build/netxms-build-tag.properties ./src/main/resources/
	@top_srcdir@/build/get_jar_file.sh $(JAR_BASE_NAME) @NETXMS_JAR_VERSION@
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp $(JAR_BASE_NAME)-@NETXMS_JAR_VERSION@.jar $(DESTDIR)$(pkglibdir)/java/

EXTRA_DIST = pom.xml src jzlib-license.txt

clean-local:
	rm -rf target *.jar
