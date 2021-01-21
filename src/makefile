include ../makefile.inc

# build destination directories
OBJDIR = ../output/$(MACHINE)

# TARGETS
TARGET_SO = $(OBJDIR)/$(COMPONENT).so.$(VERSION)
TARGET_A = $(OBJDIR)/$(COMPONENT).a

# directories
# source directories
SRCDIR = .
INCDIR_PUB = ../include
INCDIR_PRIV = ../include_priv
INCDIRS = $(INCDIR_PUB) $(INCDIR_PRIV) $(if $(STAGINGDIR), $(STAGINGDIR)/include) $(if $(STAGINGDIR), $(STAGINGDIR)/usr/include)
STAGING_LIBDIR = $(if $(STAGINGDIR), -L$(STAGINGDIR)/lib) $(if $(STAGINGDIR), -L$(STAGINGDIR)/usr/lib)

# files
HEADERS = $(wildcard $(INCDIR_PUB)/amxo/*.h)
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o)))
OBJECTS += $(OBJDIR)/lex.amxo_parser.o \
           $(OBJDIR)/amxo_parser.tab.o \
		  
		
# compilation and linking flags
CFLAGS += -Werror -Wall -Wextra \
          -Wformat=2 -Wshadow \
          -Wwrite-strings -Wredundant-decls \
		  -Wpedantic -Wmissing-declarations -Wno-attributes \
		  -Wno-format-nonliteral \
          -fPIC -g3 $(addprefix -I ,$(INCDIRS)) -I $(SRCDIR) -I$(OBJDIR) 

ifeq ($(CC_NAME),g++)
    CFLAGS += -std=c++2a
else
	CFLAGS += -Wstrict-prototypes -Wold-style-definition -Wnested-externs -std=c11
endif
		  
LDFLAGS += $(STAGING_LIBDIR) -shared -fPIC -Wl,--version-script=libamxo.version -lamxc -lamxp -lamxd -ldl

# targets
all: $(TARGET_SO) $(TARGET_A)

$(TARGET_SO): $(OBJECTS)
	$(CC) -Wl,-soname,$(COMPONENT).so.$(VMAJOR) -o $@ $(OBJECTS) $(LDFLAGS)

$(TARGET_A): $(OBJECTS)
	$(AR) rcs $(@) $(OBJECTS)

-include $(OBJECTS:.o=.d)

$(OBJDIR)/lex.amxo_parser.o $(OBJDIR)/lex.amxo_parser.c: $(OBJDIR)/amxo_parser.tab.h amxo_parser.l | $(OBJDIR)/
	flex --header-file=$(OBJDIR)/amxo_parser_flex.h -o $(OBJDIR)/lex.amxo_parser.c amxo_parser.l
	$(CC) $(CFLAGS) -Wno-sign-compare -c -o $(OBJDIR)/lex.amxo_parser.o $(OBJDIR)/lex.amxo_parser.c

$(OBJDIR)/amxo_parser.tab.o $(OBJDIR)/amxo_parser.tab.c $(OBJDIR)/amxo_parser.tab.h: amxo_parser.y | $(OBJDIR)/
	bison -d --verbose -o $(OBJDIR)/amxo_parser.tab.c amxo_parser.y
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/amxo_parser.tab.o $(OBJDIR)/amxo_parser.tab.c

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)/amxo_parser.tab.h | $(OBJDIR)/
	$(CC) $(CFLAGS) -c -o $@ $<
	@$(CC) $(CFLAGS) -MM -MP -MT '$(@) $(@:.o=.d)' -MF $(@:.o=.d) $(<)

$(OBJDIR)/:
	$(MKDIR) -p $@

clean:
	rm -rf ../output/ ../$(COMPONENT)-*.* ../$(COMPONENT)_*.*
	make -C ../test clean
	find ../ -name "run_test" -delete

.PHONY: all clean
