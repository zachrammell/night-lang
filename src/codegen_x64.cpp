#include <nightc/codegen_x64.hpp>

#include <array>
#include <string>

const register_id register_id::invalid = {-1};

namespace gen_x64
{

static class register_allocator
{
  enum class register_state
  {
    unallocated,
    allocated,
  };
  constexpr static int register_limit = 4;
  constexpr static register_id out_of_registers = { -1 };
  std::array<register_state, register_limit> registers = {};
  std::array<std::string, register_limit + 1> register_names = {
    "r8", "r9", "r10", "r11", "rax"
  };

public:
  // todo: this is stupid. make it less so.
  constexpr static int rax = register_limit;

  void deallocate_all_registers()
  {
    for (register_state& reg : registers)
    {
      reg = register_state::unallocated;
    }
  }

  void deallocate_register(register_id reg)
  {
    if (!reg.is_valid())
    {
      // this is bad
      __debugbreak();
      return;
    }
    if (registers[reg.value] != register_state::allocated)
    {
      // error trying to free register
      __debugbreak();
      return;
    }
    registers[reg.value] = register_state::unallocated;
  }

  register_id allocate_register()
  {
    for (int i = 0; i < registers.size(); ++i)
    {
      if (registers[i] == register_state::unallocated)
      {
        registers[i] = register_state::allocated;
        return register_id{ i };
      }
    }
    return out_of_registers;
  }

  std::string_view get_name(register_id reg)
  {
    if (!reg.is_valid())
    {
      // this is bad.
      return "BIG_ERROR";
    }
    return register_names[reg.value];
  }

} allocator;

static std::string get_label_id()
{
  static size_t label_id = 0;
  ++label_id;
  return std::string{ ".L" }.append(std::to_string(label_id));
}

r_id rax()
{
  return r_id{ register_allocator::rax };
}


r_id load(std::ostream& o, int value)
{
  r_id const r = allocator.allocate_register();
  mov(o, r, value);
  return r;
}

void zero(std::ostream& o, r_id r1)
{
  // xor r1, r1
  o << "xor " << allocator.get_name(r1) << ", " << allocator.get_name(r1) << "\n";
}


void mov(std::ostream& o, r_id r1, r_id r2)
{
  // mov r1, r2
  o << "mov " << allocator.get_name(r1) << ", " << allocator.get_name(r2) << "\n";
}

void mov(std::ostream& o, r_id r1, int value)
{
  // mov r1, r2
  o << "mov " << allocator.get_name(r1) << ", " << value << "\n";
}

void cmp(std::ostream& o, r_id r1, r_id r2)
{
  // cmp r1, r2
  o << "cmp " << allocator.get_name(r1) << ", " << allocator.get_name(r2) << "\n";
}

void ret(std::ostream& o)
{
  allocator.deallocate_all_registers();
  // ret
  o << "ret\n";
}


r_id neg(std::ostream& o, r_id r1)
{
  // neg r1
  o << "neg " << allocator.get_name(r1) << "\n";
  return r1;
}

r_id not(std::ostream& o, r_id r1)
{
  // not r1
  o << "not " << allocator.get_name(r1) << "\n";
  return r1;
}

r_id bool_neg(std::ostream& o, r_id r1)
{
  auto const r = allocator.get_name(r1);
  // test r1, r1
  o << "test " << r << ", " << r << "\n"; // test if r1 is 0
  // xor r1, r1
  o << "xor " << r << ", " << r << "\n";  // zero out r1
  // setz r1
  // todo: lookup name of low byte of r1
  o << "setz " << r << "b\n";       // set lowest byte of r1 to 1 if it was 0
  return r1;
}


r_id add(std::ostream& o, r_id r1, r_id r2)
{
  // add r1, r2
  o << "add " << allocator.get_name(r1) << ", " << allocator.get_name(r2) << "\n";
  allocator.deallocate_register(r2);
  return r1;
}

r_id sub(std::ostream& o, r_id r1, r_id r2)
{
  // sub r1, r2
  o << "sub " << allocator.get_name(r1) << ", " << allocator.get_name(r2) << "\n";
  allocator.deallocate_register(r2);
  return r1;
}

r_id mul(std::ostream& o, r_id r1, r_id r2)
{
  // mul r1, r2
  o << "imul " << allocator.get_name(r1) << ", " << allocator.get_name(r2) << "\n";
  allocator.deallocate_register(r2);
  return r1;
}

r_id div(std::ostream& o, r_id r1, r_id r2)
{
  // mov rax, r1
  mov(o, rax(), r1);
  // cqo (to sign-extend rax)
  o << "cqo\n";
  // idiv r2
  o << "idiv " << allocator.get_name(r2);
  // mov r1, rax
  mov(o, r1, rax());
  allocator.deallocate_register(r2);
  return r1;
}


static r_id compare_generic(std::ostream& o, r_id r1, r_id r2, std::string_view comparison_suffix)
{
  cmp(o, r1, r2);
  mov(o, r1, 0);
  // todo: lookup name of low byte of r1
  o << "set" << comparison_suffix << ' ' << allocator.get_name(r1) << "b\n";
  allocator.deallocate_register(r2);
  return r1;
}

r_id eq(std::ostream& o, r_id r1, r_id r2)
{
  return compare_generic(o, r1, r2, "e");
}

r_id neq(std::ostream& o, r_id r1, r_id r2)
{
  return compare_generic(o, r1, r2, "ne");
}

r_id gt_eq(std::ostream& o, r_id r1, r_id r2)
{
  return compare_generic(o, r1, r2, "ge");
}

r_id lt_eq(std::ostream& o, r_id r1, r_id r2)
{
  return compare_generic(o, r1, r2, "le");
}

r_id gt(std::ostream& o, r_id r1, r_id r2)
{
  return compare_generic(o, r1, r2, "g");
}

r_id lt(std::ostream& o, r_id r1, r_id r2)
{
  return compare_generic(o, r1, r2, "l");
}

//r_id bool_and(std::ostream& o, r_id r1, r_id r2)
//{
//  std::string const labels[2] = { get_label_id(), get_label_id() };
//  // test r1, r1
//  o << "test " << allocator.get_name(r1) << ", " << allocator.get_name(r1) << "\n";
//  // jne .L0
//  o << "jne " << labels[0] << "\n";
//  allocator.deallocate_register(r1);
//  // jmp .L1
//  o << "jmp " << labels[1];
//  // .L0:
//  o << labels[0] << ":\n";
//  // will test, and deallocate r1
//  compare_generic(o, r2, r1, "ne");
//  // .L1:
//  o << labels[1] << ":\n";
//  return r2;
//}
//
//r_id bool_or(std::ostream& o, r_id r1, r_id r2)
//{
//  std::string const labels[2] = { get_label_id(), get_label_id() };
//  // test r1, r1
//  o << "test " << allocator.get_name(r1) << ", " << allocator.get_name(r1) << "\n";
//  // je .L0
//  o << "je " << labels[0] << "\n";
//  // mov r2, 1
//  mov(o, r2, 1);
//  allocator.deallocate_register(r1);
//  // jmp .L1
//  o << "jmp " << labels[1];
//  // .L0:
//  o << labels[0] << ":\n";
//  // will test, and deallocate r1
//  compare_generic(o, r2, r1, "ne");
//  // .L1:
//  o << labels[1] << ":\n";
//  return r2;
//}

}
