#ifndef BUILTIN_HPP
#define BUILTIN_HPP

#include "expr.hpp"
#include "stmt.hpp"
#include "decl.hpp"
#include "type.hpp"

// Define a set of global names for each builtin functions
constexpr char const* __bind_header  = "fp_bind_header";
constexpr char const* __bind_field   = "fp_bind_field";
constexpr char const* __alias_bind   = "fp_alias_bind";
constexpr char const* __advance      = "fp_advance";
constexpr char const* __get_table    = "fp_get_table";
constexpr char const* __add_flow     = "fp_add_flow";
constexpr char const* __match        = "fp_goto_table";
constexpr char const* __load_field   = "fp_load_field";
constexpr char const* __get_port     = "fp_get_port";

// Build all builtin functions
struct Builtin
{
  Builtin(Symbol_table& syms)
    : syms(syms)
  {
    init_builtins();
  }

  Function_decl* get_builtin_fn(std::string name);

  std::unordered_map<std::string, Function_decl*> get_builtins() { return builtins_; };

private:
  void init_builtins();

  Function_decl* bind_header();
  Function_decl* bind_field();
  Function_decl* alias_bind();
  Function_decl* advance();
  Function_decl* get_table();
  Function_decl* add_flow();
  Function_decl* match();
  Function_decl* load_field();
  Function_decl* get_port();

  Symbol const* get_identifier(std::string);

  Symbol_table& syms;
  std::unordered_map<std::string, Function_decl*> builtins_;
};


// Contains builtin expressions representing the
// flowpath south-bound interface
// i.e. functions which the runtime define and we can link against

// These functions will be linked externally from the C runtime

// Bind the location of an offset
// The runtime function for bind offset has the form
//
// void __bind_offset(Context*, id, offset, length);
//
// Extract declarations become a calls to
//  1 - bind_offset
//  2 - load
// The binding is established, and then the value
// is stored into a variable with the same name and type
// as the field within the extract declaration.
//
// This expression becomes a call to that function.
struct Bind_field : Call_expr
{
  Bind_field(Expr* context, Expr* id, Expr* offset, Expr* length)
    : Call_expr(nullptr, {context, id, offset, length})
  { }
};


// Alias bind of a field
// i.e. extract f1 as f2
//
// This function is called when we want
// to extract a field and give it a name
// which is not its original name.
//
// This causes two binds to occur which
// point to the same byte offset within the
// packet.
//
// void __alias_bind(Context*, id1, id2, offset, length);
//
// This gets generated when rebind extractions are found.
struct Alias_bind : Call_expr
{
  Alias_bind(Expr* context, Expr* id1, Expr* id2, Expr* offset, Expr* length)
    : Call_expr(nullptr, {context, id1, id2, offset, length})
  { }
};


// Bind the location of a header
// The runtime function for this has the form
// The offset of the header is implicitly maintained
// by the current byte within the offset.
//
// void __bind_header(Context*, int id, int length);
//
// The values of entire headers are never immediately
// loaded into memory. This is just so we can keep
// track of the locations header which had been operated
// on.
//
// This becomes a call to that function.
struct Bind_header : Call_expr
{
  Bind_header(Expr* id, Expr* length)
    : Call_expr(nullptr, {id, length})
  { }

  Expr* first;
};


// Loads the value of a field into memory
struct Load : Call_expr
{
  Load(Expr* id)
    : Call_expr(nullptr, {id})
  { }
};


// Tell the dataplane to create a table
// The create_table function from the runtime has
// the form:
//
// void get_table(int id, int key_size, int flow_max, ...)
//
struct Create_table : Call_expr
{

};


// Remove a table
// Why do we need this per se?
struct Delete_table : Call_expr
{

};


struct Add_flow : Call_expr
{

};


// Perform a lookup and execution within a table
//
// Make the assumption that the runtime does the
// gathering operation before dispatching to
// the table.
//
// void __match(Context*, Table*);
struct Match : Call_expr
{
  Match(Expr* context, Expr* table)
    : Call_expr(nullptr, {context, table})
  { }
};


// Advance the current byte in the table
// Causes the current byte offset within the
// context to be incremented by n.
//
// void __advance(Context*, int n)
struct Advance : Call_expr
{
  Advance(Expr* context, Expr* n)
    : Call_expr(nullptr, {n})
  { }
};


// ------------------------------------------------ //
//      Instructions

// Write a drop action
struct Write_drop_stmt : Stmt
{

};


// Write an output action
struct Write_output_stmt : Stmt
{

};


// ------------------------------------------------ //
//      Required Actions


// Drop the packet
struct Drop_stmt : Stmt
{

};


// Output the packet
struct Output_stmt : Stmt
{

};


// Goto a group table
struct Group_expr : Stmt
{

};


#endif
