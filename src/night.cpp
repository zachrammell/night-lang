#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <deque>
#include <unordered_set>
#include <variant>

#include <re2/re2.h>

constexpr int FAILURE = -1;
constexpr int SUCCESS =  0;

enum token_punctuation
{
  paren_l,
  paren_r,
  bracket_l,
  bracket_r,
  semicolon,
  colon,
};

enum token_operator_unary
{
  negate,
  ones_complement,
  boolean_negate,
};

// todo: big string pool thing
std::unordered_set<std::string> keywords =
{
  "int",
  "return"
};

// todo: this becomes an index into string pool
struct keyword
{
  std::string name;
};

using token = std::variant<keyword, std::string, int, token_punctuation, token_operator_unary>;

template <typename T0, typename ... Ts>
std::ostream & operator<< (std::ostream & s,
                           std::variant<T0, Ts...> const & v)
 { std::visit([&](auto && arg){ s << arg;}, v); return s; }

std::deque<token> tokenize_chunk(std::string const& chunk)
{
  std::deque<token> tokens;

  enum tokenizer_state
  {
    in_none,
    in_text,
    in_number,
  } state = in_none;

  auto classify_character = []
  (char c) -> tokenizer_state
  {
    if (isdigit(c))
    {
      return in_number;
    }
    if (isalpha(c))
    {
      return in_text;
    }
    return in_none;
  };

  auto get_single = []
  (char c) -> std::variant<token_punctuation, token_operator_unary>
  {
    switch (c)
    {
      /* punctuation */
      case '(': return paren_l;
      case ')': return paren_r;
      case '{': return bracket_l;
      case '}': return bracket_r;
      case ';': return semicolon;
      case ':': return colon;

      /* unary operators */
      case '-': return negate;
      case '~': return ones_complement;
      case '!': return boolean_negate;
    }
  };

  auto add_text = [&tokens]
  (std::string && s)
  {
    if (keywords.find(s) != keywords.end())
    {
      tokens.emplace_back(token{keyword{s}});
      return;
    }
    tokens.emplace_back(token{s});
  };

  auto add_number = [&tokens]
  (std::string && s)
  {
    tokens.emplace_back(token{std::stoi(s)});
  };

  decltype(chunk.begin()) token_begin;
  for (auto c = chunk.begin(); c != chunk.end(); ++c)
  {
    switch (state)
    {
      case in_none:
      {
        if (isspace(*c))
        {
          continue;
        }
        tokenizer_state new_state = classify_character(*c);
        if (new_state != in_none)
        {
          state = new_state;
          token_begin = c;
          continue;
        }
        auto single = get_single(*c);
        if (std::holds_alternative<token_operator_unary>(single))
        {
          tokens.emplace_back(token{std::get<token_operator_unary>(single)});
        }
        else if (std::holds_alternative<token_punctuation>(single))
        {
          tokens.emplace_back(token{ std::get<token_punctuation>(single) });
        }
      } break;
      case in_text:
      {
        tokenizer_state new_state = classify_character(*c);
        if (new_state != in_text)
        {
          add_text(std::string{token_begin, c});
          state = new_state;
          --c;
          continue;
        }
      } break;
      case in_number:
      {
        tokenizer_state new_state = classify_character(*c);
        if (new_state != in_number)
        {
          add_number(std::string{token_begin, c});
          state = new_state;
          --c;
          continue;
        }
      } break;
    }
  }
  switch (state)
  {
    case in_text:
      add_text(std::string{token_begin, chunk.end()});
      break;
    case in_number:
      add_number(std::string{token_begin, chunk.end()});
      break;
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
  token_operator_unary op;
  expression* exp;
};

struct statement
{
  // for now there are only return statements.
  expression* m_expression;
};

struct function
{
  // name of function
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
  if (std::holds_alternative<token_operator_unary>(piece))
  {
    return new expression{
      new unary_op{
      std::get<token_operator_unary>(piece),
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
  if (std::get<token_punctuation>(terminator) != semicolon)
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
  if (std::get<token_punctuation>(param_list_open) != paren_l)
  {
    std::cerr << "Error: Missing parameter list after function name.\n";
  }
  }

  // eventually parameters go here
  {
  token param_list_close = tokens.front();
  tokens.pop_front();
  if (std::get<token_punctuation>(param_list_close) != paren_r)
  {
    std::cerr << "Error: Unclosed parameter list after function name.\n";
  }
  }

  {
  token return_type_separator = tokens.front();
  tokens.pop_front();
  if (std::get<token_punctuation>(return_type_separator) != colon)
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
  if (std::get<token_punctuation>(body_open) != bracket_l)
  {
    std::cerr << "Error: Missing '{' after function header.\n";
  }
  }

  f->m_body = parse_statement(tokens);

  {
  token body_close = tokens.front();
  tokens.pop_front();
  if (std::get<token_punctuation>(body_close) != bracket_r)
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
  if (std::holds_alternative<constant*>(e))
  {
    return 1;
  }
  else if (std::holds_alternative<unary_op*>(e))
  {
    unary_op* op = std::get<unary_op*>(e);
    if (optimize_expression(*(op->exp)))
    {
      switch (op->op)
      {
        //todo: don't leak memory
        case operation::negate:
        e.emplace<constant*>(new constant{-(std::get<constant*>(*(op->exp)))->value});
        return 1;
        case operation::ones_complement:
        e.emplace<constant*>(new constant{~(std::get<constant*>(*(op->exp)))->value});
        return 1;
        case operation::boolean_negate:
        e.emplace<constant*>(new constant{!(std::get<constant*>(*(op->exp)))->value});
        return 1;
      }
    }
  }
  return 0;
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
  case negate:
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
      auto&& tokenized = tokenize_chunk(chunk);
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
  // generate assembly output
  std::ofstream out_assembly{ "asm.s" };
  generate_program(*p, out_assembly);

  return SUCCESS;
}
