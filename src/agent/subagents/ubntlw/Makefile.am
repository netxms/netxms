SUBAGENT_NAME=ubntlw
JAR_BASE_NAME=netxms-subagent-$(SUBAGENT_NAME)

install-exec-hook:
	@top_srcdir@/build/get_jar_file.sh $(JAR_BASE_NAME) @NETXMS_JAR_VERSION@ com.rfelements
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp $(JAR_BASE_NAME)-@NETXMS_JAR_VERSION@.jar $(DESTDIR)$(pkglibdir)/java/$(SUBAGENT_NAME).jar

EXTRA_DIST = pom.xml src LICENSE README

clean-local:
	rm -rf target
