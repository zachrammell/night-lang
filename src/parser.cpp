#include <nightc/parser.hpp>

#include <unordered_map>

// todo: centralize binding powers (make an enum or table or something)

// this OOP business is kind of silly.
// maybe there should be no inheritance and simply a struct with lbp and fn pointers/lambdas.
// unless virtual dispatch is actually more convenient than that.
struct token_info_base
{
  virtual int lbp() { return 0; }
  virtual expression_node* nud(std::deque<token>& tokens) = 0;
  virtual expression_node* led(std::deque<token>& tokens, expression_node* left) = 0;
};

// todo: other literals (in separate definitions)
struct token_info_constant : token_info_base
{
  token_info_constant(int i) : m_value{ i } {}
  expression_node* nud(std::deque<token>&) override
  {
    expression_node* expr_value = new expression_node{ expression_node::expr_type::value };
    expr_value->m_atom.m_value = m_value;
    return expr_value;
  }
  expression_node* led(std::deque<token>&, expression_node*) override { return nullptr; }
  int m_value;
};

struct token_info_plus : token_info_base
{
  int lbp() override { return 10; }
  expression_node* nud(std::deque<token>& tokens) override
  {
    return parse_expression(tokens, 100);
  }
  expression_node* led(std::deque<token>& tokens, expression_node* left) override
  {
    expression_node* right = parse_expression(tokens, lbp());
    expression_node* expr_add = new expression_node{ expression_node::expr_type::binary_op };
    expr_add->m_binop.m_lhs = left;
    expr_add->m_binop.m_op_binary = operation_binary::add;
    expr_add->m_binop.m_rhs = right;
    return expr_add;
  }
};

struct token_info_minus : token_info_base
{
  int lbp() override { return 10; }
  expression_node* nud(std::deque<token>& tokens) override
  {
    expression_node* expr_negate = new expression_node{ expression_node::expr_type::unary_op };
    expr_negate->m_unop.op_unary = operation_unary::negate;
    expr_negate->m_unop.m_single = parse_expression(tokens, 100);
    return expr_negate;
  }
  expression_node* led(std::deque<token>& tokens, expression_node* left) override
  {
    expression_node* right = parse_expression(tokens, lbp());
    expression_node* expr_subtract = new expression_node{ expression_node::expr_type::binary_op };
    expr_subtract->m_binop.m_lhs = left;
    expr_subtract->m_binop.m_op_binary = operation_binary::subtract;
    expr_subtract->m_binop.m_rhs = right;
    return expr_subtract;
  }
};

struct token_info_asterisk : token_info_base
{
  int lbp() override { return 20; }
  expression_node* nud(std::deque<token>&) override { return nullptr; }
  expression_node* led(std::deque<token>& tokens, expression_node* left) override
  {
    expression_node* right = parse_expression(tokens, lbp());
    expression_node* expr_multiply = new expression_node{ expression_node::expr_type::binary_op };
    expr_multiply->m_binop.m_lhs = left;
    expr_multiply->m_binop.m_op_binary = operation_binary::multiply;
    expr_multiply->m_binop.m_rhs = right;
    return expr_multiply;
  }
};

struct token_info_slash : token_info_base
{
  int lbp() override { return 20; }
  expression_node* nud(std::deque<token>&) override { return nullptr; }
  expression_node* led(std::deque<token>& tokens, expression_node* left) override
  {
    expression_node* right = parse_expression(tokens, lbp());
    expression_node* expr_divide = new expression_node{ expression_node::expr_type::binary_op };
    expr_divide->m_binop.m_lhs = left;
    expr_divide->m_binop.m_op_binary = operation_binary::divide;
    expr_divide->m_binop.m_rhs = right;
    return expr_divide;
  }
};

struct token_info_logical_and : token_info_base
{
  int lbp() override { return 20; }
  expression_node* nud(std::deque<token>&) override { return nullptr; }
  expression_node* led(std::deque<token>& tokens, expression_node* left) override
  {
    expression_node* right = parse_expression(tokens, lbp());
    expression_node* expr_plus = new expression_node{ expression_node::expr_type::binary_op };
    expr_plus->m_binop.m_lhs = left;
    expr_plus->m_binop.m_op_binary = operation_binary::divide;
    expr_plus->m_binop.m_rhs = right;
    return expr_plus;
  }
};

struct token_info_expr_end : token_info_base
{
  int lbp() override { return 0; }
  expression_node* nud(std::deque<token>&) override { return nullptr; }
  expression_node* led(std::deque<token>&, expression_node*) override { return nullptr; }
};

// todo: memory allocator for AST and expression trees so we aren't calling new out the wazoo

std::unordered_map<std::string_view, token_info_base*> op_token_info =
{
  {"__expr_end", new token_info_expr_end{}},
  {"+", new token_info_plus{}},
  {"-", new token_info_minus{}},
  {"*", new token_info_asterisk{}},
  {"/", new token_info_slash{}}
};

// todo: smart pointers.
token_info_base* get_token_info(token const& t)
{
  switch (t.m_type)
  {
  case token::token_type::operation:
    return op_token_info.at(t.m_operation);
  case token::token_type::constant:
    // todo: fix this trash
    return new token_info_constant{ t.m_constant };
  case token::token_type::punctuation:
    if (t.m_punctuation == ';')
    {
      return op_token_info.at("__expr_end");
    }
  }

  // something happened
  return nullptr;
}

// https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
// read that ^
expression_node* parse_expression(std::deque<token>& tokens, int rbp)
{
  token tok = view_next(tokens);
  pop_next(tokens);
  token_info_base* tok_info = get_token_info(tok);

  expression_node* left = tok_info->nud(tokens);

  while (rbp < get_token_info(view_next(tokens))->lbp())
  {
    tok = view_next(tokens);
    pop_next(tokens);
    tok_info = get_token_info(tok);

    left = tok_info->led(tokens, left);
  }

  return left;
}

statement_node* parse_statement(std::deque<token>& tokens)
{
  token const tok = view_next(tokens);

  // return statement takes the form "return <expression>;"
  if (tok.m_type == token::token_type::keyword)
  {
    if (tok.m_keyword.name == "return")
    {
      pop_next(tokens);

      // parse the return statement's expression
      expression_node* return_expr = parse_expression(tokens);

      token const semicolon = view_next(tokens);
      pop_next(tokens);
      if (semicolon.m_type == token::token_type::punctuation
       && semicolon.m_punctuation == ';')
      {
        return new statement_node{ return_expr };
      }
      else
      {
        // PARSE_ERROR: expected ';' after return statement
      }
    }
  }

  // todo: implement other statements

  // this was not a statement?
  return nullptr;
}

block_node* parse_block(std::deque<token>& tokens)
{
  {
    token const open_bracket = view_next(tokens);
    pop_next(tokens);
    if (!(open_bracket.m_type == token::token_type::punctuation
           && open_bracket.m_punctuation == '{'))
    {
      // PARSE_ERROR: expected block of code opened with '{'
    }
  }

  block_node* b = new block_node{};

  // parse all statements
  while (statement_node* s = parse_statement(tokens))
  {
    b->m_statements.push_back(s);
  }

  // todo: parse declarations

  // todo: parse nested blocks and enforce scoping rules

  {
    token const close_bracket = view_next(tokens);
    pop_next(tokens);
    if (!(close_bracket.m_type == token::token_type::punctuation
           && close_bracket.m_punctuation == '}'))
    {
      // PARSE_ERROR: expected block of code opened with '}'
    }
  }

  return b;
}

function_node* parse_function(std::string_view name, std::deque<token>& tokens)
{
  {
    token const open_paren = view_next(tokens);
    pop_next(tokens);
    if (!(open_paren.m_type == token::token_type::operation
           && open_paren.m_operation[0] == '('))
    {
      // no param list: ???
    }
  }

  // todo: parse parameter list

  {
    token const close_paren = view_next(tokens);
    pop_next(tokens);
    if (!(close_paren.m_type == token::token_type::operation
           && close_paren.m_operation[0] == ')'))
    {
      // PARSE_ERROR: parameter list closed improperly
    }
  }

  {
    token const return_type_separator = view_next(tokens);
    pop_next(tokens);
    if (!(return_type_separator.m_type == token::token_type::punctuation
           && return_type_separator.m_punctuation == ':'))
    {
      // PARSE_ERROR: no return type
      // todo: allow missing return type (void)
    }
  }

  {
    token const return_type = view_next(tokens);
    pop_next(tokens);
    if (!(return_type.m_type == token::token_type::keyword
           && return_type.m_keyword.name == "int"))
    {
      // PARSE_ERROR: return type must be int
      // todo: type system
    }
  }

  return new function_node{ name, parse_block(tokens) };
}

declaration_node* parse_declaration(std::deque<token>& tokens)
{

  std::string_view name;
  token const name_tok = view_next(tokens);
  pop_next(tokens);
  if (name_tok.m_type == token::token_type::identifier)
  {
    name = name_tok.m_identifier;
  }
  else
  {
    // PARSE_ERROR: expected name of function or variable
  }

  token const tok = view_next(tokens);
  if (tok.m_type == token::token_type::operation
      && tok.m_operation[0] == '(')
  {
    // parse a function
    return new declaration_node{ parse_function(name, tokens) };
  }
  // todo: parse a variable

  return nullptr;
}

program* parse_program(std::deque<token>& tokens)
{
  program* p = new program;

  // parse all declarations
  while (!tokens.empty())
  {
    p->m_declarations.push_back(parse_declaration(tokens));
  }

  return p;
}
