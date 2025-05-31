#include "reg_allocator.hpp"

#include <cassert>
#include <set>

void RegAllocator::allocate(Module &mod) {
    for (auto &func : mod.functions) {
        allocate(func);
    }
}

void RegAllocator::allocate(FunctionPtr &func) {
    // func->alloc_reg(4, ASM::Reg::ra); // 为返回地址分配空间
    // func->alloc_reg(4, ASM::Reg::fp); // 为帧指针分配空间
    ASM::RegMap reg_map; // virtual register to physical register
    std::set<ASM::Reg> available_regs = {
        ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2, ASM::Reg::t3, ASM::Reg::t4,
        ASM::Reg::t5, ASM::Reg::t6, ASM::Reg::s2, ASM::Reg::s3, ASM::Reg::s4,
        ASM::Reg::s5, ASM::Reg::s6, ASM::Reg::s7, ASM::Reg::s8, ASM::Reg::s9,
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
#define ALLOCATE_INST(type)                                      \
    if (auto p = std::dynamic_pointer_cast<ASM::type>(inst)) {   \
        return allocate##type(p, available_regs, reg_map, func); \
    }
    // 对于每种不同类型的 ASM 指令，调用相应的 allocate 函数
    ALLOCATE_INST(Arith)
    ALLOCATE_INST(ArithImm)
    ALLOCATE_INST(Mv)
    ALLOCATE_INST(Li)
    ALLOCATE_INST(Label)
    ALLOCATE_INST(Function)
    ALLOCATE_INST(Call)
    ALLOCATE_INST(Jump)
    ALLOCATE_INST(Store)
    ALLOCATE_INST(Load)
    ALLOCATE_INST(Ret)
    ALLOCATE_INST(Zero)
    ALLOCATE_INST(Word)
    ALLOCATE_INST(La)
    ALLOCATE_INST(Branch) // 添加这一行
    ALLOCATE_INST(GlobalLabel)

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
    // reg_map.emplace(inst->rd, phys_rd);
    // if (!inst->rs1.is_phys()) {
    //   reg_map.emplace(inst->rs1, phys_rs1);
    // }
    // if (!inst->rs2.is_phys()) {
    //   reg_map.emplace(inst->rs2, phys_rs2);
    // }

    return asm_code_allocated;
}

ASM::Code RegAllocator::allocateArithImm(ASM::ArithImmPtr &inst,
                                         std::set<ASM::Reg> &available_regs,
                                         ASM::RegMap &reg_map,
                                         FunctionPtr &func) {
    ASM::Code result;

    auto get_stack_offset = [&](const ASM::Reg &reg) -> int {
        if (reg.is_phys()) return -1;
        return func->alloc_temp(4, reg);
    };

    auto get_temp_reg = [&](int index) -> ASM::Reg {
        static const ASM::Reg temp_regs[] = {ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2};
        return temp_regs[index % 3];
    };

    // 处理 rs1
    ASM::Reg phys_rs1 = inst->rs1;
    if (!inst->rs1.is_phys()) {
        int offset = get_stack_offset(inst->rs1);
        phys_rs1 = get_temp_reg(1);
        auto load_inst = ASM::Load::create(phys_rs1, ASM::Reg::sp, offset);
        result.push_back(load_inst);
    }

    // 处理 rd
    ASM::Reg phys_rd = inst->rd;
    int rd_offset = -1;
    if (!inst->rd.is_phys()) {
        rd_offset = get_stack_offset(inst->rd);
        phys_rd = get_temp_reg(0);
    }

    // 生成立即数算术指令
    auto arith_imm_inst = ASM::ArithImm::create(phys_rd, phys_rs1, inst->imm, inst->op);
    result.push_back(arith_imm_inst);

    // 如果 rd 是虚拟寄存器，存回栈
    if (!inst->rd.is_phys()) {
        auto store_inst = ASM::Store::create(ASM::Reg::sp, phys_rd, rd_offset);
        result.push_back(store_inst);
    }

    return result;
}

ASM::Code RegAllocator::allocateMv(ASM::MvPtr &inst,
                                   std::set<ASM::Reg> &available_regs,
                                   ASM::RegMap &reg_map,
                                   FunctionPtr &func) {
    ASM::Code result;

    auto get_stack_offset = [&](const ASM::Reg &reg) -> int {
        if (reg.is_phys()) return -1;
        return func->alloc_temp(4, reg);
    };

    auto get_temp_reg = [&](int index) -> ASM::Reg {
        static const ASM::Reg temp_regs[] = {ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2};
        return temp_regs[index % 3];
    };

    // 处理 rs
    ASM::Reg phys_rs = inst->rs;
    if (!inst->rs.is_phys()) {
        int offset = get_stack_offset(inst->rs);
        phys_rs = get_temp_reg(1);
        auto load_inst = ASM::Load::create(phys_rs, ASM::Reg::sp, offset);
        result.push_back(load_inst);
    }

    // 处理 rd
    ASM::Reg phys_rd = inst->rd;
    int rd_offset = -1;
    if (!inst->rd.is_phys()) {
        rd_offset = get_stack_offset(inst->rd);
        phys_rd = get_temp_reg(0);
    }

    // 生成 mv 指令
    auto mv_inst = ASM::Mv::create(phys_rd, phys_rs);
    result.push_back(mv_inst);

    // 如果 rd 是虚拟寄存器，存回栈
    if (!inst->rd.is_phys()) {
        auto store_inst = ASM::Store::create(ASM::Reg::sp, phys_rd, rd_offset);
        result.push_back(store_inst);
    }

    return result;
}

ASM::Code RegAllocator::allocateLi(ASM::LiPtr &inst,
                                   std::set<ASM::Reg> &available_regs,
                                   ASM::RegMap &reg_map,
                                   FunctionPtr &func) {
    ASM::Code result;

    auto get_stack_offset = [&](const ASM::Reg &reg) -> int {
        if (reg.is_phys()) return -1;
        return func->alloc_temp(4, reg);
    };

    auto get_temp_reg = [&](int index) -> ASM::Reg {
        static const ASM::Reg temp_regs[] = {ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2};
        return temp_regs[index % 3];
    };

    // 处理 rd
    ASM::Reg phys_rd = inst->rd;
    int rd_offset = -1;
    if (!inst->rd.is_phys()) {
        rd_offset = get_stack_offset(inst->rd);
        phys_rd = get_temp_reg(0);
    }

    // 生成 li 指令
    auto li_inst = ASM::Li::create(phys_rd, inst->imm);
    result.push_back(li_inst);

    // 如果 rd 是虚拟寄存器，存回栈
    if (!inst->rd.is_phys()) {
        auto store_inst = ASM::Store::create(ASM::Reg::sp, phys_rd, rd_offset);
        result.push_back(store_inst);
    }

    return result;
}

ASM::Code RegAllocator::allocateLa(ASM::LaPtr &inst,
                                   std::set<ASM::Reg> &available_regs,
                                   ASM::RegMap &reg_map,
                                   FunctionPtr &func) {
    ASM::Code result;

    auto get_stack_offset = [&](const ASM::Reg &reg) -> int {
        if (reg.is_phys()) return -1;
        return func->alloc_temp(4, reg);
    };

    auto get_temp_reg = [&](int index) -> ASM::Reg {
        static const ASM::Reg temp_regs[] = {ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2};
        return temp_regs[index % 3];
    };

    // 处理 rd
    ASM::Reg phys_rd = inst->rd;
    int rd_offset = -1;
    if (!inst->rd.is_phys()) {
        rd_offset = get_stack_offset(inst->rd);
        phys_rd = get_temp_reg(0);
    }

    // 生成 la 指令
    auto la_inst = ASM::La::create(phys_rd, inst->label);
    result.push_back(la_inst);

    // 如果 rd 是虚拟寄存器，存回栈
    if (!inst->rd.is_phys()) {
        auto store_inst = ASM::Store::create(ASM::Reg::sp, phys_rd, rd_offset);
        result.push_back(store_inst);
    }

    return result;
}

ASM::Code RegAllocator::allocateLoad(ASM::LoadPtr &inst,
                                     std::set<ASM::Reg> &available_regs,
                                     ASM::RegMap &reg_map,
                                     FunctionPtr &func) {
    ASM::Code result;

    auto get_stack_offset = [&](const ASM::Reg &reg) -> int {
        if (reg.is_phys()) return -1;
        return func->alloc_temp(4, reg);
    };

    auto get_temp_reg = [&](int index) -> ASM::Reg {
        static const ASM::Reg temp_regs[] = {ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2};
        return temp_regs[index % 3];
    };

    // 处理 rs1 (基址寄存器)
    ASM::Reg phys_rs1 = inst->rs1;
    if (!inst->rs1.is_phys()) {
        int offset = get_stack_offset(inst->rs1);
        phys_rs1 = get_temp_reg(1);
        auto load_base_inst = ASM::Load::create(phys_rs1, ASM::Reg::sp, offset);
        result.push_back(load_base_inst);
    }

    // 处理 rd
    ASM::Reg phys_rd = inst->rd;
    int rd_offset = -1;
    if (!inst->rd.is_phys()) {
        rd_offset = get_stack_offset(inst->rd);
        phys_rd = get_temp_reg(0);
    }

    // 生成 load 指令
    auto load_inst = ASM::Load::create(phys_rd, phys_rs1, inst->offset);
    result.push_back(load_inst);

    // 如果 rd 是虚拟寄存器，存回栈
    if (!inst->rd.is_phys()) {
        auto store_inst = ASM::Store::create(ASM::Reg::sp, phys_rd, rd_offset);
        result.push_back(store_inst);
    }

    return result;
}

ASM::Code RegAllocator::allocateStore(ASM::StorePtr &inst,
                                      std::set<ASM::Reg> &available_regs,
                                      ASM::RegMap &reg_map,
                                      FunctionPtr &func) {
    ASM::Code result;

    auto get_stack_offset = [&](const ASM::Reg &reg) -> int {
        if (reg.is_phys()) return -1;
        return func->alloc_temp(4, reg);
    };

    auto get_temp_reg = [&](int index) -> ASM::Reg {
        static const ASM::Reg temp_regs[] = {ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2};
        return temp_regs[index % 3];
    };

    // 处理 rs1 (基址寄存器)
    ASM::Reg phys_rs1 = inst->rs1;
    if (!inst->rs1.is_phys()) {
        int offset = get_stack_offset(inst->rs1);
        phys_rs1 = get_temp_reg(1);
        auto load_base_inst = ASM::Load::create(phys_rs1, ASM::Reg::sp, offset);
        result.push_back(load_base_inst);
    }

    // 处理 rs2 (源数据寄存器)
    ASM::Reg phys_rs2 = inst->rs2;
    if (!inst->rs2.is_phys()) {
        int offset = get_stack_offset(inst->rs2);
        phys_rs2 = get_temp_reg(2);
        auto load_src_inst = ASM::Load::create(phys_rs2, ASM::Reg::sp, offset);
        result.push_back(load_src_inst);
    }

    // 生成 store 指令
    auto store_inst = ASM::Store::create(phys_rs1, phys_rs2, inst->offset);
    result.push_back(store_inst);

    return result;
}

// 对于不涉及寄存器分配的指令，直接返回原指令
ASM::Code RegAllocator::allocateLabel(ASM::LabelPtr &inst,
                                      std::set<ASM::Reg> &available_regs,
                                      ASM::RegMap &reg_map,
                                      FunctionPtr &func) {
    return {inst};
}

ASM::Code RegAllocator::allocateFunction(ASM::FunctionPtr &inst,
                                         std::set<ASM::Reg> &available_regs,
                                         ASM::RegMap &reg_map,
                                         FunctionPtr &func) {
    return {inst};
}

ASM::Code RegAllocator::allocateCall(ASM::CallPtr &inst,
                                     std::set<ASM::Reg> &available_regs,
                                     ASM::RegMap &reg_map,
                                     FunctionPtr &func) {
    // Call 指令涉及参数寄存器，但这里使用朴素分配，直接返回
    return {inst};
}

ASM::Code RegAllocator::allocateJump(ASM::JumpPtr &inst,
                                     std::set<ASM::Reg> &available_regs,
                                     ASM::RegMap &reg_map,
                                     FunctionPtr &func) {
    return {inst};
}

ASM::Code RegAllocator::allocateRet(ASM::RetPtr &inst,
                                    std::set<ASM::Reg> &available_regs,
                                    ASM::RegMap &reg_map,
                                    FunctionPtr &func) {
    return {inst};
}

ASM::Code RegAllocator::allocateZero(ASM::ZeroPtr &inst,
                                     std::set<ASM::Reg> &available_regs,
                                     ASM::RegMap &reg_map,
                                     FunctionPtr &func) {
    return {inst};
}

ASM::Code RegAllocator::allocateWord(ASM::WordPtr &inst,
                                     std::set<ASM::Reg> &available_regs,
                                     ASM::RegMap &reg_map,
                                     FunctionPtr &func) {
    return {inst};
}

ASM::Code RegAllocator::allocateGlobalLabel(ASM::GlobalLabelPtr &inst,
                                            std::set<ASM::Reg> &available_regs,
                                            ASM::RegMap &reg_map,
                                            FunctionPtr &func) {
    return {inst};
}

// 添加 Branch 指令的分配函数
ASM::Code RegAllocator::allocateBranch(ASM::BranchPtr &inst,
                                       std::set<ASM::Reg> &available_regs,
                                       ASM::RegMap &reg_map,
                                       FunctionPtr &func) {
    ASM::Code result;

    auto get_stack_offset = [&](const ASM::Reg &reg) -> int {
        if (reg.is_phys()) return -1;
        return func->alloc_temp(4, reg);
    };

    auto get_temp_reg = [&](int index) -> ASM::Reg {
        static const ASM::Reg temp_regs[] = {ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2};
        return temp_regs[index % 3];
    };

    // 处理 rs1
    ASM::Reg phys_rs1 = inst->rs1;
    if (!inst->rs1.is_phys()) {
        int offset = get_stack_offset(inst->rs1);
        phys_rs1 = get_temp_reg(1);
        auto load_inst = ASM::Load::create(phys_rs1, ASM::Reg::sp, offset);
        result.push_back(load_inst);
    }

    // 处理 rs2
    ASM::Reg phys_rs2 = inst->rs2;
    if (!inst->rs2.is_phys()) {
        int offset = get_stack_offset(inst->rs2);
        phys_rs2 = get_temp_reg(2);
        auto load_inst = ASM::Load::create(phys_rs2, ASM::Reg::sp, offset);
        result.push_back(load_inst);
    }

    // 生成 branch 指令
    auto branch_inst = ASM::Branch::create(phys_rs1, phys_rs2, inst->label, inst->op);
    result.push_back(branch_inst);

    return result;
}
