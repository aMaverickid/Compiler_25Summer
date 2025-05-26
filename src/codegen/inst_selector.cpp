#include "inst_selector.hpp"

#include <cassert>
#include <memory>

#include "codegen/asm_emitter.hpp"

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
#define SELECT_NODE(type)                                   \
  if (auto p = std::dynamic_pointer_cast<IR::type>(node)) { \
    return select##type(p);                                 \
  }

  // 对于每种不同类型的 IR 节点，调用相应的 select 函数
  // 如果你添加了新的 IR 节点类型，记得在这里添加对应的 select 函数
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

  code.push_back(ASM::Function::create(node->name));
  return code;
}

ASM::Code InstSelector::selectCall(const IR::CallPtr &node) {
  ASM::Code code;
  // CALL func	-> call func
  // x = CALL func	-> call func; mv reg(x), a0

  code.push_back(ASM::Call::create(node->func));
  if (!node->x.empty()) {
    code.push_back(ASM::Mv::create(ASM::Reg(node->x), ASM::Reg::a0));    
  }
  return code;
}

ASM::Code InstSelector::selectArg(const IR::ArgPtr &node) {
  ASM::Code code;
  // ARG x	-> mv ak, reg(x)
  // k is the index of the argument

  code.push_back(ASM::Mv::create(ASM::Reg("a" + std::to_string(node->k)), ASM::Reg(node->x)));
  return code;
}

ASM::Code InstSelector::selectReturn(const IR::ReturnPtr &node) {
  ASM::Code code;
  // 已经在 cfg builder 中统一为一个 exit call
  // 因此只有 exit block 里有 return 语句
  // 在 asm emitter 中对每个函数处理时
  // 会忽略 exit block 并添加 epilogue
  // 因此这里不需要处理

#warning Not implemented: InstSelector::selectReturn
  return code;
}
