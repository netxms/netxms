SUBAGENT_NAME=opcua
JAR_BASE_NAME=netxms-subagent-$(SUBAGENT_NAME)

install-exec-hook:
if RELEASE_BUILD
	@top_srcdir@/build/get_jar_file.sh $(JAR_BASE_NAME) @NETXMS_JAR_VERSION@
else
	mvn clean package
	cp target/$(JAR_BASE_NAME)-@NETXMS_JAR_VERSION@.jar .
endif
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp $(JAR_BASE_NAME)-@NETXMS_JAR_VERSION@.jar $(DESTDIR)$(pkglibdir)/java/$(SUBAGENT_NAME).jar

EXTRA_DIST = pom.xml src

clean-local:
	rm -rf target *.jar
