#include <nightc/generator.hpp>

// todo: once IR is a thing, remove these generate_ functions.
// these are really just here so that the "vertical slice" can stay intact

// also,
// todo: make a generic interface for asm that's more expressive than printing everything

std::ostream& generate_expression(std::ostream& o, expression_node const& e)
{
  switch (e.m_type)
  {
  case expression_node::expr_type::binary_op:
    if (e.m_binop.m_op_binary == operation_binary::add
        || e.m_binop.m_op_binary == operation_binary::multiply)
    {
      // generate_expression will output code that puts the result in rax
      generate_expression(o, *(e.m_binop.m_lhs));
      o << "push rax\n";
      // the rhs will end up in rax
      generate_expression(o, *(e.m_binop.m_rhs));
      // put the lhs in rcx
      o << "pop rcx\n";
      // LHS: RCX
      // RHS: RAX
    }
    if (e.m_binop.m_op_binary == operation_binary::subtract
        || e.m_binop.m_op_binary == operation_binary::divide)
    {
      // generate_expression will output code that puts the result in rax
      generate_expression(o, *(e.m_binop.m_lhs));
      o << "push rax\n";
      // the rhs will end up in rax
      generate_expression(o, *(e.m_binop.m_rhs));
      // put the rhs in rcx
      o << "mov rcx, rax\n";
      // put the lhs in rax
      o << "pop rax\n";
      // LHS: RAX
      // RHS: RCX
    }
    // operate on rax and rcx, with result in rax.
    switch (e.m_binop.m_op_binary)
    {
    case operation_binary::add:
      // lhs = 
      o << "add rax, rcx\n";
      break;
    case operation_binary::subtract:
      // lhs = lhs - rhs
      o << "sub rax, rcx\n";
      break;
    case operation_binary::multiply:
      o << "imul rcx\n";
      break;
    case operation_binary::divide:
      // rax / rcx
      o << "xor rdx, rdx\n";
      o << "idiv rcx\n";
      break;
    }
    break;
  case expression_node::expr_type::unary_op:
    generate_expression(o, *(e.m_unop.m_single));
    // this can always operate on rax
    switch (e.m_unop.op_unary)
    {
    case operation_unary::negate:
      o << "neg rax\n";
      break;
    case operation_unary::ones_complement:
      o << "not rax\n";
      break;
    case operation_unary::boolean_negate:
      o << "test eax, eax\n"; // test if rax is 0
      o << "xor rax, rax\n";  // zero out rax
      o << "setz al\n";       // set lowest byte of rax to 1 if it was 0 
      break;
    }
    break;
  case expression_node::expr_type::value:
    o << "mov rax, " << e.m_atom.m_value << "\n";
    break;
  }
  return o;
}

std::ostream& generate_statement(std::ostream& o, statement_node const& s)
{
  generate_expression(o, *(s.m_return_value));
  o << "ret\n";
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
