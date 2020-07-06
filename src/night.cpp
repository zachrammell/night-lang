#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <re2/re2.h>

constexpr int FAILURE = -1;
constexpr int SUCCESS =  0;

enum operation
{
  minus,
  ones_complement,
  boolean_negate,
  plus,
  multiply,
  divide
};

// todo: big string pool thing
std::unordered_set<std::string> keywords =
{
  "int",
  "return"
};

using punctuation = char;
std::unordered_map<std::string, operation> operations =
{
  {"-", minus},
  {"+", plus},
  {"~", ones_complement},
  {"!", boolean_negate},
  {"*", multiply},
  {"/", divide}
};

// todo: this becomes an index into string pool
struct keyword
{
  std::string name;
};

using token = std::variant<keyword, std::string, int, punctuation, operation>;

template <typename T0, typename ... Ts>
std::ostream & operator<< (std::ostream & s,
                           std::variant<T0, Ts...> const & v)
 { std::visit([&](auto && arg){ s << arg;}, v); return s; }

std::deque<token> tokenize_chunk_regex(std::string const& chunk)
{
  std::deque<token> tokens;
  re2::StringPiece input(chunk);
  std::stringstream capture_groups;
  capture_groups << "(?P<keyword>return|int)" << "|";
  capture_groups << "(?P<identifier>[[:alpha:]]+)" << "|";
  capture_groups << "(?P<literal>[[:digit:]]+)" << "|";
  capture_groups << "(?P<operator_2char>!=)" << "|";
  capture_groups << "(?P<operator_1char>[~!\\+\\-\\*\\/])" << "|";
  capture_groups << "(?P<punctuation>[(){};:])"; // << "|";
  re2::RE2 expr(capture_groups.str());
  std::string keyword;
  std::string identifier;
  std::string literal;
  std::string op_2;
  std::string op_1;
  std::string punctuation;

  while(!input.empty() && RE2::Consume(&input, expr, &keyword, &identifier, &literal, &op_2, &op_1, &punctuation))
  {
    if (!keyword.empty())
    {
      std::cout << "found a keyword: " << keyword << std::endl;
      tokens.emplace_back(token{ ::keyword{keyword} });
    }
    else if (!identifier.empty())
    {
      std::cout << "found an identifier: " << identifier << std::endl;
      tokens.emplace_back(token{ identifier });
    }
    else if (!literal.empty())
    {
      std::cout << "found a constant: " << literal << std::endl;
      tokens.emplace_back(token{ std::in_place_type_t<int>{}, std::stoi(literal) });
    }
    else if (!op_2.empty())
    {
      std::cout << "found a 2-char operator: " << op_2 << std::endl;
      tokens.emplace_back(token{ std::in_place_type_t<operation>{}, ::operation{operations[op_2]} });
    }
    else if (!op_1.empty())
    {
      std::cout << "found a 1-char operator: " << op_1 << std::endl;
      tokens.emplace_back(token{ std::in_place_type_t<operation>{}, operation{operations[op_1]} });
    }
    else if (!punctuation.empty())
    {
      std::cout << "found punctuation: " << punctuation << std::endl;
      tokens.emplace_back(token{ std::in_place_type_t<::punctuation>{}, ::punctuation{punctuation[0]} });
    }
  }
  return tokens;
}

struct unary_op;

struct constant
{
  int value;
};

using expression = std::variant<unary_op*, constant*>;

struct unary_op
{
  operation op;
  expression* exp;
};

struct statement
{
  // for now there are only return statements.
  expression* m_expression;
};

struct function
{
  // TODO: index into string pool
  std::string m_name;
  // for now, functions can have only one statement
  statement* m_body;
};

struct program
{
  // for now, programs can only have one function
  function* m_entry_point;
};

expression* parse_expression(std::deque<token>& tokens)
{
  token piece = tokens.front();
  tokens.pop_front();
  if (std::holds_alternative<int>(piece))
  {
    return new expression{new constant{std::get<int>(piece)}};
  }
  if (std::holds_alternative<operation>(piece))
  {
    return new expression{
      new unary_op{
      std::get<operation>(piece),
      parse_expression(tokens)
      }
    };
  }
}

statement* parse_statement(std::deque<token>& tokens)
{
  token ret = tokens.front();
  tokens.pop_front();
  if (std::get<keyword>(ret).name != "return")
  {
    std::cerr << "Error: Statement must take form 'return n;' where n is an integer literal.\n";
  }

  expression* e = parse_expression(tokens);

  token terminator = tokens.front();
  tokens.pop_front();
  if (std::get<punctuation>(terminator) != ';')
  {
    std::cerr << "Error: Missing semicolon after statement.\n";
  }

  return new statement{e};
}

function* parse_function(std::deque<token>& tokens)
{
  function* f = new function;

  {
  token name = tokens.front();
  tokens.pop_front();
  f->m_name = std::get<std::string>(name);
  }

  {
  token param_list_open = tokens.front();
  tokens.pop_front();
  if (std::get<punctuation>(param_list_open) != '(')
  {
    std::cerr << "Error: Missing parameter list after function name.\n";
  }
  }

  // eventually parameters go here
  {
  token param_list_close = tokens.front();
  tokens.pop_front();
  if (std::get<punctuation>(param_list_close) != ')')
  {
    std::cerr << "Error: Unclosed parameter list after function name.\n";
  }
  }

  {
  token return_type_separator = tokens.front();
  tokens.pop_front();
  if (std::get<punctuation>(return_type_separator) != ':')
  {
    std::cerr << "Error: Unclosed parameter list after function name.\n";
  }
  }

  {
  token return_type = tokens.front();
  tokens.pop_front();
  if (std::get<keyword>(return_type).name != "int")
  {
    std::cerr << "Error: Only ints can be returned.\n";
  }
  }

  {
  token body_open = tokens.front();
  tokens.pop_front();
  if (std::get<punctuation>(body_open) != '{')
  {
    std::cerr << "Error: Missing '{' after function header.\n";
  }
  }

  f->m_body = parse_statement(tokens);

  {
  token body_close = tokens.front();
  tokens.pop_front();
  if (std::get<punctuation>(body_close) != '}')
  {
    std::cerr << "Error: Missing '}' after function body.\n";
  }
  }

  return f;
}

program* parse_program(std::deque<token>& tokens)
{
  program* p = new program;
  p->m_entry_point = parse_function(tokens);

  if (!tokens.empty())
  {
    // todo: remove once multiple functions allowed
    std::cerr << "Error: Extra code after function declaration.\n";
  }

  return p;
}

int optimize_expression(expression& e)
{
  constexpr int is_constant = 1;
  if (std::holds_alternative<constant*>(e))
  {
    return is_constant;
  }
  else if (std::holds_alternative<unary_op*>(e))
  {
    unary_op* op = std::get<unary_op*>(e);
    if (optimize_expression(*(op->exp)) == is_constant)
    {
      switch (op->op)
      {
        //todo: don't leak memory
      case operation::minus:
      {
        auto* prev_constant = std::get<constant*>(*(op->exp));
        e.emplace<constant*>(new constant{ -(prev_constant->value) });
        delete prev_constant;
        return is_constant;
      }
      case operation::ones_complement:
      {
        auto* prev_constant = std::get<constant*>(*(op->exp));
        e.emplace<constant*>(new constant{ ~(std::get<constant*>(*(op->exp)))->value });
        delete prev_constant;
        return is_constant;
      }
      case operation::boolean_negate:
      {
        auto* prev_constant = std::get<constant*>(*(op->exp));
        e.emplace<constant*>(new constant{ !(std::get<constant*>(*(op->exp)))->value });
        delete prev_constant;
        return is_constant;
      }
      }
    }
  }
  return !is_constant;
}

void optimize_statement(statement& s)
{
  optimize_expression(*s.m_expression);
}

void optimize_function(function& f)
{
  optimize_statement(*f.m_body);
}

void optimize_program(program& p)
{
  optimize_function(*p.m_entry_point);
}

void generate_expression(expression const& e, std::ostream& output);

void generate_unary_op(unary_op const& u, std::ostream& output)
{
  generate_expression(*u.exp, output);
  switch (u.op)
  {
  case minus:
    output << "neg eax\n";
    break;
  case ones_complement:
    output << "not eax\n";
    break;
  case boolean_negate:
    output << "test eax, eax\n"; // test if eax is 0
    output << "xor eax, eax\n";  // zero out eax
    output << "setz al\n";       // set lowest byte of eax to 1 if it was 0 
    break;
  }
}

void generate_expression(expression const& e, std::ostream& output)
{
  if (std::holds_alternative<constant*>(e))
  {
    output << "mov eax, " << std::get<constant*>(e)->value << "\n";
  }
  else if (std::holds_alternative<unary_op*>(e))
  {
    generate_unary_op(*std::get<unary_op*>(e), output);
  }
}

void generate_statement(statement const& s, std::ostream& output)
{
  generate_expression(*s.m_expression, output);
  output << "ret\n";
}

void generate_function(function const& f, std::ostream& output)
{
  output << ".globl " << f.m_name << "\n";
  output << f.m_name << ":\n";
  generate_statement(*f.m_body, output);
}

void generate_program(program const& p, std::ostream& output)
{
  output << ".text\n";
  output << ".intel_syntax noprefix\n";
  generate_function(*p.m_entry_point, output);
}

int main(int argc, char** argv)
{
  if (argc == 1)
  {
    std::cerr << "Error: No input files.\n";
    return FAILURE;
  }

  std::string input_filename { argv[1] };
  std::ifstream input_file { input_filename };
  
  std::deque<token> tokens;
  {
    std::string chunk;
    while (input_file >> chunk)
    {
      auto&& tokenized = tokenize_chunk_regex(chunk);
      //auto&& tokenized = tokenize_chunk(chunk);
      for (auto& token : tokenized)
      {
        tokens.emplace_back(token);
      }
    }
  }

  // parse tokens into AST
  program* p = parse_program(tokens);

  // AST optimization
  optimize_program(*p);
  
  // conversion into IR

  // IR optimization

  // generate assembly output
  std::ofstream out_assembly{ "asm.s" };
  generate_program(*p, out_assembly);

  return SUCCESS;
}
