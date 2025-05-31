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
    ASM::Code allocateLi(ASM::LiPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                         FunctionPtr &func);
    ASM::Code allocateLa(ASM::LaPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                         FunctionPtr &func);
    ASM::Code allocateLoad(ASM::LoadPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                           FunctionPtr &func);
    ASM::Code allocateStore(ASM::StorePtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                            FunctionPtr &func);
    ASM::Code allocateJump(ASM::JumpPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                           FunctionPtr &func);
    ASM::Code allocateBranch(ASM::BranchPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                             FunctionPtr &func);
    ASM::Code allocateCall(ASM::CallPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                           FunctionPtr &func);
    ASM::Code allocateRet(ASM::RetPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                          FunctionPtr &func);
    ASM::Code allocateLabel(ASM::LabelPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                            FunctionPtr &func);
    ASM::Code allocateFunction(ASM::FunctionPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                               FunctionPtr &func);
    ASM::Code allocateZero(ASM::ZeroPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                           FunctionPtr &func);
    ASM::Code allocateWord(ASM::WordPtr &inst, std::set<ASM::Reg> &available_regs, std::map<ASM::Reg, ASM::Reg> &reg_map,
                           FunctionPtr &func);
    ASM::Code allocateGlobalLabel(ASM::GlobalLabelPtr &inst, std::set<ASM::Reg> &available_regs,
                                  std::map<ASM::Reg, ASM::Reg> &reg_map, FunctionPtr &func);
};

#endif // CODEGEN_REG_ALLOCATOR_HPP