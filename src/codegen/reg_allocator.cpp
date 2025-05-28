#include "reg_allocator.hpp"

#include <cassert>
#include <set>

void RegAllocator::allocate(Module &mod) {
  for (auto &func : mod.functions) {
    allocate(func);
  }
}

void RegAllocator::allocate(FunctionPtr &func) {
  func->alloc_reg(8);   // allocate space for return address and previous fp
  ASM::RegMap reg_map;  // virtual register to physical register
  std::set<ASM::Reg> available_regs = {
      ASM::Reg::t0,  ASM::Reg::t1, ASM::Reg::t2, ASM::Reg::t3, ASM::Reg::t4,
      ASM::Reg::t5,  ASM::Reg::t6, ASM::Reg::s2, ASM::Reg::s3, ASM::Reg::s4,
      ASM::Reg::s5,  ASM::Reg::s6, ASM::Reg::s7, ASM::Reg::s8, ASM::Reg::s9,
      ASM::Reg::s10, ASM::Reg::s11};
  // 在这里实现寄存器分配算法，将结果保存在 reg_map 中
  // tips: 对于朴素的仅用到三个寄存器的算法，可能用不到 reg_map

  for (auto &block : func->blocks) {
    block->asm_code = allocate(block->asm_code, available_regs, reg_map, func);
  }

  func->reg_map = reg_map;
}

ASM::Code RegAllocator::allocate(ASM::Code &asm_code,
                                 std::set<ASM::Reg> &available_regs,
                                 ASM::RegMap &reg_map,
                                 FunctionPtr &func) {
  ASM::Code asm_code_allocated;
  for (auto &inst : asm_code) {
    auto inst_allocated = allocate(inst, available_regs, reg_map, func);
    asm_code_allocated.insert(asm_code_allocated.end(), inst_allocated.begin(),
                              inst_allocated.end());
  }
  return asm_code_allocated;
}

ASM::Code RegAllocator::allocate(ASM::InstPtr &inst,
                                 std::set<ASM::Reg> &available_regs,
                                 ASM::RegMap &reg_map,
                                 FunctionPtr &func) {
#define ALLOCATE_INST(type)                                  \
  if (auto p = std::dynamic_pointer_cast<ASM::type>(inst)) { \
    return allocate##type(p, available_regs, reg_map, func);       \
  }
  // 对于每种不同类型的 ASM 指令，调用相应的 allocate 函数
  ALLOCATE_INST(Arith)
// ALLOCATE_INST(ArithImm)
// ALLOCATE_INST(Mv)
// ALLOCATE_INST(Li)
// ALLOCATE_INST(Label)
// ALLOCATE_INST(Function)
// ALLOCATE_INST(Call)
// ALLOCATE_INST(Jump)
// ALLOCATE_INST(Store)
// ALLOCATE_INST(Load)
// ALLOCATE_INST(Exit)
// ALLOCATE_INST(Arg)
// ALLOCATE_INST(Return)
#warning Add more ASM instruction types if needed

  assert(false && "Unknown ASM instruction type");
}

ASM::Code RegAllocator::allocateArith(ASM::ArithPtr &inst,
                                      std::set<ASM::Reg> &available_regs,
                                      ASM::RegMap &reg_map,
                                      FunctionPtr &func) {
  // 对于每条汇编指令，为其中的虚拟寄存器分配空间并在语句前后添加 lw 和 sw 指令
  /*
    # Example: a = b + c
    lw t1, 8(sp)        # load b
    lw t2, 12(sp)       # load c
    add t0, t1, t2      # add reg(a), reg(b), reg(c)
    sw t0, 4(sp)        # store a
  */
  ASM::Code asm_code_allocated;

  auto get_stack_offset = [&](const ASM::Reg &reg) -> int {
    if (reg.is_phys()) return -1; // 物理寄存器不需要分配空间
    
    // 为虚拟寄存器分配空间
    return func->alloc_temp(4, reg);
  };

  // 选择可用的物理寄存器
  auto get_temp_reg = [&](int index) -> ASM::Reg {
    static const ASM::Reg temp_regs[] = {ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2};
    return temp_regs[index % 3];
  };

  ASM::Reg phys_rs1 = inst->rs1;
  if (!inst->rs1.is_phys()) {
    int offset = get_stack_offset(inst->rs1);
    phys_rs1 = get_temp_reg(1); // 使用 t1 作为临时寄存器

    // 生成load指令
    auto load_inst = ASM::Load::create(phys_rs1, ASM::Reg::sp, offset);
    asm_code_allocated.push_back(load_inst);
  }

  // 同理处理 rs2
  ASM::Reg phys_rs2 = inst->rs2;
  if (!inst->rs2.is_phys()) {
    int offset = get_stack_offset(inst->rs2);
    phys_rs2 = get_temp_reg(2); // 使用 t2 作为临时寄存器

    // 生成load指令
    auto load_inst = ASM::Load::create(phys_rs2, ASM::Reg::sp, offset);
    asm_code_allocated.push_back(load_inst);
  }
  
  // 处理 rd
  ASM::Reg phys_rd = inst->rd;
  int rd_offset = -1;
  if (!inst->rd.is_phys()) {
    rd_offset = get_stack_offset(inst->rd);
    phys_rd = get_temp_reg(0); // 使用 t0 作为临时寄存器
  }

  // 生成算术指令
  auto arith_inst = ASM::Arith::create(phys_rd, phys_rs1, phys_rs2, inst->op);
  asm_code_allocated.push_back(arith_inst);

  // 如果 rd 是虚拟寄存器，则需要将结果存回栈
  if (!inst->rd.is_phys()) {
    auto store_inst = ASM::Store::create(ASM::Reg::sp, phys_rd, rd_offset);
    asm_code_allocated.push_back(store_inst);
  }
    
  // 更新寄存器映射
  reg_map[inst->rd] = phys_rd;
  reg_map[inst->rs1] = phys_rs1;
  reg_map[inst->rs2] = phys_rs2;

  
  return asm_code_allocated;
}
