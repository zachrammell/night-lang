#include <nightc/tokenizer.hpp>
#include <sstream>
#include <string>
#include <unordered_set>

#include <re2/re2.h>

extern std::unordered_set<std::string> const operators;
extern std::unordered_set<std::string> identifiers;
extern std::unordered_set<std::string> const keywords;

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
  capture_groups << "(?P<operator_1char>[" R"(~!\+\-\*\/)" "\\(\\)" "])" << "|";
  capture_groups << "(?P<punctuation>[{};:])"; // << "|";
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

    if (!keyword.empty())
    {
      //std::cout << "found a keyword: " << keyword << std::endl;
      tokens.emplace_back(create_token_keyword(keyword));
    }
    else if (!identifier.empty())
    {
      //std::cout << "found an identifier: " << identifier << std::endl;
      tokens.emplace_back(create_token_identifier(identifier));
    }
    else if (!constant.empty())
    {
      //std::cout << "found a constant: " << constant << std::endl;
      tokens.emplace_back(create_token_constant(std::stoi(constant)));
    }
    else if (!op_2.empty())
    {
      //std::cout << "found a 2-char operator: " << op_2 << std::endl;
      tokens.emplace_back(create_token_operation(op_2));
    }
    else if (!op_1.empty())
    {
      //std::cout << "found a 1-char operator: " << op_1 << std::endl;
      tokens.emplace_back(create_token_operation(op_1));
    }
    else if (!punc.empty())
    {
      //std::cout << "found punctuation: " << punc << std::endl;
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
