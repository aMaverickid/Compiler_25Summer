#include "inst_selector.hpp"

#include <cassert>
#include <memory>

#include "codegen/asm_emitter.hpp"
#include <algorithm>

BinaryOp convertToBinaryOp(ASM::Arith::Op a) {
    return static_cast<BinaryOp>(static_cast<int>(a));
}

void InstSelector::select(Module &mod) {
    for (auto &func : mod.functions) {
        select(func);
    }
}

void InstSelector::select(FunctionPtr &func) {
    // 设置当前函数
    // 如果是 DEC，需要调用当前函数中的 alloc_temp，在栈上分配空间
    current_func = func;
    for (auto &block : func->blocks) {
        block->asm_code = select(block->ir_code);
    }
}

ASM::Code InstSelector::select(const IR::Code &ir_code) {
    ASM::Code asm_code;
    for (const auto &node : ir_code) {
        auto code = select(node);
        asm_code.insert(asm_code.end(), code.begin(), code.end());
    }
    return asm_code;
}

ASM::Code InstSelector::select(const IR::NodePtr &node) {
#define SELECT_NODE(type)                                     \
    if (auto p = std::dynamic_pointer_cast<IR::type>(node)) { \
        return select##type(p);                               \
    }

    // 对于每种不同类型的 IR 节点，调用相应的 select 函数
    // 如果你添加了新的 IR 节点类型，记得在这里添加对应的 select 函数
    SELECT_NODE(LoadAddr)
    SELECT_NODE(Dec)
    SELECT_NODE(Store)
    SELECT_NODE(Load)
    SELECT_NODE(Param)
    SELECT_NODE(LoadImm)
    SELECT_NODE(Assign)
    SELECT_NODE(Binary)
    SELECT_NODE(Unary)
    SELECT_NODE(Label)
    SELECT_NODE(Goto)
    SELECT_NODE(Function)
    SELECT_NODE(Call)
    SELECT_NODE(Arg)
    SELECT_NODE(Return)
    SELECT_NODE(If)
    SELECT_NODE(Global)

#warning Add more IR node types if needed

    assert(false && "Unknown IR node type");
}

ASM::Code InstSelector::selectLoadImm(const IR::LoadImmPtr &node) {
    ASM::Code code;
    // a = #t	-> li reg(a), t
    code.push_back(ASM::Li::create(ASM::Reg(node->x), node->k));
    return code;
}

ASM::Code InstSelector::selectAssign(const IR::AssignPtr &node) {
    ASM::Code code;
    // a = b	-> mv reg(a), reg(b)
    code.push_back(ASM::Mv::create(ASM::Reg(node->x), ASM::Reg(node->y)));
    return code;
}

ASM::Code InstSelector::selectBinary(const IR::BinaryPtr &node) {
    ASM::Code code;
    // a = b + c	-> add reg(a), reg(b), reg(c)

    code.push_back(ASM::Arith::create(
        ASM::Reg(node->x), ASM::Reg(node->y), ASM::Reg(node->z), static_cast<ASM::Arith::Op>(node->op)));
    return code;
}

ASM::Code InstSelector::selectUnary(const IR::UnaryPtr &node) {
    ASM::Code code;
    // a = -b	-> sub reg(a), zero, reg(b)

    code.push_back(ASM::Arith::create(
        ASM::Reg(node->x), ASM::Reg::zero, ASM::Reg(node->y), static_cast<ASM::Arith::Op>(node->op)));
    return code;
}

ASM::Code InstSelector::selectLabel(const IR::LabelPtr &node) {
    ASM::Code code;
    // LABEL label:	-> label:

    code.push_back(ASM::Label::create(node->label));
    return code;
}

ASM::Code InstSelector::selectGoto(const IR::GotoPtr &node) {
    ASM::Code code;
    // GOTO label	-> j label

    code.push_back(ASM::Jump::create(node->label));
    return code;
}

ASM::Code InstSelector::selectFunction(const IR::FunctionPtr &node) {
    ASM::Code code;
    // FUNCTION func:	-> func:
    // FUNCTION func:	-> func:

    // 函数标签
    code.push_back(ASM::Function::create(node->name));

    // Prologue - 函数入口栈帧设置
    // 1. 调整栈指针，为栈帧分配空间（这里先分配一个基本大小，后续会调整）
    code.push_back(ASM::ArithImm::create(ASM::Reg::sp, ASM::Reg::sp, -16, ASM::ArithImm::Op::Addi));

    // 2. 保存返回地址和帧指针
    code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::ra, 12));
    code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg::fp, 8));

    // 3. 设置帧指针
    code.push_back(ASM::ArithImm::create(ASM::Reg::fp, ASM::Reg::sp, 16, ASM::ArithImm::Op::Addi));

    return code;
}

// 修改 selectCall 以处理调用者的栈帧管理
ASM::Code InstSelector::selectCall(const IR::CallPtr &node) {
    ASM::Code code;

    // 调用者需要保存临时寄存器（如果使用了的话）
    // 这里保存 t0-t2（根据实际使用情况可以优化）
    std::vector<ASM::Reg> caller_saved_regs = {ASM::Reg::t0, ASM::Reg::t1, ASM::Reg::t2};
    int saved_count = 0;

    for (const auto &reg : caller_saved_regs) {
        // 为每个需要保存的寄存器分配栈空间
        int offset = current_func->alloc_temp(4, reg);
        code.push_back(ASM::Store::create(ASM::Reg::sp, reg, offset));
        saved_count++;
    }

    // 调用函数
    code.push_back(ASM::Call::create(node->func));

    // 如果有返回值，将其从 a0 移动到目标寄存器
    if (!node->x.empty()) {
        code.push_back(ASM::Mv::create(ASM::Reg(node->x), ASM::Reg::a0));
    }

    // 恢复临时寄存器
    for (const auto &reg : caller_saved_regs) {
        int offset = current_func->temp_offset[reg]; // 获取之前分配的偏移
        code.push_back(ASM::Load::create(reg, ASM::Reg::sp, offset));
    }

    return code;
}

ASM::Code InstSelector::selectArg(const IR::ArgPtr &node) {
    ASM::Code code;

    if (node->k < 8) {
        // 前8个参数放入寄存器 a0-a7
        code.push_back(ASM::Mv::create(ASM::Reg("a" + std::to_string(node->k)), ASM::Reg(node->x)));
    } else {
        // 超过8个参数的部分需要存储到调用者的栈帧中
        // 这需要在调用前为参数分配栈空间
        int offset = current_func->alloc_temp(4, ASM::Reg(node->x + "_arg"));
        code.push_back(ASM::Store::create(ASM::Reg::sp, ASM::Reg(node->x), offset));
    }

    return code;
}

// 添加 Param 指令的处理
ASM::Code InstSelector::selectParam(const IR::ParamPtr &node) {
    ASM::Code code;
    // PARAM x -> mv reg(x), ak
    // 从参数寄存器中读取参数到虚拟寄存器

    if (node->k < 8) {
        // 前8个参数从寄存器 a0-a7 中读取
        code.push_back(ASM::Mv::create(ASM::Reg(node->x), ASM::Reg("a" + std::to_string(node->k))));
    } else {
        // 超过8个参数的部分从栈中读取
        int offset = 4 * (node->k - 8); // 每个参数4字节
        code.push_back(ASM::Load::create(ASM::Reg(node->x), ASM::Reg::fp, offset));
    }

    return code;
}

// 修改 selectReturn 以处理函数返回
ASM::Code InstSelector::selectReturn(const IR::ReturnPtr &node) {
    ASM::Code code;

    // // 如果有返回值，将其移动到 a0
    // if (!node->value.empty()) {
    //     code.push_back(ASM::Mv::create(ASM::Reg::a0, ASM::Reg(node->value)));
    // }

    // // Epilogue - 函数退出栈帧清理
    // // 1. 恢复返回地址和帧指针
    // code.push_back(ASM::Load::create(ASM::Reg::ra, ASM::Reg::sp, 12));
    // code.push_back(ASM::Load::create(ASM::Reg::fp, ASM::Reg::sp, 8));

    // // 2. 调整栈指针，释放栈帧空间
    // code.push_back(ASM::ArithImm::create(ASM::Reg::sp, ASM::Reg::sp, 16, ASM::ArithImm::Op::Addi));

    // // 3. 返回
    // code.push_back(ASM::Ret::create());

    return code;
}

ASM::Code InstSelector::selectIf(const IR::IfPtr &node) {
    ASM::Code code;
    // IF x op y GOTO label -> branch_op reg(x), reg(y), label

    switch (node->op) {
    case BinaryOp::Gt:
        code.push_back(ASM::Branch::create(ASM::Reg(node->t1), ASM::Reg(node->t2), node->label, ASM::Branch::Op::Bgt));
        break;
    case BinaryOp::Lt:
        code.push_back(ASM::Branch::create(ASM::Reg(node->t1), ASM::Reg(node->t2), node->label, ASM::Branch::Op::Blt));
        break;
    case BinaryOp::Ge:
        code.push_back(ASM::Branch::create(ASM::Reg(node->t1), ASM::Reg(node->t2), node->label, ASM::Branch::Op::Bge));
        break;
    case BinaryOp::Le:
        code.push_back(ASM::Branch::create(ASM::Reg(node->t1), ASM::Reg(node->t2), node->label, ASM::Branch::Op::Ble));
        break;
    case BinaryOp::Eq:
        code.push_back(ASM::Branch::create(ASM::Reg(node->t1), ASM::Reg(node->t2), node->label, ASM::Branch::Op::Beq));
        break;
    case BinaryOp::Ne:
        code.push_back(ASM::Branch::create(ASM::Reg(node->t1), ASM::Reg(node->t2), node->label, ASM::Branch::Op::Bne));
        break;
    default:
        assert(false && "Unsupported branch operation");
    }

    return code;
}

ASM::Code InstSelector::selectGlobal(const IR::GlobalPtr &node) {
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

ASM::Code InstSelector::selectDec(const IR::DecPtr &node) {
    ASM::Code code;
    // DEC x #k -> 在栈上分配 k 个字节，将起始地址存入 x

    int offset = current_func->alloc_dec(node->size);

    // 计算栈上地址：sp + offset
    // addi reg(x), sp, offset
    code.push_back(ASM::ArithImm::create(ASM::Reg(node->name), ASM::Reg::sp, offset, ASM::ArithImm::Op::Addi));

    return code;
}

ASM::Code InstSelector::selectLoadAddr(const IR::LoadAddrPtr &node) {
    ASM::Code code;
    // x = &y -> la reg(x), y

    code.push_back(ASM::La::create(ASM::Reg(node->x), node->label));

    return code;
}

ASM::Code InstSelector::selectStore(const IR::StorePtr &node) {
    ASM::Code code;
    // *x = y -> sw reg(y), 0(reg(x))
    // *(x + #k) = y -> sw reg(y), k(reg(x))

    // 检查偏移量是否在12位立即数范围内 (-2048 到 2047)
    if (node->offset >= -2048 && node->offset <= 2047) {
        code.push_back(ASM::Store::create(ASM::Reg(node->value), ASM::Reg(node->addr), node->offset));
    } else {
        // 偏移量超出范围，需要先计算地址
        ASM::Reg temp_reg = ASM::Reg::t0; // 使用一个临时寄存器
        code.push_back(ASM::Li::create(ASM::Reg(temp_reg), node->offset));
        code.push_back(ASM::Arith::create(ASM::Reg(temp_reg), ASM::Reg(node->addr), ASM::Reg(temp_reg), ASM::Arith::Op::Add));
        code.push_back(ASM::Store::create(ASM::Reg(node->value), ASM::Reg(temp_reg), 0));
    }

    return code;
}

ASM::Code InstSelector::selectLoad(const IR::LoadPtr &node) {
    ASM::Code code;
    // x = *y -> lw reg(x), 0(reg(y))
    // x = *(y + #k) -> lw reg(x), k(reg(y))

    // 检查偏移量是否在12位立即数范围内 (-2048 到 2047)
    if (node->offset >= -2048 && node->offset <= 2047) {
        code.push_back(ASM::Load::create(ASM::Reg(node->x), ASM::Reg(node->addr), node->offset));
    } else {
        // 偏移量超出范围，需要先计算地址
        ASM::Reg temp_reg = ASM::Reg::t0; // 使用一个临时寄存器
        code.push_back(ASM::Li::create(ASM::Reg(temp_reg), node->offset));
        code.push_back(ASM::Arith::create(ASM::Reg(temp_reg), ASM::Reg(node->addr), ASM::Reg(temp_reg), ASM::Arith::Op::Add));
        code.push_back(ASM::Load::create(ASM::Reg(node->x), ASM::Reg(temp_reg), 0));
    }

    return code;
}