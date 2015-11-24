#include "lower.hpp"
#include "error.hpp"
#include "mangle.hpp"

#include <iostream>

// Helper function for constructing
// identifier symbols
Symbol const*
Lowerer::get_identifier(std::string s)
{
  return elab.syms.put<Identifier_sym>(s, identifier_tok);
}

namespace
{

struct Lower_expr_fn
{
  Lowerer& lower;

  // catch all case
  // simply return the original
  // expression without lowering it
  template<typename T>
  Expr* operator()(T* e) const { return e; }

  // Field name expr
  // becomes an id_expr whose declaration is
  // resolved against a variable created by lowering
  // the extracts decl
  Expr* operator()(Field_name_expr* e) const { return lower.lower(e); }
};


struct Lower_decl_fn
{
  Lowerer& lower;

  // catch all case
  // return the original declaration
  template<typename T>
  Decl* operator()(T* d) const { lower.declare(d); return d; }

  Decl* operator()(Module_decl* d) const { return lower.lower(d); }

  // network declarations
  Decl* operator()(Flow_decl* d) const { return lower.lower(d); }
};


struct Lower_stmt_fn
{
  Lowerer& lower;

  // catch all case
  template<typename T>
  Stmt_seq operator()(T* s) const { return { s }; }

  // TODO: consider how this works and if we allow this
  // Stmt_seq operator()(Assign_stmt* s) const { return lower.lower(s); }

  Stmt_seq operator()(Empty_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(Block_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(If_then_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(If_else_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(Match_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(Case_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(While_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(Expression_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(Declaration_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(Decode_stmt* s) const { return lower.lower(s); }
  Stmt_seq operator()(Goto_stmt* s) const { return lower.lower(s); }
};


struct Lower_globals_fn
{
  Lowerer& lower;

  // Catch all for non-lowered globals
  template<typename T>
  Decl* operator()(T* d) const { return d; }

  Decl* operator()(Decode_decl* d) const { return lower.lower_global(d); }
  Decl* operator()(Table_decl* d) const { return lower.lower_global(d); }
  Decl* operator()(Port_decl* d) const { return lower.lower_global(d); }
};

} // namespace





// ------------------------------------------------------------------------- //
//                    Lower Expressions


Expr*
Lowerer::lower(Expr* e)
{
  return apply(e, Lower_expr_fn{*this});
}


Expr*
Lowerer::lower(Field_name_expr* e)
{
  return e;
}


// ------------------------------------------------------------------------- //
//                    Lower Declarations

Decl*
Lowerer::lower_global(Decl* d)
{
  Lower_globals_fn fn{*this};

  return apply(d, fn);
}


Decl*
Lowerer::lower_global(Decode_decl* d)
{
  // enter a scope
  Scope_sentinel scope(*this, d);

  // declare an implicit context variable
  Type const* cxt_ref = get_reference_type(get_context_type());
  Parameter_decl* cxt = new Parameter_decl(get_identifier(__context), cxt_ref);

  declare(cxt);

  Stmt* body = lower(d->body()).back();

  // The type of all decoders is fn(Context&) -> void
  Function_decl* fn = new Function_decl(d->name(), d->type(), {cxt}, body);

  redeclare(fn);

  return fn;
}


Decl*
Lowerer::lower_global(Table_decl* d)
{
  return d;
}


Decl*
Lowerer::lower_global(Port_decl* d)
{
  Function_decl* fn = builtin.get_builtin_fn(__get_port);
  Get_port* call = new Get_port(decl_id(fn));

  Variable_decl* port = new Variable_decl(d->name(), d->type(), call);

  redeclare(port);

  return port;
}


Decl*
Lowerer::lower(Decl* d)
{
  return apply(d, Lower_decl_fn{*this});
}


Decl*
Lowerer::lower(Module_decl* d)
{
  Scope_sentinel scope(*this, d);

  Decl_seq module_decls;

  // declare all builtins
  for (auto pair : builtin.get_builtins()) {
    declare(pair.second);
  }

  // declare all globals
  for (Decl* decl : d->declarations()) {
    declare(decl);
  }

  // lower all globals
  Lower_globals_fn fn{*this};
  for (Decl* decl : d->declarations()) {
    Decl* lowered = apply(decl, fn);
    std::cout << *lowered << '\n';
    module_decls.push_back(lowered);
  }

  return d;
}


Decl*
Lowerer::lower(Flow_decl* d)
{
  return d;
}


// -------------------------------------------------------------------------- //
//                    Lowering Statements



Stmt_seq
Lowerer::lower(Stmt* s)
{
  return apply(s, Lower_stmt_fn{*this});
}


// The lowering of a block statement
// causes the generation of a new block
// whose body is a concatenation of all lowered
// statements within the original block.
Stmt_seq
Lowerer::lower(Block_stmt* s)
{
  Stmt_seq stmts;

  for (auto stmt : s->statements()) {
    Stmt_seq new_stmts = lower(stmt);
    stmts.insert(stmts.end(), new_stmts.begin(), new_stmts.end());
  }

  // TODO: possibly check that these are still the same
  // statements and if they are just return the same block.
  Block_stmt* new_block = new Block_stmt(stmts);

  return { new_block };
}


// The lowering of an if stmt
// causes the lowering of its condition,
// and its branch
Stmt_seq
Lowerer::lower(If_then_stmt* s)
{
  Expr* condition = lower(s->condition());

  // this should only ever produce 1 stmt
  // otherwise there is an internal inconsistency
  Stmt* body = lower(s->body()).back();

  // create a new if statement
  If_then_stmt* ifthen = new If_then_stmt(condition, body);

  return { ifthen };
}


Stmt_seq
Lowerer::lower(If_else_stmt* s)
{
  Expr* condition = lower(s->condition());

  Stmt* true_branch = lower(s->true_branch()).back();
  Stmt* false_branch = lower(s->false_branch()).back();

  // create a new if statement
  If_else_stmt* ifelse = new If_else_stmt(condition, true_branch, false_branch);

  return { ifelse };
}


Stmt_seq
Lowerer::lower(Match_stmt* s)
{
  // lower the condition and each case
  // stmt in turn
  Expr* condition = lower(s->condition());

  Stmt_seq cases;

  // these should all be case statements
  for (auto c : s->cases()) {
    // these should all produce exactly one stmt
    Stmt* stmt = lower(c).back();
    cases.push_back(stmt);
  }

  Match_stmt* match = new Match_stmt(condition, cases);

  return { match };
}


// A case stmt lowering causes a lowering
// of its body. The label should be a Literal
// value which does not need lowering.
Stmt_seq
Lowerer::lower(Case_stmt* s)
{
  Stmt_seq body = lower(s->stmt());
  Case_stmt* c = new Case_stmt(s->label(), block(body));

  return { c };
}


Stmt_seq
Lowerer::lower(While_stmt* s)
{
  Expr* condition  = lower(s->condition());

  Stmt* body = lower(s->body()).back();

  While_stmt* whil = new While_stmt(condition, body);

  return { whil };
}


Stmt_seq
Lowerer::lower(Expression_stmt* s)
{
  Expr* expr = lower(s->expression());

  Expression_stmt* expr_stmt = nullptr;

  if (expr != s->expression())
    expr_stmt = new Expression_stmt(expr);
  else
    expr_stmt = s;

  return { expr_stmt };
}


Stmt_seq
Lowerer::lower_extracts_decl(Extracts_decl* d)
{
  // get the context from the decoder functionl
  Overload* ovl = unqualified_lookup(get_identifier(__context));
  Decl* cxt = ovl->back();

  // get the id from the pipeline checker
  int mapping = checker.get_field_mapping(d->name());

  Field_name_expr* field = as<Field_name_expr>(d->field());
  assert(field);

  // get the offset into the layout of the field
  Expr* offset = get_offset(field);

  // get the length of the field
  Expr* length = get_length(field);

  // create the binding call
  Expr_seq args
  {
    decl_id(cxt),
    make_int(mapping),
    offset,
    length
  };
  Expr* bind_field = builtin.call_bind_field(args);
  bind_field = elab.elaborate(bind_field);

  // create the loading call
  args =
  {
    decl_id(cxt),
    make_int(mapping)
  };
  Expr* load_fld = builtin.call_load_field(args);
  load_fld = elab.elaborate(load_fld);

  // Mangle the name of the variable from the name of the
  // extracted field.
  Symbol const* field_name = get_identifier(mangle(d));
  Variable_decl* load_var = new Variable_decl(field_name, d->type(), load_fld);

  Stmt_seq stmts {
    new Expression_stmt(bind_field),
    new Declaration_stmt(load_var)
  };

  return stmts;
}


// We change a rebind decl into a call to the
// implicit function __double_bind_offset(cxt, true_env_offset, aliased_env_offset, offsetof, lengthof)
//
// bind field1 as field2
//
// The aliased env offset is the number it would receive if its name was field2
// The true_env offset is the number it would receive if its name was field1
Stmt_seq
Lowerer::lower_rebind_decl(Rebind_decl* d)
{
  Stmt_seq stmts;

  return stmts;
}



Stmt_seq
Lowerer::lower(Declaration_stmt* s)
{
  Stmt_seq stmts;

  // These are exceptions to the lowering
  // process as they are declarations which
  // lower into call expressions instead of
  // other declarations
  if (Extracts_decl* extract = as<Extracts_decl>(s->declaration())) {
    Stmt_seq l = lower_extracts_decl(extract);
    stmts.insert(stmts.end(), l.begin(), l.end());
    return stmts;
  }

  if (Rebind_decl* rebind = as<Rebind_decl>(s->declaration())) {
    Stmt_seq l = lower_rebind_decl(rebind);
    stmts.insert(stmts.end(), l.begin(), l.end());
    return stmts;
  }

  // Regular lowering process for decl stmts
  if (Decl* decl = lower(s->declaration())) {
    if (decl != s->declaration())
      stmts.push_back(new Declaration_stmt(decl));
    else
      stmts.push_back(s);
  }

  return stmts;
}


Stmt_seq
Lowerer::lower(Decode_stmt* s)
{
  return {};
}


Stmt_seq
Lowerer::lower(Goto_stmt* s)
{
  return {};
}


// -------------------------------------------------------------------------- //
// Declaration of entities


// Determine if d can be overloaded with the existing
// elements in the set.
void
Lowerer::overload(Overload& ovl, Decl* curr)
{
  // Check to make sure that curr does not conflict with any
  // declarations in the current overload set.
  for (Decl* prev : ovl) {
    // If the two declarations have the same type, this
    // is not overloading. It is redefinition.
    if (prev->type() == curr->type()) {
      std::stringstream ss;
      ss << "redefinition of " << *curr->name() << '\n';
      throw Type_error({}, ss.str());
    }

    if (!can_overload(prev, curr)) {
      std::stringstream ss;
      ss << "cannot overload " << *curr->name() << '\n';
      throw Type_error({}, ss.str());
    }
  }

  ovl.push_back(curr);
}


// Create a declarative binding for d. This also checks
// that the we are not redefining a symbol in the current
// scope.
void
Lowerer::declare(Decl* d)
{
  Scope& scope = stack.current();

  // Set d's declaration context.
  d->cxt_ = stack.context();

  // If we've already seen the name, we should
  // determine if it can be overloaded.
  if (Scope::Binding* bind = scope.lookup(d->name()))
    return overload(bind->second, d);

  // Create a new overload set.
  Scope::Binding& bind = scope.bind(d->name(), {});
  Overload& ovl = bind.second;
  ovl.push_back(d);
}


// When opening the scope of a previously declared
// entity, simply push the declaration into its
// overload set.
void
Lowerer::redeclare(Decl* d)
{
  Scope& scope = stack.current();
  Overload* ovl;
  if (Scope::Binding* bind = scope.lookup(d->name()))
    ovl = &bind->second;
  else
    ovl = &scope.bind(d->name(), {}).second;
  ovl->push_back(d);
}


// Perform lookup of an unqualified identifier. This
// will search enclosing scopes for the innermost
// binding of the identifier.
Overload*
Lowerer::unqualified_lookup(Symbol const* sym)
{
  if (Scope::Binding* bind = stack.lookup(sym))
    return &bind->second;
  else
    return nullptr;
}


// Perform a qualified lookup of a name in the given
// scope. This searches only that scope for a binding
// for the identifier.
Overload*
Lowerer::qualified_lookup(Scope* s, Symbol const* sym)
{
  if (Scope::Binding* bind = s->lookup(sym))
    return &bind->second;
  else
    return nullptr;
}
