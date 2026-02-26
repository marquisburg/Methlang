#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stddef.h>
#include <string.h>

typedef enum {
  TYPE_INT8,
  TYPE_INT16,
  TYPE_INT32,
  TYPE_INT64,
  TYPE_UINT8,
  TYPE_UINT16,
  TYPE_UINT32,
  TYPE_UINT64,
  TYPE_FLOAT32,
  TYPE_FLOAT64,
  TYPE_STRING,
  TYPE_POINTER,
  TYPE_ARRAY,
  TYPE_STRUCT,
  TYPE_VOID
} TypeKind;

typedef struct Type {
  TypeKind kind;
  char *name;
  size_t size;
  size_t alignment;
  struct Type *base_type; // For pointers and arrays
  size_t array_size;      // For arrays

  // Struct-specific fields
  char **field_names;        // For structs - field names
  struct Type **field_types; // For structs - field types
  size_t *field_offsets;     // For structs - field memory offsets
  size_t field_count;        // For structs - number of fields
} Type;

typedef enum { SCOPE_GLOBAL, SCOPE_FUNCTION, SCOPE_BLOCK } ScopeType;

typedef struct Scope {
  ScopeType type;
  struct Scope *parent;
  struct Symbol **symbols;
  size_t symbol_count;
  size_t symbol_capacity;
} Scope;

typedef enum {
  SYMBOL_VARIABLE,
  SYMBOL_FUNCTION,
  SYMBOL_STRUCT,
  SYMBOL_PARAMETER
} SymbolKind;

typedef struct Symbol {
  char *name;
  SymbolKind kind;
  Type *type;
  Scope *scope;
  int is_initialized;
  int is_forward_declaration; // For functions that are declared but not defined
  int is_extern;              // For extern declarations (C interop)
  char *link_name;            // Link-time symbol name for extern declarations
  union {
    struct {
      int register_id;
      int memory_offset;
      int is_in_register;
    } variable;
    struct {
      char **parameter_names;
      Type **parameter_types;
      size_t parameter_count;
      Type *return_type;
    } function;
  } data;
} Symbol;

typedef struct SymbolTable {
  Scope *current_scope;
  Scope *global_scope;
} SymbolTable;

// Function declarations
SymbolTable *symbol_table_create(void);
void symbol_table_destroy(SymbolTable *table);
void symbol_table_enter_scope(SymbolTable *table, ScopeType type);
void symbol_table_exit_scope(SymbolTable *table);
int symbol_table_declare(SymbolTable *table, Symbol *symbol);
Symbol *symbol_table_lookup(SymbolTable *table, const char *name);
Symbol *symbol_table_lookup_current_scope(SymbolTable *table, const char *name);
void symbol_table_insert(SymbolTable *table, Symbol *symbol);
int symbol_table_declare_forward(SymbolTable *table, Symbol *symbol);
int symbol_table_resolve_forward_declaration(SymbolTable *table,
                                             Symbol *symbol);
int symbol_table_validate_declaration(SymbolTable *table, Symbol *symbol);
Scope *symbol_table_get_current_scope(SymbolTable *table);
Symbol *symbol_create(const char *name, SymbolKind kind, Type *type);
void symbol_destroy(Symbol *symbol);
Type *type_create(TypeKind kind, const char *name);
void type_destroy(Type *type);

// Struct type creation and manipulation functions
Type *type_create_struct(const char *name, char **field_names,
                         Type **field_types, size_t field_count);
Type *type_get_field_type(Type *struct_type, const char *field_name);
size_t type_get_field_offset(Type *struct_type, const char *field_name);
int type_has_field(Type *struct_type, const char *field_name);

#endif // SYMBOL_TABLE_H
