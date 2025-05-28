#ifndef CODEGEN_REG_ALLOCATOR_HPP
#define CODEGEN_REG_ALLOCATOR_HPP

class ASMEmitter;

#include "analysis/control_flow.hpp"

class RegAllocator {
 public:
  void allocate(Module &mod);
  void allocate(FunctionPtr &func);
  ASM::Code allocate(ASM::Code &asm_code, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                    FunctionPtr &func);    
  ASM::Code allocate(ASM::InstPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                     FunctionPtr &func);

 private:

    ASM::Code allocateArith(ASM::ArithPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                            FunctionPtr &func);
    ASM::Code allocateArithImm(ASM::ArithImmPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                               FunctionPtr &func);
    ASM::Code allocateMv(ASM::MvPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                         FunctionPtr &func);
    

};

#endif  // CODEGEN_REG_ALLOCATOR_HPP