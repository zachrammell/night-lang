#include <nightc/codegen_x64.hpp>

#include <array>
#include <string>

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
    }
    if (registers[reg.value] != register_state::allocated)
    {
      // error trying to free register
      __debugbreak();
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
  o << "mov " << allocator.get_name(r) << ", " << value << "\n";
  return r;
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
  // todo: lowest byte only
  o << "setz " << r << "\n";       // set lowest byte of r1 to 1 if it was 0
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

r_id mov(std::ostream& o, r_id r1, r_id r2)
{
  // mov r1, r2
  o << "mov " << allocator.get_name(r1) << ", " << allocator.get_name(r2) << "\n";
  return r1;
}

void ret(std::ostream& o)
{
  allocator.deallocate_all_registers();
  // ret
  o << "ret\n";
}

}
