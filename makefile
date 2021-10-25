include makefile.inc

NOW = $(shell date +"%Y-%m-%d(%H:%M:%S %z)")

# Extra destination directories
PKGDIR = ./output/$(MACHINE)/pkg/

# helper functions - used in multiple targets
define install_to
	$(INSTALL) -d -m 0755 $(1)/$(INCLUDEDIR)/amxo
	$(INSTALL) -D -p -m 0644 include/amxo/*.h $(1)$(INCLUDEDIR)/amxo/
	$(INSTALL) -D -p -m 0644 output/$(MACHINE)/$(COMPONENT).so.$(VERSION) $(1)$(LIBDIR)/$(COMPONENT).so.$(VERSION)
endef

define create_changelog
	@$(ECHO) "Update changelog"
	mv CHANGELOG.md CHANGELOG.md.bak
	head -n 9 CHANGELOG.md.bak > CHANGELOG.md
	$(ECHO) "" >> CHANGELOG.md
	$(ECHO) "## Release $(VERSION) - $(NOW)" >> CHANGELOG.md
	$(ECHO) "" >> CHANGELOG.md
	$(GIT) log --pretty=format:"- %s" $$($(GIT) describe --tags | grep -v "merge" | cut -d'-' -f1)..HEAD  >> CHANGELOG.md
	$(ECHO) "" >> CHANGELOG.md
	tail -n +10 CHANGELOG.md.bak >> CHANGELOG.md
	rm CHANGELOG.md.bak
endef

# targets
all:
	$(MAKE) -C src all

clean:
	$(MAKE) -C src clean
	$(MAKE) -C test clean

install: all
	$(call install_to,$(DEST))
	ln -sfr $(DEST)$(LIBDIR)/$(COMPONENT).so.$(VERSION) $(DEST)$(LIBDIR)/$(COMPONENT).so.$(VMAJOR)
	ln -sfr $(DEST)$(LIBDIR)/$(COMPONENT).so.$(VERSION) $(DEST)$(LIBDIR)/$(COMPONENT).so

package: all
	$(call install_to,$(PKGDIR))
	cd $(PKGDIR) && $(TAR) -czvf ../$(COMPONENT)-$(VERSION).tar.gz .
	cp $(PKGDIR)../$(COMPONENT)-$(VERSION).tar.gz .
	make -C packages

changelog:
	$(call create_changelog)

doc:
	$(MAKE) -C doc doc


test:
	$(MAKE) -C test run
	$(MAKE) -C test coverage

.PHONY: all clean changelog install package doc test