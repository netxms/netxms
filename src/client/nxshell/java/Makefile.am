EXTRA_DIST = pom.xml src

install-exec-hook:
	mvn -Drevision=$(PACKAGE_VERSION) -Dmaven.test.skip=true package
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp target/nxshell-$(PACKAGE_VERSION).jar $(DESTDIR)$(pkglibdir)/java/
	cp target/lib/*.jar $(DESTDIR)$(pkglibdir)/java/

clean-local:
	rm -rf target
