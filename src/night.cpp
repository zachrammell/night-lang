#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <filesystem>

#include <re2/re2.h>

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
std::unordered_set<std::string> const keywords =
{
  "int",
  "return"
};

// todo: put into string pool.
std::unordered_set<std::string> identifiers;

std::unordered_set<std::string> const operators =
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

#pragma region Tokenizer

// todo: this becomes an index into string pool
struct keyword
{
  std::string_view name;
};

// todo: include source file information in token (filename, line number, position)
struct token
{
  enum class token_type
  {
    invalid,
    operation,
    keyword,
    identifier,
    punctuation,
    constant
  } m_type;

  union
  {
    std::string_view m_operation;
    keyword m_keyword;
    std::string_view m_identifier;
    char m_punctuation;
    int m_constant;
  };
};

bool operator==(token const& t1, token const& t2)
{
  if (t1.m_type != t2.m_type)
  {
    return false;
  }

  switch (t1.m_type)
  {
  case token::token_type::operation:
    return t1.m_operation == t2.m_operation;
  case token::token_type::keyword:
    return t1.m_keyword.name == t2.m_keyword.name;
  case token::token_type::identifier:
    return t1.m_identifier == t2.m_identifier;
  case token::token_type::punctuation:
    return t1.m_punctuation == t2.m_punctuation;
  case token::token_type::constant:
    return t1.m_constant == t2.m_constant;
  }

  return false;
}

token create_token_operation(std::string const& op)
{
  token t = token{ token::token_type::operation };
  t.m_operation = { operators.find(op)->data() };
  return t;
}

token create_token_keyword(std::string const& keyword)
{
  token t = token{ token::token_type::keyword };
  t.m_keyword = { keywords.find(keyword)->data() };
  return t;
}

token create_token_identifier(std::string const& id)
{
  token t = token{ token::token_type::identifier };
  auto stored_id = identifiers.find(id);
  if (stored_id == identifiers.end())
  {
    stored_id = identifiers.insert(id).first;
  }
  t.m_identifier = { stored_id->data() };
  return t;
}

token create_token_punctuation(char punc)
{
  token t = { token::token_type::punctuation };
  t.m_punctuation = punc;
  return t;
}

token create_token_constant(int i)
{
  token t = { token::token_type::constant };
  t.m_constant = i;
  return t;
}

std::deque<token> tokenize_chunk(std::string const& chunk)
{
  // todo: move initialization code like this out of the function
  std::deque<token> tokens;
  re2::StringPiece input(chunk);
  // todo: automatic regex builder for keywords, operators.
  std::stringstream capture_groups;
  // matches /* and */
  // maybe just make it capture the entire comment and do away with nesting. who knows.
  capture_groups << "(?P<comment>" R"(\/\*|\*\/)" ")" << "|";
  capture_groups << "(?P<keyword>return|int)" << "|";
  capture_groups << "(?P<identifier>[[:alpha:]]+)" << "|";
  capture_groups << "(?P<constant>[[:digit:]]+)" << "|";
  capture_groups << "(?P<operator_2char>" R"(&&|\|\|)" ")" << "|";
  capture_groups << "(?P<operator_1char>" R"([~!\+\-\*\/])" ")" << "|";
  capture_groups << "(?P<punctuation>[(){};:])"; // << "|";
  re2::RE2 expr(capture_groups.str());
  std::string comment;
  std::string keyword;
  std::string identifier;
  std::string constant;
  std::string op_2;
  std::string op_1;
  std::string punc;

  static size_t comment_depth = 0;

  while(!input.empty()
    && RE2::Consume(&input, expr, &comment, &keyword, &identifier, &constant, &op_2, &op_1, &punc))
  {
    // todo: maybe do comments in a pre-pass? unless that's too slow or parsing them is useful for something
    if (!comment.empty())
    {
      if (comment == "/*")
      {
        ++comment_depth;
      }
      else if (comment == "*/")
      {
        // decrementing allows for nested comments.
        // for C-like comment behavior, set depth to 0 here.
        --comment_depth;
      }
    }

    // discards everything while in a comment. could capture the text for something if needed
    if (comment_depth > 0)
    {
      continue;
    }

    unless (keyword.empty())
    {
      std::cout << "found a keyword: " << keyword << std::endl;
      tokens.emplace_back(create_token_keyword(keyword));
    }
    else unless (identifier.empty())
    {
      std::cout << "found an identifier: " << identifier << std::endl;
      tokens.emplace_back(create_token_identifier(identifier));
    }
    else unless (constant.empty())
    {
      std::cout << "found a constant: " << constant << std::endl;
      tokens.emplace_back(create_token_constant(std::stoi(constant)));
    }
    else unless (op_2.empty())
    {
      std::cout << "found a 2-char operator: " << op_2 << std::endl;
      tokens.emplace_back(create_token_operation(op_2));
    }
    else unless (op_1.empty())
    {
      std::cout << "found a 1-char operator: " << op_1 << std::endl;
      tokens.emplace_back(create_token_operation(op_1));
    }
    else unless (punc.empty())
    {
      std::cout << "found punctuation: " << punc << std::endl;
      tokens.emplace_back(create_token_punctuation(punc[0]));
    }
  }
  return tokens;
}

token const& view_next(std::deque<token> const& tokens)
{
  return tokens.front();
}

void pop_next(std::deque<token>& tokens)
{
  tokens.pop_front();
}

token consume(std::deque<token>& tokens, token const& t)
{
  token const tok = view_next(tokens);
  if (tok == t)
  {
    pop_next(tokens);
    return tok;
  }

  // todo: error handling via optional (monad) here?
  return {};
}

token consume_if_not(std::deque<token>& tokens, token::token_type t)
{
  token const tok = view_next(tokens);
  if (tok.m_type != t)
  {
    pop_next(tokens);
    return tok;
  }

  // todo: error handling via optional (monad) here?
  return {};
}

#pragma endregion 

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

  // todo: generic assembly interface for output to NASM or JIT

  return SUCCESS;
}
