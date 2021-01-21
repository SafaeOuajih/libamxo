include ../test_defines.mk

TARGET = run_test
SOURCES += $(wildcard ./*.c)
OBJECTS = $(addprefix $(OBJDIR)/,$(notdir $(SOURCES:.c=.o))) \
          $(OBJDIR)/../amxo_parser.tab.o \
		  $(OBJDIR)/../lex.amxo_parser.o

include ../test_targets.mk
