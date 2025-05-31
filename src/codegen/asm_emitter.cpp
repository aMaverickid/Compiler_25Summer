#include "asm_emitter.hpp"
#include <algorithm>

void ASMEmitter::emit(const Module &mod) {
    // 添加 Venus 的 read 和 write 系统调用
    if (use_venus) {
        output << R"(
    .text
    .globl read
read:
    li a0, 6
    ecall
    ret

    .globl write
write:
    mv a1, a0
    li a0, 1
    ecall
    ret

)";
    }

    // 添加 .data 段
    output << "    " << ".data" << std::endl;

    for (const auto &global : mod.globals) {
        emit(global);
    }

    // 添加 .text 段
    output << std::endl
           << "    " << ".text" << std::endl;
    for (const auto &func : mod.functions) {
        emit(func);
    }
}

void ASMEmitter::emit(const FunctionPtr &func) {
    current_func = func; // 设置当前函数
    int stack_size = func->temp_stack_size + func->reg_stack_size;
    reg_map = func->reg_map; // 设置当前函数的寄存器映射

// 添加 prologue，处理 sp, ra, fp 等寄存器
#warning Not implemented: ASMEmitter::emit prologue

    // 为了方便 emit epilogue，这里忽略 exit block，也就是最后一个 block
    for (size_t i = 0; i < func->blocks.size() - 1; i++) {
        emit(func->blocks[i]);
    }

    // 添加 epilogue，处理 sp, ra, fp 等寄存器
    emitEpilogue(func->blocks.back());
}

void ASMEmitter::emit(const IR::GlobalPtr &global) {
    auto code = selectGlobal(global);
    for (const auto &inst : code) {
        emit(inst);
    }
}

void ASMEmitter::emit(const BasicBlockPtr &block) {
    // insert epilogue for the last block
    for (const auto &inst : block->asm_code) {
        emit(inst);
    }
}

void ASMEmitter::emitEpilogue(const BasicBlockPtr &block) {
    // label
    output << block->label << ":" << std::endl;
    ASM::Code code;
    auto node = std::dynamic_pointer_cast<IR::Return>(block->ir_code.back());
    if (!node->x.empty()) {
        code.push_back(ASM::Mv::create(ASM::Reg::a0, ASM::Reg(node->x)));
    }

    // Epilogue - 函数退出栈帧清理
    // 1. 恢复返回地址和帧指针

    code.push_back(ASM::Load::create(ASM::Reg::ra, ASM::Reg::sp, ra_offset));
    code.push_back(ASM::Load::create(ASM::Reg::fp, ASM::Reg::sp, fp_offset));

    // 2. 调整栈指针，释放栈帧空间
    int stack_size = current_func->temp_stack_size + current_func->reg_stack_size;
    code.push_back(ASM::ArithImm::create(ASM::Reg::sp, ASM::Reg::sp, stack_size, ASM::ArithImm::Op::Addi));

    // 3. 返回
    code.push_back(ASM::Ret::create());

    // 输出 epilogue 指令
    for (const auto &inst : code) {
        output << "    " << inst->to_string() << std::endl;
    }
}

void ASMEmitter::emit(const ASM::InstPtr &inst) {
    inst->replace_all(reg_map);
    if (type_of<ASM::Label>(inst) || type_of<ASM::Function>(inst)) {
        output << inst->to_string() << std::endl;
        if (type_of<ASM::Function>(inst)) {
            // Prologue - 函数入口栈帧设置
            fp_offset = current_func->alloc_temp(4, ASM::Reg::fp); // 为帧指针分配空间
            ra_offset = current_func->alloc_temp(4, ASM::Reg::ra); // 为返回地址分配空间

            // 1. 调整栈指针，为栈帧分配空间
            int stack_size = current_func->temp_stack_size + current_func->reg_stack_size;
            ASM::Code code;
            code.push_back(ASM::ArithImm::create(ASM::Reg::sp, ASM::Reg::sp, -stack_size, ASM::ArithImm::Op::Addi));

            // 2. 保存返回地址和帧指针
            code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::ra, ra_offset)); // ra 保存到 sp + stack_size - 4
            code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::fp, fp_offset)); // fp 保存到 sp + stack_size - 8

            // 3. 设置帧指针
            code.push_back(ASM::ArithImm::create(ASM::Reg::fp, ASM::Reg::sp, stack_size, ASM::ArithImm::Op::Addi));

            // 输出 prologue 指令
            for (const auto &p : code) {
                output << "    " << p->to_string() << std::endl;
            }
        }
    } else {
        output << "    " << inst->to_string() << std::endl;
    }
}

ASM::Code ASMEmitter::selectGlobal(const IR::GlobalPtr &node) {
    ASM::Code code;
    // GLOBAL x: #k -> x:, .zero k
    // GLOBAL x: #k = #v1, #v2, ... -> x:, .word v1, v2, ...

    code.push_back(ASM::Label::create(node->name));

    if (node->values.empty() || std::all_of(node->values.begin(), node->values.end(), [](int v) { return v == 0; })) {
        // 全零初始化，使用 .zero 指令
        code.push_back(ASM::Zero::create(node->size));
    } else {
        // 有初始值，使用 .word 指令
        for (int value : node->values) {
            code.push_back(ASM::Word::create(value));
        }
    }

    return code;
}