#include "control_flow.hpp"

#include "common.hpp"

int Function::alloc_temp(int size, ASM::Reg reg) {
  // 为临时变量分配空间，临时变量通常分配在靠近 sp 的位置
  // 返回值是相对于 sp 的偏移

  if (size <= 0) {
    throw std::invalid_argument("Size must be positive");
  }
  
  if (temp_offset.find(reg) != temp_offset.end()) {
    return temp_offset[reg];
  }

  int offset = temp_stack_size;
  temp_stack_size += size;
  temp_offset[reg] = offset;
  
  return offset;
}

int Function::alloc_reg(int size, ASM::Reg reg) {
  // 为保存寄存器分配空间，保存寄存器通常分配在靠近 fp 的位置
  // 返回值是相对于 fp 的偏移

  if (size <= 0) {
    throw std::invalid_argument("Size must be positive");
  }

  if (reg_offset.find(reg) != reg_offset.end()) {
    return reg_offset[reg];
  }

  int offset = -reg_stack_size; // 使用负数偏移量表示相对于 fp 的位置
  reg_stack_size += size;
  reg_offset[reg] = offset;

  return offset;
}

int Function::alloc_dec(int size) {
  // 为声明的变量分配空间，通常分配在靠近 sp 的位置
  // 返回值是相对于 sp 的偏移

  if (size <= 0) {
    throw std::invalid_argument("Size must be positive");
  }

  int offset = temp_stack_size;
  temp_stack_size += size;

  return offset;
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