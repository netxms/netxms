install-exec-hook:
	mvn -Dmaven.test.skip=true -Drevision=$(PACKAGE_VERSION) package
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp target/ubntlw.jar $(DESTDIR)$(pkglibdir)/java/

EXTRA_DIST = pom.xml src LICENSE README

clean-local:
	rm -rf target
