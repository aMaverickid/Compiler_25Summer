#include "control_flow.hpp"

#include "common.hpp"

int Function::alloc_temp(int size) {
  // 为临时变量分配空间，临时变量通常分配在靠近 sp 的位置
  // 返回值是相对于 sp 的偏移

#warning Not implemented: Function::alloc_temp
}

int Function::alloc_reg(int size) {
  // 为保存寄存器分配空间，保存寄存器通常分配在靠近 fp 的位置
  // 返回值是相对于 fp 的偏移

#warning Not implemented: Function::alloc_reg
}

IR::Code Module::get_ir() const {
  IR::Code code;
  for (const auto &global : globals) {
    code.push_back(global);
  }
  for (const auto &func : functions) {
    for (const auto &block : func->blocks) {
      std::copy(block->ir_code.begin(), block->ir_code.end(),
                std::back_inserter(code));
    }
  }
  return code;
}