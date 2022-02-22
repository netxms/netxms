JAR_BASE_NAME=netxms-java-bridge

install-exec-hook:
	@top_srcdir@/build/get_jar_file.sh $(JAR_BASE_NAME) @NETXMS_JAR_VERSION@
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp $(JAR_BASE_NAME)-@NETXMS_JAR_VERSION@.jar $(DESTDIR)$(pkglibdir)/java/

EXTRA_DIST = pom.xml src

clean-local:
	rm -rf target *.jar
