#pragma once

#include <string_view>
#include <deque>

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

bool operator==(token const& t1, token const& t2);

token create_token_operation(std::string const& op);

token create_token_keyword(std::string const& keyword);

token create_token_identifier(std::string const& id);

token create_token_punctuation(char punc);

token create_token_constant(int i);

std::deque<token> tokenize_chunk(std::string const& chunk);

token const& view_next(std::deque<token> const& tokens);

void pop_next(std::deque<token>& tokens);

token consume(std::deque<token>& tokens, token const& t);

token consume_if_not(std::deque<token>& tokens, token::token_type t);
