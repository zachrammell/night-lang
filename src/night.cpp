#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include <unordered_set>
#include <filesystem>

#include <nightc/tokenizer.hpp>
#include <nightc/parser.hpp>
#include <nightc/generator.hpp>

namespace fs = std::filesystem;

constexpr int FAILURE = -1;
constexpr int SUCCESS =  0;

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
  "==",
  "!=",
  "<=",
  ">=",
  "<",
  ">",
  "+",
  "-",
  "*",
  "/",
  "!",
  "~",
  "(",
  ")"
};

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
  // print out for debugging purposes
  generate_program(std::cerr, *p);

  return SUCCESS;
}
