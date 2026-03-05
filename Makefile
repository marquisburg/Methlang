CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE
SRCDIR = src
OBJDIR = obj
BINDIR = bin
STDLIBDIR = stdlib
RUNTIMEDIR = src/runtime

# Source files
LEXER_SOURCES = $(SRCDIR)/lexer/lexer.c
PARSER_SOURCES = $(SRCDIR)/parser/parser.c $(SRCDIR)/parser/ast.c
SEMANTIC_SOURCES = $(SRCDIR)/semantic/symbol_table.c $(SRCDIR)/semantic/type_checker.c $(SRCDIR)/semantic/register_allocator.c $(SRCDIR)/semantic/import_resolver.c $(SRCDIR)/semantic/monomorphize.c
IR_SOURCES = $(wildcard $(SRCDIR)/ir/*.c)
CODEGEN_SOURCES = $(wildcard $(SRCDIR)/codegen/*.c)
ERROR_SOURCES = $(SRCDIR)/error/error_reporter.c
DEBUG_SOURCES = $(SRCDIR)/debug/debug_info.c
MAIN_SOURCES = $(SRCDIR)/main.c
RUNTIME_SOURCES = $(SRCDIR)/runtime/gc.c

SOURCES = $(LEXER_SOURCES) $(PARSER_SOURCES) $(SEMANTIC_SOURCES) $(IR_SOURCES) $(CODEGEN_SOURCES) $(ERROR_SOURCES) $(DEBUG_SOURCES) $(RUNTIME_SOURCES) $(MAIN_SOURCES)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET = $(BINDIR)/methlang

.PHONY: all clean test install bundle-stdlib bundle-runtime

all: $(TARGET) bundle-stdlib bundle-runtime

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@

bundle-stdlib: | $(BINDIR)
	rm -rf $(BINDIR)/stdlib
	cp -r $(STDLIBDIR) $(BINDIR)/stdlib

bundle-runtime: | $(BINDIR)
	rm -rf $(BINDIR)/runtime
	cp -r $(RUNTIMEDIR) $(BINDIR)/runtime

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)/lexer $(OBJDIR)/parser $(OBJDIR)/semantic $(OBJDIR)/ir $(OBJDIR)/codegen $(OBJDIR)/error $(OBJDIR)/debug $(OBJDIR)/runtime

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

test: $(TARGET)
	@echo "Running GC runtime tests..."
	$(CC) $(CFLAGS) tests/gc_runtime_test.c src/runtime/gc.c -o $(BINDIR)/gc_runtime_test
	@$(BINDIR)/gc_runtime_test

install: $(TARGET) bundle-stdlib bundle-runtime
	mkdir -p /usr/local/bin /usr/local/stdlib /usr/local/runtime
	cp $(TARGET) /usr/local/bin/
	cp -r $(BINDIR)/stdlib/* /usr/local/stdlib/
	cp -r $(BINDIR)/runtime/* /usr/local/runtime/

.PHONY: debug
debug: CFLAGS += -DDEBUG
debug: $(TARGET)
