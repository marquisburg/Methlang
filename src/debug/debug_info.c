#include "debug_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function for string duplication
static char* string_duplicate(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

#define INITIAL_SYMBOL_CAPACITY 64
#define INITIAL_MAPPING_CAPACITY 256
#define INITIAL_RUNTIME_FUNCTION_CAPACITY 64
#define INITIAL_RUNTIME_LOCATION_CAPACITY 256

DebugInfo* debug_info_create(const char* source_filename, const char* assembly_filename) {
    DebugInfo* debug_info = malloc(sizeof(DebugInfo));
    if (!debug_info) return NULL;
    
    debug_info->symbols = malloc(sizeof(DebugSymbol) * INITIAL_SYMBOL_CAPACITY);
    if (!debug_info->symbols) {
        free(debug_info);
        return NULL;
    }
    
    debug_info->line_mappings = malloc(sizeof(SourceLineMapping) * INITIAL_MAPPING_CAPACITY);
    if (!debug_info->line_mappings) {
        free(debug_info->symbols);
        free(debug_info);
        return NULL;
    }
    
    debug_info->symbol_count = 0;
    debug_info->symbol_capacity = INITIAL_SYMBOL_CAPACITY;
    debug_info->mapping_count = 0;
    debug_info->mapping_capacity = INITIAL_MAPPING_CAPACITY;
    debug_info->runtime_functions =
        malloc(sizeof(RuntimeFunctionMapping) * INITIAL_RUNTIME_FUNCTION_CAPACITY);
    if (!debug_info->runtime_functions) {
        free(debug_info->line_mappings);
        free(debug_info->symbols);
        free(debug_info);
        return NULL;
    }

    debug_info->runtime_locations =
        malloc(sizeof(RuntimeLocationMapping) * INITIAL_RUNTIME_LOCATION_CAPACITY);
    if (!debug_info->runtime_locations) {
        free(debug_info->runtime_functions);
        free(debug_info->line_mappings);
        free(debug_info->symbols);
        free(debug_info);
        return NULL;
    }

    debug_info->runtime_function_count = 0;
    debug_info->runtime_function_capacity = INITIAL_RUNTIME_FUNCTION_CAPACITY;
    debug_info->runtime_location_count = 0;
    debug_info->runtime_location_capacity = INITIAL_RUNTIME_LOCATION_CAPACITY;
    
    debug_info->source_filename = string_duplicate(source_filename);
    debug_info->assembly_filename = string_duplicate(assembly_filename);
    
    return debug_info;
}

void debug_info_destroy(DebugInfo* debug_info) {
    if (!debug_info) return;
    
    // Clean up symbols
    for (size_t i = 0; i < debug_info->symbol_count; i++) {
        free(debug_info->symbols[i].name);
        free(debug_info->symbols[i].type_name);
        free(debug_info->symbols[i].register_name);
    }
    free(debug_info->symbols);
    
    // Clean up line mappings
    for (size_t i = 0; i < debug_info->mapping_count; i++) {
        free(debug_info->line_mappings[i].filename);
    }
    free(debug_info->line_mappings);

    for (size_t i = 0; i < debug_info->runtime_function_count; i++) {
        free(debug_info->runtime_functions[i].function_name);
        free(debug_info->runtime_functions[i].start_label);
        free(debug_info->runtime_functions[i].end_label);
        free(debug_info->runtime_functions[i].filename);
    }
    free(debug_info->runtime_functions);

    for (size_t i = 0; i < debug_info->runtime_location_count; i++) {
        free(debug_info->runtime_locations[i].function_name);
        free(debug_info->runtime_locations[i].address_label);
        free(debug_info->runtime_locations[i].filename);
    }
    free(debug_info->runtime_locations);
    
    free(debug_info->source_filename);
    free(debug_info->assembly_filename);
    free(debug_info);
}

static int debug_info_expand_symbols(DebugInfo* debug_info) {
    if (debug_info->symbol_count >= debug_info->symbol_capacity) {
        size_t new_capacity = debug_info->symbol_capacity * 2;
        DebugSymbol* new_symbols = realloc(debug_info->symbols, 
                                          sizeof(DebugSymbol) * new_capacity);
        if (!new_symbols) return 0;
        
        debug_info->symbols = new_symbols;
        debug_info->symbol_capacity = new_capacity;
    }
    return 1;
}

static int debug_info_expand_mappings(DebugInfo* debug_info) {
    if (debug_info->mapping_count >= debug_info->mapping_capacity) {
        size_t new_capacity = debug_info->mapping_capacity * 2;
        SourceLineMapping* new_mappings = realloc(debug_info->line_mappings,
                                                 sizeof(SourceLineMapping) * new_capacity);
        if (!new_mappings) return 0;
        
        debug_info->line_mappings = new_mappings;
        debug_info->mapping_capacity = new_capacity;
    }
    return 1;
}

static int debug_info_expand_runtime_functions(DebugInfo* debug_info) {
    if (debug_info->runtime_function_count >= debug_info->runtime_function_capacity) {
        size_t new_capacity = debug_info->runtime_function_capacity * 2;
        RuntimeFunctionMapping* new_items = realloc(
            debug_info->runtime_functions,
            sizeof(RuntimeFunctionMapping) * new_capacity);
        if (!new_items) return 0;

        debug_info->runtime_functions = new_items;
        debug_info->runtime_function_capacity = new_capacity;
    }
    return 1;
}

static int debug_info_expand_runtime_locations(DebugInfo* debug_info) {
    if (debug_info->runtime_location_count >= debug_info->runtime_location_capacity) {
        size_t new_capacity = debug_info->runtime_location_capacity * 2;
        RuntimeLocationMapping* new_items = realloc(
            debug_info->runtime_locations,
            sizeof(RuntimeLocationMapping) * new_capacity);
        if (!new_items) return 0;

        debug_info->runtime_locations = new_items;
        debug_info->runtime_location_capacity = new_capacity;
    }
    return 1;
}

void debug_info_add_symbol(DebugInfo* debug_info, const char* name, DebugSymbolType type,
                          const char* type_name, size_t line, size_t column) {
    if (!debug_info || !name) return;
    
    if (!debug_info_expand_symbols(debug_info)) return;
    
    DebugSymbol* symbol = &debug_info->symbols[debug_info->symbol_count];
    symbol->name = string_duplicate(name);
    symbol->type = type;
    symbol->type_name = string_duplicate(type_name);
    symbol->line = line;
    symbol->column = column;
    symbol->address = 0;
    symbol->size = 0;
    symbol->is_register = 0;
    symbol->register_name = NULL;
    symbol->stack_offset = 0;
    
    debug_info->symbol_count++;
}

void debug_info_set_symbol_address(DebugInfo* debug_info, const char* name, size_t address, size_t size) {
    DebugSymbol* symbol = debug_info_find_symbol(debug_info, name);
    if (symbol) {
        symbol->address = address;
        symbol->size = size;
        symbol->is_register = 0;
    }
}

void debug_info_set_symbol_register(DebugInfo* debug_info, const char* name, const char* register_name) {
    DebugSymbol* symbol = debug_info_find_symbol(debug_info, name);
    if (symbol) {
        symbol->is_register = 1;
        free(symbol->register_name);
        symbol->register_name = string_duplicate(register_name);
    }
}

void debug_info_set_symbol_stack_offset(DebugInfo* debug_info, const char* name, int stack_offset) {
    DebugSymbol* symbol = debug_info_find_symbol(debug_info, name);
    if (symbol) {
        symbol->is_register = 0;
        symbol->stack_offset = stack_offset;
    }
}

DebugSymbol* debug_info_find_symbol(DebugInfo* debug_info, const char* name) {
    if (!debug_info || !name) return NULL;
    
    for (size_t i = 0; i < debug_info->symbol_count; i++) {
        if (debug_info->symbols[i].name && strcmp(debug_info->symbols[i].name, name) == 0) {
            return &debug_info->symbols[i];
        }
    }
    return NULL;
}

void debug_info_add_line_mapping(DebugInfo* debug_info, size_t source_line, size_t source_column,
                                size_t assembly_line, const char* filename) {
    if (!debug_info) return;
    
    if (!debug_info_expand_mappings(debug_info)) return;
    
    SourceLineMapping* mapping = &debug_info->line_mappings[debug_info->mapping_count];
    mapping->source_line = source_line;
    mapping->source_column = source_column;
    mapping->assembly_line = assembly_line;
    mapping->filename = string_duplicate(filename);
    
    debug_info->mapping_count++;
}

SourceLineMapping* debug_info_find_line_mapping(DebugInfo* debug_info, size_t assembly_line) {
    if (!debug_info) return NULL;
    
    for (size_t i = 0; i < debug_info->mapping_count; i++) {
        if (debug_info->line_mappings[i].assembly_line == assembly_line) {
            return &debug_info->line_mappings[i];
        }
    }
    return NULL;
}

void debug_info_add_runtime_function_mapping(DebugInfo* debug_info,
                                             const char* function_name,
                                             const char* start_label,
                                             const char* end_label,
                                             const char* filename,
                                             size_t line, size_t column) {
    if (!debug_info || !function_name || !start_label || !end_label) return;

    if (!debug_info_expand_runtime_functions(debug_info)) return;

    RuntimeFunctionMapping* mapping =
        &debug_info->runtime_functions[debug_info->runtime_function_count];
    mapping->function_name = string_duplicate(function_name);
    mapping->start_label = string_duplicate(start_label);
    mapping->end_label = string_duplicate(end_label);
    mapping->filename = string_duplicate(filename);
    mapping->line = line;
    mapping->column = column;
    debug_info->runtime_function_count++;
}

void debug_info_add_runtime_location_mapping(DebugInfo* debug_info,
                                             const char* function_name,
                                             const char* address_label,
                                             const char* filename,
                                             size_t line, size_t column) {
    if (!debug_info || !function_name || !address_label) return;

    if (!debug_info_expand_runtime_locations(debug_info)) return;

    RuntimeLocationMapping* mapping =
        &debug_info->runtime_locations[debug_info->runtime_location_count];
    mapping->function_name = string_duplicate(function_name);
    mapping->address_label = string_duplicate(address_label);
    mapping->filename = string_duplicate(filename);
    mapping->line = line;
    mapping->column = column;
    debug_info->runtime_location_count++;
}

// Debug output generation functions

void debug_info_generate_dwarf(DebugInfo* debug_info, const char* output_filename) {
    if (!debug_info || !output_filename) return;
    
    FILE* file = fopen(output_filename, "w");
    if (!file) return;
    
    fprintf(file, "# DWARF Debug Information\n");
    fprintf(file, "# Generated for %s -> %s\n\n", 
            debug_info->source_filename ? debug_info->source_filename : "unknown",
            debug_info->assembly_filename ? debug_info->assembly_filename : "unknown");
    
    // Generate compilation unit header
    fprintf(file, ".section .debug_info\n");
    fprintf(file, ".long .Ldebug_info_end - .Ldebug_info_start\n");
    fprintf(file, ".Ldebug_info_start:\n");
    fprintf(file, ".short 4  # DWARF version\n");
    fprintf(file, ".long .debug_abbrev  # Abbreviation table offset\n");
    fprintf(file, ".byte 8  # Address size\n\n");
    
    // Generate debug symbols
    for (size_t i = 0; i < debug_info->symbol_count; i++) {
        DebugSymbol* symbol = &debug_info->symbols[i];
        
        fprintf(file, "# Symbol: %s\n", symbol->name);
        fprintf(file, ".uleb128 1  # DW_TAG_variable\n");
        fprintf(file, ".string \"%s\"  # DW_AT_name\n", symbol->name);
        fprintf(file, ".long %zu  # DW_AT_decl_line\n", symbol->line);
        
        if (symbol->is_register) {
            fprintf(file, ".byte 0x50  # DW_OP_reg0 + register offset\n");
        } else {
            fprintf(file, ".byte 0x91  # DW_OP_fbreg\n");
            fprintf(file, ".sleb128 %d  # Stack offset\n", symbol->stack_offset);
        }
        fprintf(file, "\n");
    }
    
    fprintf(file, ".byte 0  # End of compilation unit\n");
    fprintf(file, ".Ldebug_info_end:\n\n");
    
    // Generate line number information
    fprintf(file, ".section .debug_line\n");
    fprintf(file, ".long .Ldebug_line_end - .Ldebug_line_start\n");
    fprintf(file, ".Ldebug_line_start:\n");
    fprintf(file, ".short 4  # DWARF version\n");
    fprintf(file, ".long .Ldebug_line_header_end - .Ldebug_line_header_start\n");
    fprintf(file, ".Ldebug_line_header_start:\n");
    fprintf(file, ".byte 1  # Minimum instruction length\n");
    fprintf(file, ".byte 1  # Default is_stmt\n");
    fprintf(file, ".byte -5  # Line base\n");
    fprintf(file, ".byte 14  # Line range\n");
    fprintf(file, ".byte 13  # Opcode base\n");
    
    // Standard opcode lengths
    for (int i = 1; i < 13; i++) {
        fprintf(file, ".byte %d\n", i == 1 ? 0 : 1);
    }
    
    fprintf(file, ".Ldebug_line_header_end:\n");
    
    // Generate line number program
    for (size_t i = 0; i < debug_info->mapping_count; i++) {
        SourceLineMapping* mapping = &debug_info->line_mappings[i];
        fprintf(file, ".byte 0  # Extended opcode\n");
        fprintf(file, ".uleb128 9  # Length\n");
        fprintf(file, ".byte 2  # DW_LNE_set_address\n");
        fprintf(file, ".quad .L%zu  # Assembly line %zu\n", mapping->assembly_line, mapping->assembly_line);
        fprintf(file, ".byte %zu  # Advance line to %zu\n", 
                mapping->source_line + 5, mapping->source_line);  // +5 for line base
    }
    
    fprintf(file, ".byte 0  # Extended opcode\n");
    fprintf(file, ".uleb128 1  # Length\n");
    fprintf(file, ".byte 1  # DW_LNE_end_sequence\n");
    fprintf(file, ".Ldebug_line_end:\n");
    
    fclose(file);
}

void debug_info_generate_stabs(DebugInfo* debug_info, const char* output_filename) {
    if (!debug_info || !output_filename) return;
    
    FILE* file = fopen(output_filename, "w");
    if (!file) return;
    
    fprintf(file, "# STABS Debug Information\n");
    fprintf(file, "# Generated for %s -> %s\n\n", 
            debug_info->source_filename ? debug_info->source_filename : "unknown",
            debug_info->assembly_filename ? debug_info->assembly_filename : "unknown");
    
    // Source file information
    if (debug_info->source_filename) {
        fprintf(file, ".stabs \"%s\",100,0,0,.Ltext0\n", debug_info->source_filename);
    }
    
    // Generate symbol information
    for (size_t i = 0; i < debug_info->symbol_count; i++) {
        DebugSymbol* symbol = &debug_info->symbols[i];
        
        const char* stab_type;
        switch (symbol->type) {
            case DEBUG_SYMBOL_VARIABLE:
                stab_type = "128";  // N_LSYM (local symbol)
                break;
            case DEBUG_SYMBOL_FUNCTION:
                stab_type = "36";   // N_FUN (function)
                break;
            case DEBUG_SYMBOL_PARAMETER:
                stab_type = "160";  // N_PSYM (parameter)
                break;
            default:
                stab_type = "128";
                break;
        }
        
        if (symbol->is_register) {
            fprintf(file, ".stabs \"%s:r%s\",%s,0,%zu,%s\n",
                    symbol->name,
                    symbol->type_name ? symbol->type_name : "int",
                    stab_type,
                    symbol->line,
                    symbol->register_name ? symbol->register_name : "eax");
        } else {
            fprintf(file, ".stabs \"%s:%s\",%s,0,%zu,%d\n",
                    symbol->name,
                    symbol->type_name ? symbol->type_name : "int",
                    stab_type,
                    symbol->line,
                    symbol->stack_offset);
        }
    }
    
    fclose(file);
}

void debug_info_generate_debug_map(DebugInfo* debug_info, const char* output_filename) {
    if (!debug_info || !output_filename) return;
    
    FILE* file = fopen(output_filename, "w");
    if (!file) return;
    
    fprintf(file, "# Debug Symbol Map\n");
    fprintf(file, "# Source: %s\n", 
            debug_info->source_filename ? debug_info->source_filename : "unknown");
    fprintf(file, "# Assembly: %s\n\n", 
            debug_info->assembly_filename ? debug_info->assembly_filename : "unknown");
    
    fprintf(file, "SYMBOLS:\n");
    for (size_t i = 0; i < debug_info->symbol_count; i++) {
        DebugSymbol* symbol = &debug_info->symbols[i];
        
        fprintf(file, "%s\t%s\t%zu:%zu\t", 
                symbol->name,
                symbol->type_name ? symbol->type_name : "unknown",
                symbol->line, symbol->column);
        
        if (symbol->is_register) {
            fprintf(file, "register:%s\n", 
                    symbol->register_name ? symbol->register_name : "unknown");
        } else {
            fprintf(file, "stack:%d\tsize:%zu\n", symbol->stack_offset, symbol->size);
        }
    }
    
    fprintf(file, "\nLINE_MAPPINGS:\n");
    for (size_t i = 0; i < debug_info->mapping_count; i++) {
        SourceLineMapping* mapping = &debug_info->line_mappings[i];
        fprintf(file, "%zu:%zu\t->\tasm:%zu\t%s\n",
                mapping->source_line, mapping->source_column,
                mapping->assembly_line,
                mapping->filename ? mapping->filename : "");
    }
    
    fclose(file);
}

// Stack trace support implementation

StackTrace* stack_trace_create(void) {
    StackTrace* trace = malloc(sizeof(StackTrace));
    if (!trace) return NULL;
    
    trace->frames = malloc(sizeof(StackFrame) * 16);
    if (!trace->frames) {
        free(trace);
        return NULL;
    }
    
    trace->frame_count = 0;
    trace->frame_capacity = 16;
    
    return trace;
}

void stack_trace_destroy(StackTrace* trace) {
    if (!trace) return;
    
    for (size_t i = 0; i < trace->frame_count; i++) {
        free(trace->frames[i].function_name);
        free(trace->frames[i].filename);
    }
    
    free(trace->frames);
    free(trace);
}

void stack_trace_add_frame(StackTrace* trace, const char* function_name, 
                          const char* filename, size_t line, size_t address) {
    if (!trace) return;
    
    if (trace->frame_count >= trace->frame_capacity) {
        size_t new_capacity = trace->frame_capacity * 2;
        StackFrame* new_frames = realloc(trace->frames, sizeof(StackFrame) * new_capacity);
        if (!new_frames) return;
        
        trace->frames = new_frames;
        trace->frame_capacity = new_capacity;
    }
    
    StackFrame* frame = &trace->frames[trace->frame_count];
    frame->function_name = string_duplicate(function_name);
    frame->filename = string_duplicate(filename);
    frame->line = line;
    frame->address = address;
    
    trace->frame_count++;
}

void stack_trace_print(StackTrace* trace) {
    if (!trace) return;
    
    printf("Stack trace:\n");
    for (size_t i = 0; i < trace->frame_count; i++) {
        StackFrame* frame = &trace->frames[i];
        printf("  #%zu: %s at %s:%zu (0x%zx)\n",
               i,
               frame->function_name ? frame->function_name : "<unknown>",
               frame->filename ? frame->filename : "<unknown>",
               frame->line,
               frame->address);
    }
}

void debug_info_generate_stack_trace_code(DebugInfo* debug_info, const char* output_filename) {
    if (!debug_info || !output_filename) return;
    
    FILE* file = fopen(output_filename, "w");
    if (!file) return;
    
    fprintf(file, "# Stack trace generation code\n");
    fprintf(file, "# This code can be included in the generated assembly to provide runtime stack traces\n\n");
    
    fprintf(file, ".section .text\n");
    fprintf(file, ".global print_stack_trace\n");
    fprintf(file, "print_stack_trace:\n");
    fprintf(file, "    push %%rbp\n");
    fprintf(file, "    mov %%rsp, %%rbp\n");
    fprintf(file, "    \n");
    fprintf(file, "    # Save registers\n");
    fprintf(file, "    push %%rax\n");
    fprintf(file, "    push %%rbx\n");
    fprintf(file, "    push %%rcx\n");
    fprintf(file, "    push %%rdx\n");
    fprintf(file, "    \n");
    fprintf(file, "    # Print stack trace header\n");
    fprintf(file, "    lea stack_trace_header(%%rip), %%rdi\n");
    fprintf(file, "    call printf\n");
    fprintf(file, "    \n");
    fprintf(file, "    # Walk the stack\n");
    fprintf(file, "    mov %%rbp, %%rbx  # Current frame pointer\n");
    fprintf(file, "    xor %%rcx, %%rcx  # Frame counter\n");
    fprintf(file, "    \n");
    fprintf(file, "stack_walk_loop:\n");
    fprintf(file, "    cmp $10, %%rcx    # Limit to 10 frames\n");
    fprintf(file, "    jge stack_walk_done\n");
    fprintf(file, "    \n");
    fprintf(file, "    test %%rbx, %%rbx  # Check if frame pointer is valid\n");
    fprintf(file, "    jz stack_walk_done\n");
    fprintf(file, "    \n");
    fprintf(file, "    # Get return address\n");
    fprintf(file, "    mov 8(%%rbx), %%rax\n");
    fprintf(file, "    \n");
    fprintf(file, "    # Print frame info\n");
    fprintf(file, "    lea stack_frame_format(%%rip), %%rdi\n");
    fprintf(file, "    mov %%rcx, %%rsi\n");
    fprintf(file, "    mov %%rax, %%rdx\n");
    fprintf(file, "    call printf\n");
    fprintf(file, "    \n");
    fprintf(file, "    # Move to next frame\n");
    fprintf(file, "    mov (%%rbx), %%rbx\n");
    fprintf(file, "    inc %%rcx\n");
    fprintf(file, "    jmp stack_walk_loop\n");
    fprintf(file, "    \n");
    fprintf(file, "stack_walk_done:\n");
    fprintf(file, "    # Restore registers\n");
    fprintf(file, "    pop %%rdx\n");
    fprintf(file, "    pop %%rcx\n");
    fprintf(file, "    pop %%rbx\n");
    fprintf(file, "    pop %%rax\n");
    fprintf(file, "    \n");
    fprintf(file, "    pop %%rbp\n");
    fprintf(file, "    ret\n");
    fprintf(file, "    \n");
    fprintf(file, ".section .rodata\n");
    fprintf(file, "stack_trace_header:\n");
    fprintf(file, "    .string \"Stack trace:\\n\"\n");
    fprintf(file, "stack_frame_format:\n");
    fprintf(file, "    .string \"  #%%zu: 0x%%zx\\n\"\n");
    
    fclose(file);
}
