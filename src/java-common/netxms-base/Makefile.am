install-exec-hook:
	cp ../../../build/netxms-build-tag.properties ./src/main/resources/
	mvn -Dmaven.test.skip=true -Drevision=$(PACKAGE_VERSION) install
	mkdir -p $(DESTDIR)$(pkglibdir)/java
	cp target/netxms-base-$(PACKAGE_VERSION).jar $(DESTDIR)$(pkglibdir)/java/

EXTRA_DIST = pom.xml src jzlib-license.txt

clean-local:
	rm -rf target
