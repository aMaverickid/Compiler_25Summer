#include "symbol_table.hpp"

SymbolPtr SymbolTable::add_symbol(std::string name, TypePtr type) {
  // 实现符号表的插入操作
  // 并设置 symbol 的 unique_name 属性（你也可以等到 IR Translation 阶段再设置）
  // 对于局部变量和数组，最好为该标识符重新生成一个唯一名称
  // 对于全局变量和函数，直接使用原名称即可
  // 最后，如果插入成功，返回新的符号
  // 如果符号已经存在，返回 nullptr

// #warning Not implemented: SymbolTable::add_symbol
  if (scopes.empty()) enter_scope();
  auto& current_scope = scopes.back();
  if (current_scope.find(name) != current_scope.end()) {
    return nullptr;  // Symbol already exists
  }
  SymbolPtr symbol = Symbol::create(name, type);
  symbol->unique_name = name + "@" + std::to_string(scope_depth);
  current_scope[name] = symbol;
  return symbol;
}

SymbolPtr SymbolTable::find_symbol(std::string name,
                                   bool in_current_scope) const {
  // 实现符号表的查找操作
  // 找到了返回对应的符号，否则返回 nullptr
  // in_current_scope 为 true 时，只在当前作用域查找

// #warning Not implemented: SymbolTable::find_symbol
  if (scopes.empty()) return nullptr;
  if (in_current_scope) {
    auto& current_scope = scopes.back();
    auto it = current_scope.find(name);
    if (it != current_scope.end()) {
      return it->second;
    }    
  }
  else {
    for (int i = scopes.size() - 1; i >= 0; --i) {
      auto& scope = scopes[i];
      auto it = scope.find(name);
      if (it != scope.end()) {
        return it->second;
      }
    }
  }
  return nullptr;
}

void SymbolTable::enter_scope() {
  // 实现符号表的进入作用域操作
  // 需要创建一个新的作用域
  scopes.emplace_back();
  ++ scope_depth;
// #warning Not implemented: SymbolTable::enter_scope
}

void SymbolTable::exit_scope() {
  // 实现符号表的退出作用域操作
  // 需要删除当前作用域中的所有符号
  if (!scopes.empty()) {
    scopes.pop_back();
    -- scope_depth;
  }
// #warning Not implemented: SymbolTable::exit_scope
}
