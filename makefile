include makefile.inc

NOW = $(shell date +"%Y-%m-%d(%H:%M:%S %z)")

# build destination directories
OBJDIR = ./output/$(MACHINE)
PKGDIR = ./output/$(MACHINE)/pkg/

# TARGETS
TARGET_SO = $(OBJDIR)/$(TARGET)-$(VERSION).so
TARGET_A = $(OBJDIR)/$(TARGET)-$(VERSION).a

# directories
# source directories
SRCDIR = ./src
INCDIR_PUB = ./include
INCDIR_PRIV = ./include_priv
INCDIRS = $(INCDIR_PUB) $(INCDIR_PRIV) $(if $(STAGINGDIR), $(STAGINGDIR)/include)
LIBDIR = $(if $(STAGINGDIR), -L$(STAGINGDIR)/lib)

# files
HEADERS = $(wildcard $(INCDIR_PUB)/$(TARGET_NAME)/*.h)
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o)))
OBJECTS += $(OBJDIR)/lex.amxo_parser.o \
           $(OBJDIR)/amxo_parser.tab.o \
		  
		
# compilation and linking flags
CFLAGS += -Werror -Wall -Wextra \
          -Wformat=2 -Wshadow \
          -Wwrite-strings -Wredundant-decls -Wmissing-include-dirs \
		  -Wpedantic -Wmissing-declarations -Wno-attributes \
		  -Wno-format-nonliteral \
          -fPIC -g3 $(addprefix -I ,$(INCDIRS)) -I $(SRCDIR) -I$(OBJDIR) 

ifeq ($(CC_NAME),g++)
    CFLAGS += -std=c++2a
else
	CFLAGS += -Wstrict-prototypes -Wold-style-definition -Wnested-externs -std=c11
endif
		  
LDFLAGS += $(LIBDIR) -shared -fPIC -Wl,--version-script=libamxo.version -lamxc -lamxp -lamxd -ldl

# helper functions - used in multiple targets
define install_to
	$(INSTALL) -d $(1)$(PREFIX)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(1)$(PREFIX)$(INSTALL_INC_DIR)
	$(INSTALL) -d $(1)$(PREFIX)$(INSTALL_DOC_DIR)
	$(INSTALL) -m 0644 $(TARGET_SO) $(1)$(PREFIX)$(INSTALL_LIB_DIR)/$(TARGET)-$(VERSION).so
	$(INSTALL) -m 0644 $(TARGET_A) $(1)$(PREFIX)$(INSTALL_LIB_DIR)/$(TARGET)-$(VERSION).a
	$(INSTALL) -m 0644 $(HEADERS) $(1)$(PREFIX)$(INSTALL_INC_DIR)
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
all: $(TARGET_SO) $(TARGET_A)

$(TARGET_SO): $(OBJECTS)
	$(CC) -Wl,-soname,$(TARGET)-$(VMAJOR).so -o $@ $(OBJECTS) $(LDFLAGS)

$(TARGET_A): $(OBJECTS)
	$(AR) rcs $(@) $^

-include $(OBJECTS:.o=.d)

$(OBJDIR)/lex.amxo_parser.o $(OBJDIR)/lex.amxo_parser.c: $(OBJDIR)/amxo_parser.tab.h src/amxo_parser.l | $(OBJDIR)/
	flex --header-file=$(OBJDIR)/amxo_parser_flex.h -o $(OBJDIR)/lex.amxo_parser.c src/amxo_parser.l
	$(CC) $(CFLAGS) -Wno-sign-compare -c -o $(OBJDIR)/lex.amxo_parser.o $(OBJDIR)/lex.amxo_parser.c

$(OBJDIR)/amxo_parser.tab.o $(OBJDIR)/amxo_parser.tab.c $(OBJDIR)/amxo_parser.tab.h: src/amxo_parser.y | $(OBJDIR)/
	bison -d --verbose -o $(OBJDIR)/amxo_parser.tab.c src/amxo_parser.y
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/amxo_parser.tab.o $(OBJDIR)/amxo_parser.tab.c

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)/amxo_parser.tab.h | $(OBJDIR)/
	$(CC) $(CFLAGS) -c -o $@ $<
	@$(CC) $(CFLAGS) -MM -MP -MT '$(@) $(@:.o=.d)' -MF $(@:.o=.d) $(<)

$(OBJDIR)/:
	$(MKDIR) -p $@

changelog:
	$(call create_changelog) 

package: $(TARGET_SO) $(TARGET_A) $(HEADERS)
	$(call install_to,$(PKGDIR)) 
	cd $(PKGDIR) && $(TAR) -czvf ../$(TARGET)-$(VERSION).tar.gz .
	cp $(PKGDIR)../$(TARGET)-$(VERSION).tar.gz .
	make -C packages

install: $(TARGET_SO) $(TARGET_A) $(HEADERS)
	$(call install_to,$(DEST))
	-rm -rf $(DEST)$(PREFIX)$(INSTALL_LIB_DIR)/$(TARGET).so
	cd $(DEST)$(PREFIX)$(INSTALL_LIB_DIR) && ln -fs $(TARGET)-$(VERSION).so $(TARGET).so
	cd $(DEST)$(PREFIX)$(INSTALL_LIB_DIR) && ln -fs $(TARGET)-$(VERSION).so $(TARGET)-$(VMAJOR).so

test: 
	make -C tests
	make -C tests coverage

doc: lib$(TARGET_NAME).doxy
	mkdir -p ./output/doc
	VERSION=$(VERSION) doxygen $<

clean:
	rm -rf ./output/ $(TARGET)-*.* $(TARGET)_*.*
	make -C tests clean
	find . -name "run_test" -delete


.PHONY: clean test doc package changelog