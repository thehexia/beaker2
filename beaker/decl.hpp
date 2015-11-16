// Copyright (c) 2015 Andrew Sutton
// All rights reserved

#ifndef BEAKER_DECL_HPP
#define BEAKER_DECL_HPP

#include "prelude.hpp"
#include "specifier.hpp"


// Represents the declaration of a named entity.
// Every declaration has a name and a type. Note that
// user-defined type declarations (e.g., modulues)
// have nullptr type. We use this to indicate a higher
// order type.
struct Decl
{
  struct Visitor;
  struct Mutator;

  Decl(Symbol const* s, Type const* t)
    : spec_(no_spec), name_(s), type_(t), cxt_(nullptr)
  { }

  Decl(Specifier spec, Symbol const* s, Type const* t)
    : spec_(spec), name_(s), type_(t), cxt_(nullptr)
  { }

  virtual ~Decl() { }

  virtual void accept(Visitor&) const = 0;
  virtual void accept(Mutator&) = 0;

  // Declaration specifiers
  Specifier specifiers() const { return spec_; }
  bool      is_foreign() const { return spec_ & foreign_spec; }

  Symbol const* name() const { return name_; }
  Type const*   type() const { return type_; }

  Decl const*   context() const { return cxt_; }

  Specifier     spec_;
  Symbol const* name_;
  Type const*   type_;
  Decl const*   cxt_;
};


// The read-only declaration visitor.
struct Decl::Visitor
{
  virtual void visit(Variable_decl const*) = 0;
  virtual void visit(Function_decl const*) = 0;
  virtual void visit(Parameter_decl const*) = 0;
  virtual void visit(Record_decl const*) = 0;
  virtual void visit(Field_decl const*) = 0;
  virtual void visit(Module_decl const*) = 0;

  // network declarations
  virtual void visit(Layout_decl const*) = 0;
  virtual void visit(Decode_decl const*) = 0;
  virtual void visit(Table_decl const*) = 0;
  virtual void visit(Flow_decl const*) = 0;
  virtual void visit(Port_decl const*) = 0;
  virtual void visit(Extracts_decl const*) = 0;
  virtual void visit(Rebind_decl const*) = 0;
};


// The read/write declaration visitor.
struct Decl::Mutator
{
  virtual void visit(Variable_decl*) = 0;
  virtual void visit(Function_decl*) = 0;
  virtual void visit(Parameter_decl*) = 0;
  virtual void visit(Record_decl*) = 0;
  virtual void visit(Field_decl*) = 0;
  virtual void visit(Module_decl*) = 0;

  // network declarations
  virtual void visit(Layout_decl*) = 0;
  virtual void visit(Decode_decl*) = 0;
  virtual void visit(Table_decl*) = 0;
  virtual void visit(Flow_decl*) = 0;
  virtual void visit(Port_decl*) = 0;
  virtual void visit(Extracts_decl*) = 0;
  virtual void visit(Rebind_decl*) = 0;
};


// Represents variable declarations.
struct Variable_decl : Decl
{
  Variable_decl(Symbol const* n, Type const* t, Expr* e)
    : Decl(n, t), init_(e)
  { }

  Variable_decl(Specifier spec, Symbol const* n, Type const* t, Expr* e)
    : Decl(spec, n, t), init_(e)
  { }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

  Expr const* init() const { return init_; }
  Expr*       init()       { return init_; }

  Expr* init_;
};


// Represents function declarations.
struct Function_decl : Decl
{
  Function_decl(Symbol const* n, Type const* t, Decl_seq const& p, Stmt* b)
    : Decl(n, t), parms_(p), body_(b)
  { }

  Function_decl(Specifier spec, Symbol const* n, Type const* t, Decl_seq const& p, Stmt* b)
    : Decl(spec, n, t), parms_(p), body_(b)
  { }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

  Decl_seq const&      parameters() const { return parms_; }

  Function_type const* type() const;
  Type const*          return_type() const;

  Stmt const* body() const { return body_; }
  Stmt*       body()       { return body_; }

  Decl_seq parms_;
  Stmt*    body_;
};


// Represents parameter declarations.
struct Parameter_decl : Decl
{
  using Decl::Decl;

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }
};


// Declares a user-defined record type.
struct Record_decl : Decl
{
  Record_decl(Symbol const* n, Decl_seq const& f)
    : Decl(n, nullptr), fields_(f)
  { }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

  Decl_seq const& fields() const { return fields_; }

  Decl_seq fields_;
};


// A member of a record.
struct Field_decl : Decl
{
  using Decl::Decl;

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }
};


// A module is a sequence of top-level declarations.
struct Module_decl : Decl
{
  Module_decl(Symbol const* n, Decl_seq const& d)
    : Decl(n, nullptr), decls_(d)
  { }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

  Decl_seq const& declarations() const { return decls_; }

  Decl_seq decls_;
};


// A layout decl describes the layout of a
// packet header. These are similar to records, but
// objects of layouts cannot be made
// so therefore, this declaration has no intrinsic type,
// and are also discarded before code generation.
struct Layout_decl : Decl
{
  Layout_decl(Symbol const* n, Decl_seq const& f)
    : Decl(n, nullptr), fields_(f)
  { }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

  Decl_seq const& fields() const { return fields_; }

  Decl_seq fields_;
};


// A decoder declaration
// A decode declaration  is defined for a type and gives 
// conditions  to determine the next decoder in line.
//
// Stmt* s is a block stmt containing all stmt inside a decoder
// Type* h is the header type 
struct Decode_decl : Decl
{
  Decode_decl(Symbol const* n, Type const* t, Stmt* s, Type const* h)
    : Decl(n, t), header_(h), body_(s), start_(false)
  { }

  Decode_decl(Symbol const* n, Type const* t, Stmt* s, Type const* h, bool start)
    : Decl(n, t), header_(h), body_(s), start_(start)
  { }

  Type  const* header() const { return header_; }
  Stmt  const* body()   const { return body_; }
  Stmt*        body()        { return body_; }
  bool         is_start() const { return start_; }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

  void set_body(Stmt* s) { body_ = s; }
  void set_start() { start_ = true; }

  Type const* header_;
  Stmt* body_;
  bool start_;
};


// A flow table.
struct Table_decl : Decl
{
  // Table kind
  enum Table_kind
  {
    exact_table, 
    wildcard_table,
    prefix_table,
    string_table
  };

  // Default exact table
  Table_decl(Symbol const* n, Type const* t, int num, Expr_seq& conds, 
             Decl_seq& init)
    : Decl(n, t), num(num), conditions_(conds), body_(init), start_(false), kind_(exact_table)
  { }

  Table_decl(Symbol const* n, Type const* t, int num, Expr_seq& conds, 
             Decl_seq& init, Table_kind k)
    : Decl(n, t), num(num), conditions_(conds), body_(init), start_(false), kind_(k)
  { }


  int             number() const     { return num; }
  Expr_seq const& conditions() const { return conditions_; }
  Decl_seq const& body() const { return body_; }
  Table_kind      kind() const { return kind_; }
  bool is_start() const { return start_; }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }


  int      num;
  Expr_seq conditions_;
  Decl_seq body_;
  bool start_;
  Table_kind kind_;
};


// An entry within a flow table.
//
// FIXME: We should check during compile time that the
// length of the subkey does not exceed the maximum key
// size of the table.
struct Flow_decl : Decl
{
  Flow_decl(Expr_seq& conds, int prio, Stmt* i)
    : Decl(nullptr, nullptr), prio_(prio), keys_(conds), instructions_(i)
  { }
  
  int             priority() const { return prio_; }
  Expr_seq const& keys() const { return keys_; }
  Stmt const*     instructions() const { return instructions_; }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

  void set_instructions(Stmt* i) { instructions_ = i; }

  int prio_;
  Expr_seq keys_;
  Stmt* instructions_;
};


// Declaration for extracting a field into a context
// The name and type fields are applied during elaboration
struct Extracts_decl : Decl
{
  Extracts_decl(Expr* e)
    : Decl(nullptr, nullptr), field_(e)
  { }

  Expr* field() const { return field_; }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

  Expr* field_;
};


// Extracts a field using the same name as another field
struct Rebind_decl : Decl
{
  Rebind_decl(Expr* e1, Expr* e2)
    : Decl(nullptr, nullptr), f1(e1), f2(e2)
  { }

  Expr* field1() { return f1; }
  Expr* field2() { return f2; }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

  Expr* f1;
  Expr* f2;
};


// Declares the name of a port
struct Port_decl : Decl
{
  Port_decl(Symbol const* n, Type const* t)
    : Decl(n, t)
  { }

  void accept(Visitor& v) const { v.visit(this); }
  void accept(Mutator& v)       { v.visit(this); }

};


// -------------------------------------------------------------------------- //
// Queries

// Returns true if v is a global variable.
inline bool
is_global_variable(Variable_decl const* v)
{
  return is<Module_decl>(v->context());
}


// Returns true if v is a local variable.
//
// TODO: This actually depends more on storage properties
// than on declaration context. For example, if the language
// allowed static local variables (as in C++), then this
// would also need to check for an appropriate declaration
// specifier.
inline bool
is_local_variable(Variable_decl const* v)
{
  return is<Function_decl>(v->context());
}


// Returns true if the declaration defines an object.
inline bool 
defines_object(Decl const* d)
{
  return is<Variable_decl>(d) 
      || is<Parameter_decl>(d)
      || is<Field_decl>(d)
      || is<Table_decl>(d)
      || is<Flow_decl>(d)
      || is<Port_decl>(d);
}


// Returns true if a decl is a top-level pipeline declarations
//
//    top-level pipeline-decl -> decoders
//                               layouts
//                               tables
inline bool
is_pipeline_decl(Decl const* d)
{
  return is<Decode_decl>(d)
      || is<Layout_decl>(d)
      || is<Table_decl>(d);
}


// -------------------------------------------------------------------------- //
//                              Generic visitors

template<typename F, typename T>
struct Generic_decl_visitor : Decl::Visitor, lingo::Generic_visitor<F, T>
{
  Generic_decl_visitor(F fn)
    : lingo::Generic_visitor<F, T>(fn)
  { }


  void visit(Variable_decl const* d) { this->invoke(d); }
  void visit(Function_decl const* d) { this->invoke(d); }
  void visit(Parameter_decl const* d) { this->invoke(d); }
  void visit(Record_decl const* d) { this->invoke(d); }
  void visit(Field_decl const* d) { this->invoke(d); }
  void visit(Module_decl const* d) { this->invoke(d); }

  // network declarations
  void visit(Layout_decl const* d) { this->invoke(d); }  
  void visit(Decode_decl const* d) { this->invoke(d); }
  void visit(Table_decl const* d) { this->invoke(d); }
  void visit(Flow_decl const* d) { this->invoke(d); }
  void visit(Port_decl const* d) { this->invoke(d); }
  void visit(Extracts_decl const* d) { this->invoke(d); }
  void visit(Rebind_decl const* d) { this->invoke(d); }
};


// Apply fn to the declaration d.
template<typename F, typename T = typename std::result_of<F(Variable_decl const*)>::type>
inline T
apply(Decl const* d, F fn)
{
  Generic_decl_visitor<F, T> v = fn;
  return accept(d, v);
}


template<typename F, typename T>
struct Generic_decl_mutator : Decl::Mutator, lingo::Generic_mutator<F, T>
{
  Generic_decl_mutator(F fn)
    : lingo::Generic_mutator<F, T>(fn)
  { }

  void visit(Variable_decl* d) { this->invoke(d); }
  void visit(Function_decl* d) { this->invoke(d); }
  void visit(Parameter_decl* d) { this->invoke(d); }
  void visit(Record_decl* d) { this->invoke(d); }
  void visit(Field_decl* d) { this->invoke(d); }
  void visit(Module_decl* d) { this->invoke(d); }

  // network declarations
  void visit(Layout_decl* d) { this->invoke(d); }  
  void visit(Decode_decl* d) { this->invoke(d); }
  void visit(Table_decl* d) { this->invoke(d); }
  void visit(Flow_decl* d) { this->invoke(d); }
  void visit(Port_decl* d) { this->invoke(d); }
  void visit(Extracts_decl* d) { this->invoke(d); }
  void visit(Rebind_decl* d) { this->invoke(d); }
};


// Apply fn to the propositoin p.
template<typename F, typename T = typename std::result_of<F(Variable_decl*)>::type>
inline T
apply(Decl* d, F fn)
{
  Generic_decl_mutator<F, T> v = fn;
  return accept(d, v);
}


// -------------------------------------------------------------------------- //
//                                  Queries

// Returns true if the record `r` contains the member `m`.
//
// TODO: This is currently a linear search. We could optimize
// this by equipping the class with a hash set that stores
// know declrations.
//
// This function is used to guarntee compiler consistency
// in the checking of member expressions.
inline bool 
has_member(Record_decl const* r, Field_decl const* m)
{
  Decl_seq const& mem = r->fields();
  return std::find(mem.begin(), mem.end(), m) != mem.end();
}


// Returns the member decl with a specific name within a Record_decl
// or nullptr if no member declaration with the given name can
// be found.
inline Field_decl const*
find_member(Record_decl const* r, Symbol const* name)
{
  Decl_seq const& mems = r->fields();
  for (auto member : mems) {
    if (member->name() == name)
      return as<Field_decl>(member);
  }

  return nullptr;
}


// Returns the index of the member `m` in the record 
// declaration `r`.
inline int
member_index(Record_decl const* r, Field_decl const* m)
{
  Decl_seq const& mem = r->fields();
  auto iter = std::find(mem.begin(), mem.end(), m);
  return iter - mem.begin();
}


inline bool 
has_field(Layout_decl const* r, Field_decl const* m)
{
  Decl_seq const& mem = r->fields();
  return std::find(mem.begin(), mem.end(), m) != mem.end();
}


// Returns the member decl with a specific name within a Layout_decl
// or nullptr if no member declaration with the given name can
// be found.
inline Field_decl*
find_field(Layout_decl const* r, Symbol const* name)
{
  Decl_seq const& mems = r->fields();
  for (auto member : mems) {
    if (member->name() == name)
      return as<Field_decl>(member);
  }

  return nullptr;
}


// Returns the index of the member `m` in the record 
// declaration `r`.
inline int
field_index(Layout_decl const* r, Field_decl const* m)
{
  Decl_seq const& mem = r->fields();
  auto iter = std::find(mem.begin(), mem.end(), m);
  return iter - mem.begin();
}


#endif
