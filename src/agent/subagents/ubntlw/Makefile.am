SUBAGENT_NAME=ubntlw
JAR_BASE_NAME=netxms-subagent-$(SUBAGENT_NAME)

install-exec-hook:
	mvn clean package
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp target/$(SUBAGENT_NAME).jar $(DESTDIR)$(pkglibdir)/java/$(SUBAGENT_NAME).jar

EXTRA_DIST = pom.xml src LICENSE README

clean-local:
	rm -rf target *.jar
