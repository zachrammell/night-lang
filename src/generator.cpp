#include <nightc/generator.hpp>

#include <string>

#include <nightc/codegen_x64.hpp>

// todo: once IR is a thing, remove these generate_ functions.
// these are really just here so that the "vertical slice" can stay intact

// also,
// todo: make a generic interface for asm that's more expressive than printing everything

static r_id generate_expression_recursive(std::ostream& o, expression_node const& e)
{
  switch (e.m_type)
  {
  case expression_node::expr_type::binary_op:
  {
      // todo: allow for short circuiting
    r_id const r_left = generate_expression_recursive(o, *(e.m_binop.m_lhs));
    r_id const r_right = generate_expression_recursive(o, *(e.m_binop.m_rhs));
    switch (e.m_binop.m_op_binary)
    {
      /* arithmetic */
    case operation_binary::add:
      return gen_x64::add(o, r_left, r_right);
    case operation_binary::subtract:
      return gen_x64::sub(o, r_left, r_right);
    case operation_binary::multiply:
      return gen_x64::mul(o, r_left, r_right);
    case operation_binary::divide:
      return gen_x64::div(o, r_left, r_right);

      /* comparison */
    case operation_binary::equal:
      return gen_x64::eq(o, r_left, r_right);
    case operation_binary::not_equal:
      return gen_x64::neq(o, r_left, r_right);
    case operation_binary::more_or_equal:
      return gen_x64::gt_eq(o, r_left, r_right);
    case operation_binary::less_or_equal:
      return gen_x64::lt_eq(o, r_left, r_right);
    case operation_binary::more:
      return gen_x64::gt(o, r_left, r_right);
    case operation_binary::less:
      return gen_x64::lt(o, r_left, r_right);

      /* logical */
    //case operation_binary::logical_and:
    //  return gen_x64::bool_and(o, r_left, r_right);
    //case operation_binary::logical_or:
    //  return gen_x64::bool_or(o, r_left, r_right);
    }
  }
    break;
  case expression_node::expr_type::unary_op:
  {
    r_id const r = generate_expression_recursive(o, *(e.m_unop.m_single));
    switch (e.m_unop.op_unary)
    {
      /* arithmetic */
    case operation_unary::negate:
      return gen_x64::neg(o, r);
      /* bitwise */
    case operation_unary::ones_complement:
      return gen_x64::not(o, r);
      /* logical */
    case operation_unary::logical_negate:
      return gen_x64::bool_neg(o, r);
    }
  }
    break;
  case expression_node::expr_type::value:
    return gen_x64::load(o, e.m_atom.m_value);
  }
  return r_id::invalid;
}

std::ostream& generate_expression(std::ostream& o, expression_node const& e)
{
  generate_expression_recursive(o, e);
  return o;
}

std::ostream& generate_statement(std::ostream& o, statement_node const& s)
{
  r_id const r1 = generate_expression_recursive(o, *(s.m_return_value));
  gen_x64::mov(o, gen_x64::rax(), r1);
  gen_x64::ret(o);
  return o;
}

std::ostream& generate_block(std::ostream& o, block_node const& b)
{
  for (auto const statement : b.m_statements)
  {
    generate_statement(o, *statement);
  }
  return o;
}

std::ostream& generate_function(std::ostream& o, function_node const& f)
{
  o << f.m_name << ":\n";

  generate_block(o, *(f.m_block));

  o << "\n"; // readability

  return o;
}

std::ostream& generate_declaration(std::ostream& o, declaration_node const& d)
{
  // if (is_function)
  return generate_function(o, *(d.m_function));
}

std::ostream& generate_program(std::ostream& o, program const& p)
{
  for (auto const decl : p.m_declarations)
  {
    o << "global " << decl->m_function->m_name << "\n";
  }
  o << "global __nightmain\n";
  o << "extern _ExitProcess@4\n\n";

  o << "section .text\n"; 
  for (auto const decl : p.m_declarations)
  {
    generate_declaration(o, *decl);
  }

  // the __nightmain function is the true entry point of all programs.
  // later we can add language startup code here if needed
  o << "__nightmain:\n";
  o << "sub rsp, 40\n";
  // todo: replaceable entry point name
  o << "call main\n";
  o << "add rsp, 40\n";
  o << "mov rcx, rax\n";
  o << "call _ExitProcess@4\n";

  return o;
}
