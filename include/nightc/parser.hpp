#pragma once

#include <deque>
#include <nightc/tokenizer.hpp>

enum class operation_unary
{
  negate,
  ones_complement,
  boolean_negate
};

enum class operation_binary
{
  add,
  subtract,
  multiply,
  divide
};

struct expression_node
{
  enum class expr_type
  {
    binary_op,
    unary_op,
    value,
    // todo: add variable node type: just an index into symbol table
  } m_type;
  union
  {
    struct binop
    {
      operation_binary m_op_binary;
      expression_node* m_lhs;
      expression_node* m_rhs;
    } m_binop;
    struct unop
    {
      operation_unary op_unary;
      expression_node* m_single;
    } m_unop;
    // todo: make atom a 64-bit unsigned and 64-bit float
    union atom
    {
      int m_value;
    } m_atom;
  };
};

// statements are pieces of code that do things.
// right now there is just a return statement.
// todo: other kinds of statements
struct statement_node
{
  expression_node* m_return_value;
};

// a block (scope) is a list of statements and declarations.
// todo: declarations, scoping, nested blocks.
struct block_node
{
  std::deque<statement_node*> m_statements;
};

// a function is a callable block.
// todo: replace with symbol table entry
struct function_node
{
  std::string_view m_name;
  block_node* m_block;
};

// a declaration is either a function or variable
// todo: replace with symbol table entry
struct declaration_node
{
  function_node* m_function;
};

// a program is a list of declarations of functions and variables.
// todo: replace with symbol table
struct program
{
  std::deque<declaration_node*> m_declarations;
};

expression_node* parse_expression(std::deque<token>& tokens, int rbp = 0);

statement_node* parse_statement(std::deque<token>& tokens);

block_node* parse_block(std::deque<token>& tokens);

function_node* parse_function(std::string_view name, std::deque<token>& tokens);

declaration_node* parse_declaration(std::deque<token>& tokens);

program* parse_program(std::deque<token>& tokens);
