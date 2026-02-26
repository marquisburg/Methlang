#include "register_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

RegisterAllocator* register_allocator_create(void) {
    RegisterAllocator* allocator = malloc(sizeof(RegisterAllocator));
    if (!allocator) return NULL;
    
    allocator->allocations = NULL;
    allocator->allocation_count = 0;
    allocator->allocation_capacity = 0;
    
    allocator->intervals = NULL;
    allocator->interval_count = 0;
    allocator->interval_capacity = 0;
    
    allocator->active_intervals = NULL;
    allocator->sorted_intervals = NULL;
    allocator->unhandled_count = 0;
    
    allocator->current_position = 0;
    allocator->current_loop_depth = 0;
    allocator->stack_offset = 0;
    
    // Initialize calling convention support
    allocator->calling_convention = NULL;
    allocator->saved_registers = NULL;
    allocator->saved_register_count = 0;
    allocator->function_call_in_progress = 0;
    
    // Initialize register usage tracking
    memset(allocator->register_usage, 0, sizeof(allocator->register_usage));
    
    // Set default calling convention based on platform
#ifdef _WIN32
    register_allocator_set_calling_convention(allocator, CALLING_CONV_MS_X64);
#else
    register_allocator_set_calling_convention(allocator, CALLING_CONV_SYSV);
#endif
    
    return allocator;
}

void register_allocator_destroy(RegisterAllocator* allocator) {
    if (allocator) {
        // Free allocations
        if (allocator->allocations) {
            for (size_t i = 0; i < allocator->allocation_count; i++) {
                free(allocator->allocations[i].variable_name);
            }
        free(allocator->allocations);
        }
        
        // Free intervals and associated data
        register_allocator_destroy_intervals(allocator);
        
        // Free sorted intervals array
        free(allocator->sorted_intervals);
        
        // Free active intervals list
        ActiveInterval* current = allocator->active_intervals;
        while (current) {
            ActiveInterval* next = current->next;
            free(current);
            current = next;
        }
        
        // Free calling convention and saved registers
        if (allocator->calling_convention) {
            register_allocator_destroy_calling_convention_spec(allocator->calling_convention);
        }
        free(allocator->saved_registers);
        
        free(allocator);
    }
}

int register_allocator_allocate_function(RegisterAllocator* allocator, ASTNode* function, SymbolTable* symbol_table) {
    if (!allocator || !function || !symbol_table) {
        return 0;
    }
    
    // Reset allocator state for new function
    allocator->current_position = 0;
    allocator->current_loop_depth = 0;
    allocator->stack_offset = 0;
    memset(allocator->register_usage, 0, sizeof(allocator->register_usage));
    
    // Mark certain registers as reserved (RSP, RBP for stack management)
    allocator->register_usage[REG_RSP] = 1;
    allocator->register_usage[REG_RBP] = 1;
    
    // Clear previous allocations and intervals
    allocator->allocation_count = 0;
    register_allocator_destroy_intervals(allocator);
    allocator->interval_count = 0;
    
    // Clear active intervals
    ActiveInterval* current = allocator->active_intervals;
    while (current) {
        ActiveInterval* next = current->next;
        free(current);
        current = next;
    }
    allocator->active_intervals = NULL;
    
    // Step 1: Analyze live intervals with enhanced tracking
    register_allocator_analyze_live_intervals(allocator, function, symbol_table);
    
    // Step 2: Use linear scan algorithm for register allocation
    return register_allocator_linear_scan(allocator, symbol_table);
}

x86Register register_allocator_get_register(RegisterAllocator* allocator, const char* variable) {
    if (!allocator || !variable) {
        return REG_NONE;
    }
    
    for (size_t i = 0; i < allocator->allocation_count; i++) {
        if (strcmp(allocator->allocations[i].variable_name, variable) == 0) {
            if (allocator->allocations[i].is_in_register) {
                return allocator->allocations[i].register_id;
            }
            break;
        }
    }
    return REG_NONE;
}

int register_allocator_get_memory_offset(RegisterAllocator* allocator, const char* variable) {
    if (!allocator || !variable) {
        return 0;
    }
    
    for (size_t i = 0; i < allocator->allocation_count; i++) {
        if (strcmp(allocator->allocations[i].variable_name, variable) == 0) {
            return allocator->allocations[i].memory_offset;
        }
    }
    return 0;
}

void register_allocator_spill_variable(RegisterAllocator* allocator, const char* variable, SymbolTable* symbol_table) {
    if (!allocator || !variable || !symbol_table) {
        return;
    }
    
    // Find the allocation for this variable
    for (size_t i = 0; i < allocator->allocation_count; i++) {
        if (strcmp(allocator->allocations[i].variable_name, variable) == 0) {
            RegisterAllocation* alloc = &allocator->allocations[i];
            
            if (alloc->is_in_register) {
                // Free the register
                allocator->register_usage[alloc->register_id] = 0;
                
                // Assign memory location
                allocator->stack_offset -= 8; // Assuming 8-byte alignment
                alloc->memory_offset = allocator->stack_offset;
                alloc->spill_location = allocator->stack_offset;
                alloc->is_in_register = 0;
                alloc->register_id = REG_NONE;
                
                // Update symbol table
                Symbol* symbol = symbol_table_lookup(symbol_table, variable);
                if (symbol && symbol->kind == SYMBOL_VARIABLE) {
                    symbol->data.variable.is_in_register = 0;
                    symbol->data.variable.register_id = REG_NONE;
                    symbol->data.variable.memory_offset = alloc->memory_offset;
                }
            }
            break;
        }
    }
}

// Legacy live range analysis implementation (delegates to enhanced version)
void register_allocator_analyze_live_ranges(RegisterAllocator* allocator, ASTNode* node, SymbolTable* symbol_table) {
    // Delegate to enhanced interval analysis
    register_allocator_analyze_live_intervals(allocator, node, symbol_table);
}

LiveRange* register_allocator_find_live_range(RegisterAllocator* allocator, const char* variable) {
    // Delegate to interval finder (LiveRange is typedef'd to LiveInterval)
    return register_allocator_find_interval(allocator, variable);
}

void register_allocator_add_live_range(RegisterAllocator* allocator, const char* variable, Type* type) {
    // Delegate to interval adder
    register_allocator_add_interval(allocator, variable, type);
}

void register_allocator_extend_live_range(RegisterAllocator* allocator, const char* variable) {
    // Delegate to interval extender
    register_allocator_extend_interval(allocator, variable);
}

int register_allocator_assign_registers(RegisterAllocator* allocator, SymbolTable* symbol_table) {
    // Delegate to linear scan algorithm
    return register_allocator_linear_scan(allocator, symbol_table);
}

int register_allocator_has_conflict(RegisterAllocator* allocator, LiveRange* range1, LiveRange* range2) {
    (void)allocator; // Suppress unused parameter warning
    
    if (!range1 || !range2) {
        return 0;
    }
    
    // Two intervals conflict if they overlap
    return !(range1->end_position < range2->start_position || range2->end_position < range1->start_position);
}

x86Register register_allocator_find_free_register(RegisterAllocator* allocator, Type* type, int position) {
    if (!allocator || !type) {
        return REG_NONE;
    }
    
    // Create a temporary interval for this check
    LiveInterval temp_interval;
    temp_interval.type = type;
    temp_interval.start_position = position;
    temp_interval.end_position = position;
    
    return register_allocator_try_allocate_free_register(allocator, &temp_interval);
}

LiveRange* register_allocator_select_spill_candidate(RegisterAllocator* allocator, Type* type) {
    // Delegate to enhanced spill candidate finder
    return register_allocator_find_spill_candidate(allocator, type);
}

// Utility functions
int register_allocator_is_floating_point_type(Type* type) {
    if (!type) {
        return 0;
    }
    
    return (type->kind == TYPE_FLOAT32 || type->kind == TYPE_FLOAT64);
}

const char* register_allocator_register_name(x86Register reg) {
    static const char* register_names[] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
        "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
        "none"
    };
    
    if (reg >= 0 && reg <= REG_NONE) {
        return register_names[reg];
    }
    return "unknown";
}

// Helper function to add allocation (updated for intervals)
int register_allocator_add_allocation(RegisterAllocator* allocator, const char* variable_name, 
                                    x86Register register_id, int memory_offset, int is_in_register, LiveInterval* live_interval) {
    if (!allocator || !variable_name) {
        return 0;
    }
    
    // Expand array if needed
    if (allocator->allocation_count >= allocator->allocation_capacity) {
        size_t new_capacity = allocator->allocation_capacity == 0 ? 8 : allocator->allocation_capacity * 2;
        RegisterAllocation* new_allocations = realloc(allocator->allocations, new_capacity * sizeof(RegisterAllocation));
        if (!new_allocations) {
            return 0; // Memory allocation failed
        }
        allocator->allocations = new_allocations;
        allocator->allocation_capacity = new_capacity;
    }
    
    // Add new allocation
    RegisterAllocation* alloc = &allocator->allocations[allocator->allocation_count];
    alloc->variable_name = malloc(strlen(variable_name) + 1);
    if (!alloc->variable_name) {
        return 0; // Memory allocation failed
    }
    strcpy(alloc->variable_name, variable_name);
    
    alloc->register_id = register_id;
    alloc->memory_offset = memory_offset;
    alloc->spill_location = memory_offset;
    alloc->is_in_register = is_in_register;
    alloc->live_interval = live_interval;
    
    allocator->allocation_count++;
    return 1;
}

// Helper functions for interval management
void register_allocator_destroy_use_positions(UsePosition* use_pos) {
    while (use_pos) {
        UsePosition* next = use_pos->next;
        free(use_pos);
        use_pos = next;
    }
}

void register_allocator_destroy_intervals(RegisterAllocator* allocator) {
    if (!allocator || !allocator->intervals) {
        return;
    }
    
    for (size_t i = 0; i < allocator->interval_count; i++) {
        free(allocator->intervals[i].variable_name);
        register_allocator_destroy_use_positions(allocator->intervals[i].use_positions);
    }
    free(allocator->intervals);
    allocator->intervals = NULL;
    allocator->interval_capacity = 0;
}

// Enhanced live interval analysis
void register_allocator_analyze_live_intervals(RegisterAllocator* allocator, ASTNode* node, SymbolTable* symbol_table) {
    if (!allocator || !node || !symbol_table) {
        return;
    }
    
    allocator->current_position++;
    
    // Detect loop structures for spill cost calculation
    if (node->type == AST_WHILE_STATEMENT) {
        allocator->current_loop_depth++;
    }
    
    // Handle different node types for live interval analysis
    switch (node->type) {
        case AST_VAR_DECLARATION: {
            VarDeclaration* var = (VarDeclaration*)node->data;
            if (var && var->name) {
                Symbol* symbol = symbol_table_lookup(symbol_table, var->name);
                if (symbol && symbol->kind == SYMBOL_VARIABLE) {
                    register_allocator_add_interval(allocator, var->name, symbol->type);
                    LiveInterval* interval = register_allocator_find_interval(allocator, var->name);
                    if (interval) {
                        register_allocator_add_use_position(interval, allocator->current_position, 1); // Definition
                    }
                }
                
                // If there's an initializer, analyze it
                if (var->initializer) {
                    register_allocator_analyze_live_intervals(allocator, var->initializer, symbol_table);
                }
            }
            break;
        }
        
        case AST_ASSIGNMENT: {
            Assignment* assign = (Assignment*)node->data;
            if (assign && assign->variable_name) {
                // Variable is being assigned to - add definition use
                register_allocator_extend_interval(allocator, assign->variable_name);
                LiveInterval* interval = register_allocator_find_interval(allocator, assign->variable_name);
                if (interval) {
                    register_allocator_add_use_position(interval, allocator->current_position, 1); // Definition
                }
                
                // Analyze the value expression
                if (assign->value) {
                    register_allocator_analyze_live_intervals(allocator, assign->value, symbol_table);
                }
            }
            break;
        }
        
        case AST_IDENTIFIER: {
            Identifier* id = (Identifier*)node->data;
            if (id && id->name) {
                // Variable is being used - add use position
                register_allocator_extend_interval(allocator, id->name);
                LiveInterval* interval = register_allocator_find_interval(allocator, id->name);
                if (interval) {
                    register_allocator_add_use_position(interval, allocator->current_position, 0); // Use
                }
            }
            break;
        }
        
        case AST_FUNCTION_CALL: {
            CallExpression* call = (CallExpression*)node->data;
            if (call) {
                // Analyze function arguments
                for (size_t i = 0; i < call->argument_count; i++) {
                    if (call->arguments[i]) {
                        register_allocator_analyze_live_intervals(allocator, call->arguments[i], symbol_table);
                    }
                }
            }
            break;
        }
        
        case AST_BINARY_EXPRESSION: {
            BinaryExpression* bin = (BinaryExpression*)node->data;
            if (bin) {
                if (bin->left) {
                    register_allocator_analyze_live_intervals(allocator, bin->left, symbol_table);
                }
                if (bin->right) {
                    register_allocator_analyze_live_intervals(allocator, bin->right, symbol_table);
                }
            }
            break;
        }
        
        case AST_UNARY_EXPRESSION: {
            UnaryExpression* unary = (UnaryExpression*)node->data;
            if (unary && unary->operand) {
                register_allocator_analyze_live_intervals(allocator, unary->operand, symbol_table);
            }
            break;
        }
        
        default:
            break;
    }
    
    // Recursively analyze child nodes
    for (size_t i = 0; i < node->child_count; i++) {
        if (node->children[i]) {
            register_allocator_analyze_live_intervals(allocator, node->children[i], symbol_table);
        }
    }
    
    // Exit loop structure
    if (node->type == AST_WHILE_STATEMENT) {
        allocator->current_loop_depth--;
    }
}

LiveInterval* register_allocator_find_interval(RegisterAllocator* allocator, const char* variable) {
    if (!allocator || !variable) {
        return NULL;
    }
    
    for (size_t i = 0; i < allocator->interval_count; i++) {
        if (strcmp(allocator->intervals[i].variable_name, variable) == 0) {
            return &allocator->intervals[i];
        }
    }
    return NULL;
}

void register_allocator_add_interval(RegisterAllocator* allocator, const char* variable, Type* type) {
    if (!allocator || !variable) {
        return;
    }
    
    // Check if interval already exists
    LiveInterval* existing = register_allocator_find_interval(allocator, variable);
    if (existing) {
        return; // Already exists
    }
    
    // Expand array if needed
    if (allocator->interval_count >= allocator->interval_capacity) {
        size_t new_capacity = allocator->interval_capacity == 0 ? 8 : allocator->interval_capacity * 2;
        LiveInterval* new_intervals = realloc(allocator->intervals, new_capacity * sizeof(LiveInterval));
        if (!new_intervals) {
            return; // Memory allocation failed
        }
        allocator->intervals = new_intervals;
        allocator->interval_capacity = new_capacity;
    }
    
    // Add new interval
    LiveInterval* interval = &allocator->intervals[allocator->interval_count];
    interval->variable_name = malloc(strlen(variable) + 1);
    if (!interval->variable_name) {
        return; // Memory allocation failed
    }
    strcpy(interval->variable_name, variable);
    
    interval->start_position = allocator->current_position;
    interval->end_position = allocator->current_position;
    interval->loop_depth = allocator->current_loop_depth;
    interval->spill_cost = 0.0f; // Will be calculated later
    interval->type = type;
    interval->use_positions = NULL;
    interval->assigned_register = REG_NONE;
    interval->is_split = 0;
    interval->parent = NULL;
    
    allocator->interval_count++;
}

void register_allocator_add_use_position(LiveInterval* interval, int position, int is_def) {
    if (!interval) {
        return;
    }
    
    UsePosition* use_pos = malloc(sizeof(UsePosition));
    if (!use_pos) {
        return;
    }
    
    use_pos->position = position;
    use_pos->is_def = is_def;
    use_pos->next = interval->use_positions;
    interval->use_positions = use_pos;
}

void register_allocator_extend_interval(RegisterAllocator* allocator, const char* variable) {
    if (!allocator || !variable) {
        return;
    }
    
    LiveInterval* interval = register_allocator_find_interval(allocator, variable);
    if (interval) {
        interval->end_position = allocator->current_position;
        // Spill cost will be calculated after all use positions are collected
    }
}

// Linear scan register allocation algorithm
int register_allocator_linear_scan(RegisterAllocator* allocator, SymbolTable* symbol_table) {
    if (!allocator || !symbol_table) {
        return 0;
    }
    
    // Step 1: Calculate spill costs for all intervals
    for (size_t i = 0; i < allocator->interval_count; i++) {
        allocator->intervals[i].spill_cost = register_allocator_calculate_spill_cost(&allocator->intervals[i]);
    }
    
    // Step 2: Sort intervals by start position
    register_allocator_sort_intervals(allocator);
    
    // Step 3: Process each interval in order
    for (size_t i = 0; i < allocator->interval_count; i++) {
        LiveInterval* current = &allocator->intervals[i];
        
        // Remove expired intervals from active list
        register_allocator_expire_old_intervals(allocator, current->start_position);
        
        // Try to allocate a free register
        x86Register reg = register_allocator_try_allocate_free_register(allocator, current);
        
        if (reg != REG_NONE) {
            // Successfully allocated a register
            current->assigned_register = reg;
            allocator->register_usage[reg] = 1;
            register_allocator_add_to_active(allocator, current);
            
            // Create allocation record
            register_allocator_add_allocation(allocator, current->variable_name, reg, 0, 1, current);
            
            // Update symbol table
            Symbol* symbol = symbol_table_lookup(symbol_table, current->variable_name);
            if (symbol && symbol->kind == SYMBOL_VARIABLE) {
                symbol->data.variable.is_in_register = 1;
                symbol->data.variable.register_id = reg;
                symbol->data.variable.memory_offset = 0;
            }
        } else {
            // No free register - need to spill or allocate blocked register
            register_allocator_allocate_blocked_register(allocator, current, symbol_table);
        }
    }
    
    return 1; // Success
}

void register_allocator_sort_intervals(RegisterAllocator* allocator) {
    if (!allocator || allocator->interval_count <= 1) {
        return;
    }
    
    // Allocate sorted intervals array if needed
    if (!allocator->sorted_intervals) {
        allocator->sorted_intervals = malloc(allocator->interval_count * sizeof(LiveInterval*));
        if (!allocator->sorted_intervals) {
            return;
        }
    }
    
    // Initialize pointers to intervals
    for (size_t i = 0; i < allocator->interval_count; i++) {
        allocator->sorted_intervals[i] = &allocator->intervals[i];
    }
    
    // Simple bubble sort by start position (could be optimized)
    for (size_t i = 0; i < allocator->interval_count; i++) {
        for (size_t j = 0; j < allocator->interval_count - 1 - i; j++) {
            if (allocator->sorted_intervals[j]->start_position > 
                allocator->sorted_intervals[j + 1]->start_position) {
                LiveInterval* temp = allocator->sorted_intervals[j];
                allocator->sorted_intervals[j] = allocator->sorted_intervals[j + 1];
                allocator->sorted_intervals[j + 1] = temp;
            }
        }
    }
}

void register_allocator_expire_old_intervals(RegisterAllocator* allocator, int current_position) {
    if (!allocator) {
        return;
    }
    
    ActiveInterval* prev = NULL;
    ActiveInterval* current = allocator->active_intervals;
    
    while (current) {
        if (current->interval->end_position < current_position) {
            // This interval has expired - free its register
            if (current->interval->assigned_register != REG_NONE) {
                allocator->register_usage[current->interval->assigned_register] = 0;
            }
            
            // Remove from active list
            if (prev) {
                prev->next = current->next;
            } else {
                allocator->active_intervals = current->next;
            }
            
            ActiveInterval* to_free = current;
            current = current->next;
            free(to_free);
        } else {
            prev = current;
            current = current->next;
        }
    }
}

x86Register register_allocator_try_allocate_free_register(RegisterAllocator* allocator, LiveInterval* interval) {
    if (!allocator || !interval) {
        return REG_NONE;
    }
    
    // Determine register set based on type
    x86Register start_reg, end_reg;
    
    if (register_allocator_is_floating_point_type(interval->type)) {
        start_reg = REG_XMM0;
        end_reg = REG_XMM15;
    } else {
        start_reg = REG_RAX;
        end_reg = REG_R15;
    }
    
    // Look for a free register
    for (x86Register reg = start_reg; reg <= end_reg; reg++) {
        // Skip reserved registers
        if (reg == REG_RSP || reg == REG_RBP) {
            continue;
        }
        
        if (!allocator->register_usage[reg]) {
            return reg;
        }
    }
    
    return REG_NONE; // No free register found
}

void register_allocator_allocate_blocked_register(RegisterAllocator* allocator, LiveInterval* interval, SymbolTable* symbol_table) {
    if (!allocator || !interval || !symbol_table) {
        return;
    }
    
    // Find the best spill candidate among active intervals
    LiveInterval* spill_candidate = register_allocator_find_spill_candidate(allocator, interval->type);
    
    if (spill_candidate && spill_candidate->spill_cost > interval->spill_cost) {
        // Spill the candidate and take its register
        x86Register reg = spill_candidate->assigned_register;
        
        // Spill the candidate
        register_allocator_spill_interval(allocator, spill_candidate, symbol_table);
        register_allocator_remove_from_active(allocator, spill_candidate);
        
        // Assign register to current interval
        interval->assigned_register = reg;
        allocator->register_usage[reg] = 1;
        register_allocator_add_to_active(allocator, interval);
        
        // Create allocation record
        register_allocator_add_allocation(allocator, interval->variable_name, reg, 0, 1, interval);
        
        // Update symbol table
        Symbol* symbol = symbol_table_lookup(symbol_table, interval->variable_name);
        if (symbol && symbol->kind == SYMBOL_VARIABLE) {
            symbol->data.variable.is_in_register = 1;
            symbol->data.variable.register_id = reg;
            symbol->data.variable.memory_offset = 0;
        }
    } else {
        // Spill current interval to memory
        register_allocator_spill_interval(allocator, interval, symbol_table);
    }
}

// Active interval list management
void register_allocator_add_to_active(RegisterAllocator* allocator, LiveInterval* interval) {
    if (!allocator || !interval) {
        return;
    }
    
    ActiveInterval* active = malloc(sizeof(ActiveInterval));
    if (!active) {
        return;
    }
    
    active->interval = interval;
    active->next = allocator->active_intervals;
    allocator->active_intervals = active;
}

void register_allocator_remove_from_active(RegisterAllocator* allocator, LiveInterval* interval) {
    if (!allocator || !interval) {
        return;
    }
    
    ActiveInterval* prev = NULL;
    ActiveInterval* current = allocator->active_intervals;
    
    while (current) {
        if (current->interval == interval) {
            if (prev) {
                prev->next = current->next;
            } else {
                allocator->active_intervals = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

LiveInterval* register_allocator_find_spill_candidate(RegisterAllocator* allocator, Type* type) {
    if (!allocator || !type) {
        return NULL;
    }
    
    LiveInterval* best_candidate = NULL;
    float highest_spill_cost = -1.0f;
    
    ActiveInterval* current = allocator->active_intervals;
    while (current) {
        if (register_allocator_is_floating_point_type(current->interval->type) == 
            register_allocator_is_floating_point_type(type)) {
            
            if (current->interval->spill_cost > highest_spill_cost) {
                highest_spill_cost = current->interval->spill_cost;
                best_candidate = current->interval;
            }
        }
        current = current->next;
    }
    
    return best_candidate;
}

// Interval splitting and spilling
void register_allocator_spill_interval(RegisterAllocator* allocator, LiveInterval* interval, SymbolTable* symbol_table) {
    if (!allocator || !interval || !symbol_table) {
        return;
    }
    
    // Assign memory location
    allocator->stack_offset -= 8; // Assuming 8-byte alignment
    
    // Create allocation record for memory
    register_allocator_add_allocation(allocator, interval->variable_name, REG_NONE, 
                                    allocator->stack_offset, 0, interval);
    
    // Update symbol table
    Symbol* symbol = symbol_table_lookup(symbol_table, interval->variable_name);
    if (symbol && symbol->kind == SYMBOL_VARIABLE) {
        symbol->data.variable.is_in_register = 0;
        symbol->data.variable.register_id = REG_NONE;
        symbol->data.variable.memory_offset = allocator->stack_offset;
    }
    
    // Free register if it was assigned
    if (interval->assigned_register != REG_NONE) {
        allocator->register_usage[interval->assigned_register] = 0;
        interval->assigned_register = REG_NONE;
    }
}

float register_allocator_calculate_spill_cost(LiveInterval* interval) {
    if (!interval) {
        return 0.0f;
    }
    
    float cost = 1.0f; // Base cost
    int use_count = 0;
    
    // Count use positions and weight by loop depth
    UsePosition* use_pos = interval->use_positions;
    while (use_pos) {
        use_count++;
        use_pos = use_pos->next;
    }
    
    // Weight by loop depth - variables in deeper loops are costlier to spill
    if (interval->loop_depth > 0) {
        for (int i = 0; i < interval->loop_depth; i++) {
            cost *= 10.0f;
        }
    }
    
    // Weight by use count
    cost *= (use_count + 1);
    
    // Normalize by interval length to prefer spilling shorter intervals
    int interval_length = interval->end_position - interval->start_position + 1;
    if (interval_length > 0) {
        cost = cost / (float)interval_length;
    }
    
    return cost;
}

// Calling convention implementation
void register_allocator_set_calling_convention(RegisterAllocator* allocator, CallingConvention convention) {
    if (!allocator) {
        return;
    }
    
    // Free existing calling convention
    if (allocator->calling_convention) {
        register_allocator_destroy_calling_convention_spec(allocator->calling_convention);
    }
    
    // Set new calling convention
    allocator->calling_convention = register_allocator_get_calling_convention_spec(convention);
}

CallingConventionSpec* register_allocator_get_calling_convention_spec(CallingConvention convention) {
    CallingConventionSpec* spec = malloc(sizeof(CallingConventionSpec));
    if (!spec) {
        return NULL;
    }
    
    spec->convention = convention;
    
    if (convention == CALLING_CONV_SYSV) {
        // System V ABI (Linux, Unix)
        
        // Integer parameter registers: RDI, RSI, RDX, RCX, R8, R9
        spec->int_param_count = 6;
        spec->int_param_registers = malloc(spec->int_param_count * sizeof(x86Register));
        spec->int_param_registers[0] = REG_RDI;
        spec->int_param_registers[1] = REG_RSI;
        spec->int_param_registers[2] = REG_RDX;
        spec->int_param_registers[3] = REG_RCX;
        spec->int_param_registers[4] = REG_R8;
        spec->int_param_registers[5] = REG_R9;
        
        // Float parameter registers: XMM0-XMM7
        spec->float_param_count = 8;
        spec->float_param_registers = malloc(spec->float_param_count * sizeof(x86Register));
        for (int i = 0; i < 8; i++) {
            spec->float_param_registers[i] = REG_XMM0 + i;
        }
        
        // Return registers
        spec->int_return_register = REG_RAX;
        spec->float_return_register = REG_XMM0;
        
        // Caller-saved registers (volatile): RAX, RCX, RDX, RSI, RDI, R8-R11, XMM0-XMM15
        spec->caller_saved_count = 19;
        spec->caller_saved_registers = malloc(spec->caller_saved_count * sizeof(x86Register));
        spec->caller_saved_registers[0] = REG_RAX;
        spec->caller_saved_registers[1] = REG_RCX;
        spec->caller_saved_registers[2] = REG_RDX;
        spec->caller_saved_registers[3] = REG_RSI;
        spec->caller_saved_registers[4] = REG_RDI;
        spec->caller_saved_registers[5] = REG_R8;
        spec->caller_saved_registers[6] = REG_R9;
        spec->caller_saved_registers[7] = REG_R10;
        spec->caller_saved_registers[8] = REG_R11;
        // XMM0-XMM15 are all caller-saved in System V
        for (int i = 0; i < 10; i++) {
            spec->caller_saved_registers[9 + i] = REG_XMM0 + i;
        }
        
        // Callee-saved registers (non-volatile): RBX, RBP, R12-R15
        spec->callee_saved_count = 6;
        spec->callee_saved_registers = malloc(spec->callee_saved_count * sizeof(x86Register));
        spec->callee_saved_registers[0] = REG_RBX;
        spec->callee_saved_registers[1] = REG_RBP;
        spec->callee_saved_registers[2] = REG_R12;
        spec->callee_saved_registers[3] = REG_R13;
        spec->callee_saved_registers[4] = REG_R14;
        spec->callee_saved_registers[5] = REG_R15;
        
        spec->stack_alignment = 16;
        spec->shadow_space_size = 0;
        
    } else if (convention == CALLING_CONV_MS_X64) {
        // Microsoft x64 calling convention (Windows)
        
        // Integer parameter registers: RCX, RDX, R8, R9
        spec->int_param_count = 4;
        spec->int_param_registers = malloc(spec->int_param_count * sizeof(x86Register));
        spec->int_param_registers[0] = REG_RCX;
        spec->int_param_registers[1] = REG_RDX;
        spec->int_param_registers[2] = REG_R8;
        spec->int_param_registers[3] = REG_R9;
        
        // Float parameter registers: XMM0-XMM3
        spec->float_param_count = 4;
        spec->float_param_registers = malloc(spec->float_param_count * sizeof(x86Register));
        spec->float_param_registers[0] = REG_XMM0;
        spec->float_param_registers[1] = REG_XMM1;
        spec->float_param_registers[2] = REG_XMM2;
        spec->float_param_registers[3] = REG_XMM3;
        
        // Return registers
        spec->int_return_register = REG_RAX;
        spec->float_return_register = REG_XMM0;
        
        // Caller-saved registers (volatile): RAX, RCX, RDX, R8-R11, XMM0-XMM5
        spec->caller_saved_count = 13;
        spec->caller_saved_registers = malloc(spec->caller_saved_count * sizeof(x86Register));
        spec->caller_saved_registers[0] = REG_RAX;
        spec->caller_saved_registers[1] = REG_RCX;
        spec->caller_saved_registers[2] = REG_RDX;
        spec->caller_saved_registers[3] = REG_R8;
        spec->caller_saved_registers[4] = REG_R9;
        spec->caller_saved_registers[5] = REG_R10;
        spec->caller_saved_registers[6] = REG_R11;
        // XMM0-XMM5 are caller-saved, XMM6-XMM15 are callee-saved
        for (int i = 0; i < 6; i++) {
            spec->caller_saved_registers[7 + i] = REG_XMM0 + i;
        }
        
        // Callee-saved registers (non-volatile): RBX, RBP, RDI, RSI, R12-R15, XMM6-XMM15
        spec->callee_saved_count = 16;
        spec->callee_saved_registers = malloc(spec->callee_saved_count * sizeof(x86Register));
        spec->callee_saved_registers[0] = REG_RBX;
        spec->callee_saved_registers[1] = REG_RBP;
        spec->callee_saved_registers[2] = REG_RDI;
        spec->callee_saved_registers[3] = REG_RSI;
        spec->callee_saved_registers[4] = REG_R12;
        spec->callee_saved_registers[5] = REG_R13;
        spec->callee_saved_registers[6] = REG_R14;
        spec->callee_saved_registers[7] = REG_R15;
        // XMM6-XMM15 are callee-saved in MS x64
        for (int i = 0; i < 8; i++) {
            spec->callee_saved_registers[8 + i] = REG_XMM6 + i;
        }
        
        spec->stack_alignment = 16;
        spec->shadow_space_size = 32; // 4 * 8 bytes shadow space
        
    } else {
        // Unknown convention
        free(spec);
        return NULL;
    }
    
    return spec;
}

void register_allocator_destroy_calling_convention_spec(CallingConventionSpec* spec) {
    if (spec) {
        free(spec->int_param_registers);
        free(spec->float_param_registers);
        free(spec->caller_saved_registers);
        free(spec->callee_saved_registers);
        free(spec);
    }
}

RegisterClass register_allocator_get_register_class(CallingConventionSpec* spec, x86Register reg) {
    if (!spec || reg == REG_NONE) {
        return REG_CLASS_RESERVED;
    }
    
    // Check if reserved
    if (reg == REG_RSP || reg == REG_RBP) {
        return REG_CLASS_RESERVED;
    }
    
    // Check if parameter register
    for (size_t i = 0; i < spec->int_param_count; i++) {
        if (spec->int_param_registers[i] == reg) {
            return REG_CLASS_PARAMETER;
        }
    }
    for (size_t i = 0; i < spec->float_param_count; i++) {
        if (spec->float_param_registers[i] == reg) {
            return REG_CLASS_PARAMETER;
        }
    }
    
    // Check if return register
    if (reg == spec->int_return_register || reg == spec->float_return_register) {
        return REG_CLASS_RETURN;
    }
    
    // Check if caller-saved
    for (size_t i = 0; i < spec->caller_saved_count; i++) {
        if (spec->caller_saved_registers[i] == reg) {
            return REG_CLASS_CALLER_SAVED;
        }
    }
    
    // Check if callee-saved
    for (size_t i = 0; i < spec->callee_saved_count; i++) {
        if (spec->callee_saved_registers[i] == reg) {
            return REG_CLASS_CALLEE_SAVED;
        }
    }
    
    return REG_CLASS_RESERVED; // Default to reserved if not found
}

int register_allocator_is_caller_saved(CallingConventionSpec* spec, x86Register reg) {
    if (!spec) {
        return 0;
    }
    
    for (size_t i = 0; i < spec->caller_saved_count; i++) {
        if (spec->caller_saved_registers[i] == reg) {
            return 1;
        }
    }
    return 0;
}

int register_allocator_is_callee_saved(CallingConventionSpec* spec, x86Register reg) {
    if (!spec) {
        return 0;
    }
    
    for (size_t i = 0; i < spec->callee_saved_count; i++) {
        if (spec->callee_saved_registers[i] == reg) {
            return 1;
        }
    }
    return 0;
}

// Function call support
void register_allocator_prepare_function_call(RegisterAllocator* allocator, const char* function_name) {
    if (!allocator || !function_name) {
        return;
    }
    
    allocator->function_call_in_progress = 1;
    
    // Save caller-saved registers that are currently in use
    register_allocator_save_caller_saved_registers(allocator);
}

void register_allocator_complete_function_call(RegisterAllocator* allocator) {
    if (!allocator) {
        return;
    }
    
    // Restore caller-saved registers
    register_allocator_restore_caller_saved_registers(allocator);
    
    allocator->function_call_in_progress = 0;
}

x86Register register_allocator_get_parameter_register(RegisterAllocator* allocator, int param_index, Type* param_type) {
    if (!allocator || !allocator->calling_convention || !param_type) {
        return REG_NONE;
    }
    
    CallingConventionSpec* spec = allocator->calling_convention;
    if (param_index < 0) {
        return REG_NONE;
    }
    
    if (spec->convention == CALLING_CONV_MS_X64) {
        // Win64 uses shared argument slots across integer and floating classes.
        size_t slot_count = spec->int_param_count;
        if (spec->float_param_count < slot_count) {
            slot_count = spec->float_param_count;
        }
        if ((size_t)param_index >= slot_count) {
            return REG_NONE;
        }
        if (register_allocator_is_floating_point_type(param_type)) {
            return spec->float_param_registers[param_index];
        }
        return spec->int_param_registers[param_index];
    }
    
    if (register_allocator_is_floating_point_type(param_type)) {
        // Floating point parameter
        if (param_index >= 0 && (size_t)param_index < spec->float_param_count) {
            return spec->float_param_registers[param_index];
        }
    } else {
        // Integer parameter
        if (param_index >= 0 && (size_t)param_index < spec->int_param_count) {
            return spec->int_param_registers[param_index];
        }
    }
    
    return REG_NONE; // Parameter passed on stack
}

x86Register register_allocator_get_return_register(RegisterAllocator* allocator, Type* return_type) {
    if (!allocator || !allocator->calling_convention || !return_type) {
        return REG_NONE;
    }
    
    CallingConventionSpec* spec = allocator->calling_convention;
    
    if (register_allocator_is_floating_point_type(return_type)) {
        return spec->float_return_register;
    } else {
        return spec->int_return_register;
    }
}

void register_allocator_save_caller_saved_registers(RegisterAllocator* allocator) {
    if (!allocator || !allocator->calling_convention) {
        return;
    }
    
    CallingConventionSpec* spec = allocator->calling_convention;
    
    // Clear existing saved registers
    free(allocator->saved_registers);
    allocator->saved_registers = NULL;
    allocator->saved_register_count = 0;
    
    // Count caller-saved registers currently in use
    size_t count = 0;
    for (size_t i = 0; i < spec->caller_saved_count; i++) {
        x86Register reg = spec->caller_saved_registers[i];
        if (allocator->register_usage[reg]) {
            count++;
        }
    }
    
    if (count == 0) {
        return;
    }
    
    // Allocate array for saved registers
    allocator->saved_registers = malloc(count * sizeof(int));
    if (!allocator->saved_registers) {
        return;
    }
    
    // Save registers that need to be preserved
    size_t saved_count = 0;
    for (size_t i = 0; i < spec->caller_saved_count; i++) {
        x86Register reg = spec->caller_saved_registers[i];
        if (allocator->register_usage[reg]) {
            allocator->saved_registers[saved_count] = reg;
            saved_count++;
        }
    }
    
    allocator->saved_register_count = saved_count;
}

void register_allocator_restore_caller_saved_registers(RegisterAllocator* allocator) {
    if (!allocator || !allocator->saved_registers) {
        return;
    }
    
    // Restore register usage state
    for (size_t i = 0; i < allocator->saved_register_count; i++) {
        x86Register reg = (x86Register)allocator->saved_registers[i];
        allocator->register_usage[reg] = 1;
    }
    
    // Clear saved registers
    free(allocator->saved_registers);
    allocator->saved_registers = NULL;
    allocator->saved_register_count = 0;
}

void register_allocator_mark_callee_saved_registers(RegisterAllocator* allocator) {
    if (!allocator || !allocator->calling_convention) {
        return;
    }
    
    CallingConventionSpec* spec = allocator->calling_convention;
    
    // Mark callee-saved registers as preferred for long-lived variables
    // This is a hint to the register allocator to prefer these registers
    // for variables that live across function calls
    
    (void)spec; // Suppress unused parameter warning for now
    // This functionality would be integrated with the linear scan algorithm
    // to prefer callee-saved registers for variables with longer live ranges
}

LiveInterval* register_allocator_split_interval(LiveInterval* interval, int position) {
    if (!interval || position <= interval->start_position || position >= interval->end_position) {
        return NULL;
    }
    
    // For now, return NULL - full interval splitting is complex and would be implemented
    // in a more advanced version. This is a placeholder for the interface.
    (void)interval; // Suppress unused parameter warning
    (void)position; // Suppress unused parameter warning
    return NULL;
}
