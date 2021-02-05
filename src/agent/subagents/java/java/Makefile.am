install-exec-hook:
	mvn -Dmaven.test.skip=true -Drevision=$(PACKAGE_VERSION) install
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp target/netxms-agent-$(PACKAGE_VERSION).jar $(DESTDIR)$(pkglibdir)/java/

EXTRA_DIST = pom.xml src

clean-local:
	rm -rf target
