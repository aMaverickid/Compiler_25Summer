#include "type_checker.hpp"

#include "common.hpp"

TypeChecker::TypeChecker() {
  // 你需要在这里对 symbol_table 进行初始化
  // 插入一些内置函数，如 read 和 write
  symbol_table.enter_scope();
  std::vector<TypePtr> read_params;
  read_params.push_back(PrimitiveType::Int);
  auto read_func = FuncType::create(PrimitiveType::Void, read_params);
  symbol_table.add_symbol("read", read_func);
  std::vector<TypePtr> write_params;
  write_params.push_back(PrimitiveType::Int);
  auto write_func = FuncType::create(PrimitiveType::Void, write_params);
  symbol_table.add_symbol("write", write_func);

}

TypePtr TypeChecker::check(AST::NodePtr node) {
#define CHECK_NODE(type)                                     \
  if (auto n = std::dynamic_pointer_cast<AST::type>(node)) { \
    return check##type(n);                                   \
  }

  // 递归检查 AST 的每个节点
  // 如果你添加了新的 AST 节点类型，记得在这里添加对应的检查函数
  CHECK_NODE(CompUnit)
  CHECK_NODE(FuncDef)
  CHECK_NODE(VarDecl)
  CHECK_NODE(Block)
  CHECK_NODE(AssignStmt)
  CHECK_NODE(ReturnStmt)
  CHECK_NODE(LVal)
  CHECK_NODE(IntConst)
  CHECK_NODE(FuncCall)
  CHECK_NODE(UnaryExp)
  CHECK_NODE(BinaryExp)
  CHECK_NODE(IfStmt)
  CHECK_NODE(WhileStmt)
  CHECK_NODE(EmptyStmt)

#warning Add more AST node types if needed

#undef CHECK_NODE

  ASSERT(false, "Unknown AST node type " + node->to_string() +
                    " in type checking at line " +
                    std::to_string(node->lineno));
}

TypePtr TypeChecker::checkCompUnit(AST::CompUnitPtr node) {
  for (auto &unit : node->units) {
    check(unit);
  }
  return nullptr;
}

TypePtr TypeChecker::checkFuncDef(AST::FuncDefPtr node) {
  // 在这个函数中，你需要判断函数是否已经被定义过
  // 如果函数已经被定义过，你需要报错
  // 否则，你需要将函数插入符号表，并在符号表中创建一个新的作用域
  // 再将函数参数也插入符号表，并将符号表中对应的 symbol 挂到 FuncDef 节点上
  // 最后检查函数体的语句块

  // 检查函数是否已经被定义过
  if (symbol_table.find_symbol(node->name)) {
    ASSERT(false, "Function " + node->name + " is already defined");
  }
  // 将函数插入符号表并挂载到 FuncDef 节点上
  std::vector<TypePtr> param_types;
  auto return_type = PrimitiveType::create(node->return_btype);  
  // std::cout << "return type: " << return_type->to_string() << std::endl;
  // std:: cout << "param type: " << param_types.size() << std::endl;
  if (node->params && !node->params->params.empty()) {
    // 函数参数不为空    
    for (auto param : node->params->params) {
      if (param->dim.empty()) {
        param_types.push_back(PrimitiveType::create(BasicType::Int));
      }
      else {
        // 数组类型
        auto array_type = ArrayType::create(PrimitiveType::create(BasicType::Int), param->dim);
        param_types.push_back(array_type);
      }
    }
  }
  auto func_type = FuncType::create(return_type, param_types);
  node->symbol = symbol_table.add_symbol(node->name, func_type);

  // 创建新的作用域
  symbol_table.enter_scope();
  // 将函数参数插入新的作用域，为了比较返回类型
  symbol_table.add_symbol(node->name, func_type);
  // 将函数参数插入符号表
  if (node->params && !node->params->params.empty()) 
    for (auto param : node->params->params) {
      auto type = PrimitiveType::create(BasicType::Int);
      // 如果是数组类型
      if (!param->dim.empty()) {
        auto array_type = ArrayType::create(type, param->dim);
        param->symbol = symbol_table.add_symbol(param->ident, array_type);
      }
      else {
        param->symbol = symbol_table.add_symbol(param->ident, type);
      }
    }
  
  // 检查函数体
  check(node->block);
  // 离开作用域
  symbol_table.exit_scope();
  return nullptr;
}

TypePtr TypeChecker::checkVarDecl(AST::VarDeclPtr node) {
  for (auto var_def : node->defs) {
    checkVarDef(var_def, node->btype);
  }
  return nullptr;
}

TypePtr TypeChecker::checkVarDef(AST::VarDefPtr node, BasicType var_type) {
  TypePtr type = PrimitiveType::create(var_type);
  ArrayTypePtr array_type = nullptr;
  if (node->dim.size() > 0) {
    // 数组类型
    array_type = ArrayType::create(type, node->dim);
    type = array_type;
  }
  // 判断变量是否已经被定义过
  if (symbol_table.find_symbol(node->ident)) {
    ASSERT(false, "Variable " + node->ident + " is already defined");
  }

  if (node->inits == nullptr) {
    // 没有初始化表达式，直接插入符号表
    node->symbol = symbol_table.add_symbol(node->ident, type);
    return nullptr;
  }

  // 如果有初始化表达式，你需要检查初始化表达式的类型是否和变量类型相同  
  checkInitVal(node->inits, array_type);
  
  

  // 将变量插入符号表，并将符号表中的 symbol 挂到 VarDef 节点上
  node->symbol = symbol_table.add_symbol(node->ident, type);
  return nullptr;
}

TypePtr TypeChecker::checkBlock(AST::BlockPtr node, bool new_scope) {
  // 检查块内的每个语句
  // 如果 new_scope 为 true
  // 你需要在进入和退出块时更新符号表，创建、销毁新的作用域
  if (new_scope) symbol_table.enter_scope();
  for (auto it : node->stmts) {
    check(it);
  }
  if (new_scope) symbol_table.exit_scope();
  return nullptr;
}

TypePtr TypeChecker::checkAssignStmt(AST::AssignStmtPtr node) {
  TypePtr lval_type = check(node->lval);
  TypePtr expr_type = check(node->exp);
  // 判断赋值号两边的类型是否相同
  // 我们实验中只支持 int 类型
  // 因此你需要判断 lval_type 和 expr_type 是否都为 int 类型

  if (!lval_type->equals(PrimitiveType::Int) || 
      !expr_type->equals(PrimitiveType::Int)) {
    ASSERT(false, "Assignment type mismatch at line " +
                      std::to_string(node->lineno));
  }
  return nullptr;
}

TypePtr TypeChecker::checkReturnStmt(AST::ReturnStmtPtr node) {
  TypePtr expr_type = check(node->exp);
  // 检查返回值的类型是否和函数的返回值类型一致
  // 你需要从当前作用域的符号表中获取函数的返回值类型
  // 如果函数没有返回值，你需要判断 expr_type 是否为 void
  // 否则，你需要判断 expr_type 是否和函数的返回值类型一致
  auto& scope = symbol_table.scopes[symbol_table.scope_depth];
  for (auto& it : scope) {  
      auto func_type = std::dynamic_pointer_cast<FuncType>(it.second->type);
      if (func_type) {
        if (!func_type->return_type->equals(expr_type)) {
          // std::cout << "func type: " << func_type->to_string() << std::endl;
          // std::cout << "expr type: " << expr_type->to_string() << std::endl;
          ASSERT(false, "Return type mismatch at line " +
                            std::to_string(node->lineno));            
        }
      }
  }
  return nullptr;
}

TypePtr TypeChecker::checkLVal(AST::LValPtr node) {
  // 你需要在这里查找符号表，判断变量是否被定义过
  // 根据符号表中的信息设置 LVal 的类型
  // 若变量未定义，你需要报错
  // 否则，将符号表中的 symbol 挂到 LVal 节点上
  // 如果 LVal 是数组，你还需要根据下标索引来设置 LVal 的类型

  // 你需要返回 LVal 的类型
  auto symbol = symbol_table.find_symbol(node->ident);
  if (!symbol) {
    ASSERT(false, "Variable " + node->ident + "at line " + std::to_string(node->lineno) + " is not defined");
  }

  if (auto type = std::dynamic_pointer_cast<ArrayType>(symbol->type)) {    
    // 数组类型
    std::vector<int> dims = type->dims;
    int diff = dims.size() - node->indexes.size();
    if (diff < 0) {
      ASSERT(false, "Array index out of range at line " +
                        std::to_string(node->lineno));
    }
    std::vector<int> new_dims;
    for (int i = 0; i < diff; i++) {
      new_dims.push_back(dims[i]);
    }
    if (new_dims.empty()) {
      node->symbol = symbol;
      return type->element_type;
    }
    else {
      // 数组类型
      auto new_type = ArrayType::create(type->element_type, new_dims);
      node->symbol = symbol;
      return new_type;
    }          
  }
    
  node->symbol = symbol;
  auto type = symbol->type;

  return type;
}

TypePtr TypeChecker::checkIntConst(AST::IntConstPtr node) {
  // 整数常量的类型是 int
  return PrimitiveType::Int;
}

TypePtr TypeChecker::checkFuncCall(AST::FuncCallPtr node) {
  // 首先需要查找函数是否被定义过
  // 然后需要判断函数调用的参数个数和类型是否和声明一致
  // 最后设置函数调用表达式的类型为函数的返回值类型
  // 并将函数的 symbol 挂到 FuncCall 节点上

  // 你需要返回函数调用表达式的类型
  auto symbol = symbol_table.find_symbol(node->name);
  if (!symbol) {
    ASSERT(false, "Function " + node->name + " is not defined");
  }
  node->symbol = symbol;
  auto func_type = std::dynamic_pointer_cast<FuncType>(symbol->type);
  if (!func_type) {
    ASSERT(false, "Function " + node->name + " is not a function");
  }

  return func_type->return_type;

}

TypePtr TypeChecker::checkUnaryExp(AST::UnaryExpPtr node) {
  auto type = check(node->exp);
  // 一元表达式只支持 int 类型，因此你需要判断 type 是否为 int

#warning Not implemented: TypeChecker::checkUnaryExp
  return PrimitiveType::Int;
}

TypePtr TypeChecker::checkBinaryExp(AST::BinaryExpPtr node) {
  TypePtr left_type = check(node->left);
  TypePtr right_type = check(node->right);
  // 二元表达式只支持 int 类型，因此你需要判断左右表达式的类型是否为 int

#warning Not implemented: TypeChecker::checkBinaryExp
  return PrimitiveType::Int;
}

TypePtr TypeChecker::checkInitVal(AST::InitValPtr node,
                                  ArrayTypePtr array_type) {                                    
  auto val = node->inits[0];  
  if (auto n = std::dynamic_pointer_cast<AST::InitList>(val)) {
    // 初值列表
    if (array_type == nullptr) {
      ASSERT(false, "Non-Array Can not initialize with list " +
                        std::to_string(node->lineno));
    }
    auto type = checkInitList(n, array_type);
    return type;
  }
  else{
    if (array_type->dims.size() != 0) {
      // 数组
      ASSERT(false, "Array must initialize with list" +
                        std::to_string(node->lineno));
    }
    auto type = check(val);
    if (!type->equals(array_type->element_type)) {
      ASSERT(false, "Initialization type mismatch at line " +
                        std::to_string(node->lineno));
    }
    return type;
  }
}

TypePtr TypeChecker::checkInitList(AST::InitListPtr node,
  ArrayTypePtr array_type) {
  /// @brief 检查数组初始化
  // 检查数组的维度和初始化表达式的维度是否匹配
  // 1. 在初值列表中，可以直接使用标量进行横跨任意维度的初始化
  // 2. 在抵达初值列表结尾时，如果对应数组仍未被填满，则填 0 直到对应数组被填满：
  // 3. 在初值列表内，可以用嵌套的初值列表对子数组进行初始化：
  // 4. 遇到嵌套的初值列表时，根据已经填充的元素数决定该初值列表对应的子数组，并尽量选择更大的子数组：
  std::cout << "array type: " << array_type->to_string() << std::endl;
  std::cout << "array type: " << array_type->element_type->to_string() << std::endl;
  std::cout << "array type: " << array_type->dims.size() << std::endl;
  int filled_elements = 0;
  int total_elements = 1;
  for (auto d : array_type->dims) {
    total_elements *= d;
  }
  auto elements = node->elements;
  ArrayTypePtr subarray_type = nullptr;
  int subarray_size;
  for (auto element : elements) {  
    auto val = std::dynamic_pointer_cast<AST::InitVal>(element);
    if (auto n = std::dynamic_pointer_cast<AST::InitList>(val->inits[0])) {
      // 子数组
      for (int i = 1; i < array_type->dims.size(); i++) {
          subarray_type = nullptr;
          subarray_size = 1;
          for (int j = i; j < array_type->dims.size(); j++) {
            subarray_size *= array_type->dims[j];
          }
          if (filled_elements % subarray_size == 0) {
            // 检查子数组的维度和初始化表达式的维度是否匹配
            subarray_type = ArrayType::create(array_type->element_type,
                                                  std::vector<int>(array_type->dims.begin() + i,
                                                                  array_type->dims.end()));  
            break;
          }      
        }
        if (subarray_type == nullptr) {
          ASSERT(false, "Subarray type mismatch at line " +
                            std::to_string(node->lineno));
        }
        checkInitList(n, subarray_type);
        filled_elements += subarray_size;
    }
    else {
      // 标量
      auto type = check(val->inits[0]);
      std::cout << "type: " << type->to_string() << std::endl;
      std::cout << "array type: " << array_type->to_string() << std::endl;
      if (!type->equals(array_type->element_type)) {
        ASSERT(false, "Initialization type mismatch at line " +
                          std::to_string(node->lineno));
      }
      filled_elements++;
    }      
    std::cout << "filled elements: " << filled_elements << "/" << total_elements << std::endl;
   
    if (filled_elements > total_elements) {
      ASSERT(false, "Excess initializers at line " +
                        std::to_string(node->lineno));
    }
  }

  return nullptr;
}

TypePtr TypeChecker::checkIfStmt(AST::IfStmtPtr node) {
  // 检查 if 语句的条件表达式是否为 int 类型
  auto cond_type = check(node->cond);
  if (!cond_type->equals(PrimitiveType::Int)) {
    ASSERT(false, "If condition type mismatch at line " +
                      std::to_string(node->lineno));
  }
  // 检查 if 语句的语句块
  check(node->true_stmt);
  if (node->false_stmt) {
    // 检查 else 语句的语句块
    check(node->false_stmt);
  }
  return nullptr;
}

TypePtr TypeChecker::checkWhileStmt(AST::WhileStmtPtr node) {
  // 检查 while 语句的条件表达式是否为 int 类型
  auto cond_type = check(node->cond);
  if (!cond_type->equals(PrimitiveType::Int)) {
    ASSERT(false, "While condition type mismatch at line " +
                      std::to_string(node->lineno));
  }
  // 检查 while 语句的语句块
  check(node->stmt);
  return nullptr;
}

TypePtr TypeChecker::checkEmptyStmt(AST::EmptyStmtPtr node) {
  // 空语句不需要检查
  return nullptr;
}
