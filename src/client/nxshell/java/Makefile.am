JAR_BASE_NAME=nxshell

EXTRA_DIST = pom.xml src

install-exec-hook:
	@top_srcdir@/build/get_jar_file.sh $(JAR_BASE_NAME) @NETXMS_JAR_VERSION@
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp $(JAR_BASE_NAME)-@NETXMS_JAR_VERSION@.jar $(DESTDIR)$(pkglibdir)/java/
	mvn dependency:copy-dependencies -DincludeScope=runtime
	cp target/dependency/*.jar $(DESTDIR)$(pkglibdir)/java/

clean-local:
	rm -rf target *.jar
