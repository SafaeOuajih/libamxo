all: $(TARGET)

run: $(TARGET)
	set -o pipefail; CMOCKA_TEST_ABORT='1' valgrind --error-exitcode=1 ./$< 2>&1 | tee -a $(OBJDIR)/unit_test_results.txt;

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) -fprofile-arcs -ftest-coverage

-include $(OBJECTS:.o=.d)

$(OBJDIR)/../lex.amxo_parser.o $(OBJDIR)/../lex.amxo_parser.c: $(OBJDIR)/../amxo_parser.tab.h ../../src/amxo_parser.l | $(OBJDIR)/
	flex --header-file=$(OBJDIR)/../amxo_parser_flex.h -o $(OBJDIR)/../lex.amxo_parser.c ../../src/amxo_parser.l
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/../lex.amxo_parser.o $(OBJDIR)/../lex.amxo_parser.c

$(OBJDIR)/../amxo_parser.tab.o $(OBJDIR)/../amxo_parser.tab.c $(OBJDIR)/../amxo_parser.tab.h: ../../src/amxo_parser.y | $(OBJDIR)/
	bison -d --verbose -o $(OBJDIR)/../amxo_parser.tab.c ../../src/amxo_parser.y
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/../amxo_parser.tab.o $(OBJDIR)/../amxo_parser.tab.c

$(OBJDIR)/%.o: ./%.c $(OBJDIR)/../amxo_parser.tab.h | $(OBJDIR)/
	$(CC) $(CFLAGS) -fprofile-arcs -ftest-coverage -c -o $@ $<
	@$(CC) $(CFLAGS) -MM -MP -MT '$(@) $(@:.o=.d)' -MF $(@:.o=.d) $(<)
	
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)/../amxo_parser.tab.h | $(OBJDIR)/
	$(CC) $(CFLAGS) -fprofile-arcs -ftest-coverage -c -o $@ $<
	@$(CC) $(CFLAGS) -MM -MP -MT '$(@) $(@:.o=.d)' -MF $(@:.o=.d) $(<)

$(OBJDIR)/:
	mkdir -p $@

clean:
	rm -rf $(TARGET) $(OBJDIR) 

.PHONY: clean $(OBJDIR)/
