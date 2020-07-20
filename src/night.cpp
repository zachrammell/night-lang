#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

#include <nightc/tokenizer.hpp>

#define until(exp) while(!(exp))
#define unless(exp) if(!(exp))

namespace fs = std::filesystem;

constexpr int FAILURE = -1;
constexpr int SUCCESS =  0;

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

// todo: big string pool thing
extern std::unordered_set<std::string> const keywords =
{
  "int",
  "return"
};

// todo: put into string pool.
std::unordered_set<std::string> identifiers;

extern std::unordered_set<std::string> const operators =
{
  "&&",
  "||",
  "+",
  "-",
  "*",
  "/",
  "!",
  "~"
};

// todo: memory allocator for AST and expression trees so we aren't calling new out the wazoo

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

expression_node* parse_expression(std::deque<token>& tokens, int rbp = 0);

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

// statements are pieces of code that do things.
// right now there is just a return statement.
// todo: other kinds of statements
struct statement_node
{
  expression_node* m_return_value;
};

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

// a block (scope) is a list of statements and declarations.
// todo: declarations, scoping, nested blocks.
struct block_node
{
  std::deque<statement_node*> m_statements;
};

block_node* parse_block(std::deque<token>& tokens)
{
  {
    token const open_bracket = view_next(tokens);
    pop_next(tokens);
    unless(open_bracket.m_type == token::token_type::punctuation
           && open_bracket.m_punctuation == '{')
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
    unless(close_bracket.m_type == token::token_type::punctuation
           && close_bracket.m_punctuation == '}')
    {
      // PARSE_ERROR: expected block of code opened with '}'
    }
  }

  return b;
}

// a function is a callable block.
// todo: replace with symbol table entry
struct function_node
{
  std::string_view m_name;
  block_node* m_block;
};

function_node* parse_function(std::string_view name, std::deque<token>& tokens)
{
  {
    token const open_paren = view_next(tokens);
    pop_next(tokens);
    unless(open_paren.m_type == token::token_type::punctuation
           && open_paren.m_punctuation == '(')
    {
      // no param list: ???
    }
  }

  // todo: parse parameter list

  {
    token const close_paren = view_next(tokens);
    pop_next(tokens);
    unless(close_paren.m_type == token::token_type::punctuation
           && close_paren.m_punctuation == ')')
    {
      // PARSE_ERROR: parameter list closed improperly
    }
  }

  {
    token const return_type_separator = view_next(tokens);
    pop_next(tokens);
    unless(return_type_separator.m_type == token::token_type::punctuation
           && return_type_separator.m_punctuation == ':')
    {
      // PARSE_ERROR: no return type
      // todo: allow missing return type (void)
    }
  }

  {
    token const return_type = view_next(tokens);
    pop_next(tokens);
    unless(return_type.m_type == token::token_type::keyword
           && return_type.m_keyword.name == "int")
    {
      // PARSE_ERROR: return type must be int
      // todo: type system
    }
  }

  return new function_node{ name, parse_block(tokens) };
}

// a declaration is either a function or variable
// todo: replace with symbol table entry
struct declaration_node
{
  function_node* m_function;
};

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
  if (tok.m_type == token::token_type::punctuation
      && tok.m_punctuation == '(')
  {
    // parse a function
    return new declaration_node{ parse_function(name, tokens) };
  }
  // todo: parse a variable

  return nullptr;
}

// a program is a list of declarations of functions and variables.
// todo: replace with symbol table
struct program
{
  std::deque<declaration_node*> m_declarations;
};

program* parse_program(std::deque<token>& tokens)
{
  program* p = new program;

  // parse all declarations
  until (tokens.empty())
  {
    p->m_declarations.push_back(parse_declaration(tokens));
  }

  return p;
}

// todo: once IR is a thing, remove these generate_ functions.
// these are really just here so that the "vertical slice" can stay intact

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
  o << "extern _ExitProcess@4\n";

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

int main(int argc, char** argv)
{
  if (argc == 1)
  {
    std::cerr << "Error: No input files.\n";
    return FAILURE;
  }

  fs::path in_path{ argv[1] };
  if (!fs::exists(in_path))
  {
    std::cerr << "Error: Input file " << in_path.generic_string() << " does not exist.\n";
    return FAILURE;
  }
  std::ifstream input_file { in_path };

  // tokenize input file
  std::deque<token> tokens;
  {
    // todo: just read the whole file into memory at once.
    std::string chunk;
    while (input_file >> chunk)
    {
      auto const tokenized = tokenize_chunk(chunk);
      for (auto& token : tokenized)
      {
        tokens.emplace_back(token);
      }
    }
  }

  // parse tokens into AST
  program* p = parse_program(tokens);

  // AST optimization
  
  // conversion into IR

  // IR optimization

  // generate assembly output
  std::ofstream out_assembly{ "asm.s" };
  generate_program(out_assembly, *p);
  generate_program(std::cout, *p);

  // todo: generic assembly interface for output to NASM or JIT

  return SUCCESS;
}
