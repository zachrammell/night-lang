#pragma once

#include <iostream>

#include <nightc/parser.hpp>

std::ostream& generate_expression(std::ostream& o, expression_node const& e);

std::ostream& generate_statement(std::ostream& o, statement_node const& s);

std::ostream& generate_block(std::ostream& o, block_node const& b);

std::ostream& generate_function(std::ostream& o, function_node const& f);

std::ostream& generate_declaration(std::ostream& o, declaration_node const& d);

std::ostream& generate_program(std::ostream& o, program const& p);

