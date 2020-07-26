#pragma once

#include <ostream>

struct register_id
{
  int value;
  bool is_valid() const { return value != invalid.value; }
  static const register_id invalid;
};

using r_id = register_id;

namespace gen_x64
{
// todo: this is stupid. make it less so.
r_id rax();

r_id load(std::ostream& o, int value);
void zero(std::ostream& o, r_id r1);

void mov(std::ostream& o, r_id r1, r_id r2);
void mov(std::ostream& o, r_id r1, int value);
void cmp(std::ostream& o, r_id r1, r_id r2);
void ret(std::ostream& o);

r_id neg(std::ostream& o, r_id r1);
r_id not(std::ostream& o, r_id r1);
r_id bool_neg(std::ostream& o, r_id r1);

r_id add(std::ostream& o, r_id r1, r_id r2);
r_id sub(std::ostream& o, r_id r1, r_id r2);
r_id mul(std::ostream& o, r_id r1, r_id r2);
r_id div(std::ostream& o, r_id r1, r_id r2);

r_id eq(std::ostream& o, r_id r1, r_id r2);
r_id neq(std::ostream& o, r_id r1, r_id r2);
r_id gt_eq(std::ostream& o, r_id r1, r_id r2);
r_id lt_eq(std::ostream& o, r_id r1, r_id r2);
r_id gt(std::ostream& o, r_id r1, r_id r2);
r_id lt(std::ostream& o, r_id r1, r_id r2);

r_id bool_and(std::ostream& o, r_id r1, r_id r2);
r_id bool_or(std::ostream& o, r_id r1, r_id r2);

}