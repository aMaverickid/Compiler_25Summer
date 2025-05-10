#include "ir_translator.hpp"

#include <cassert>

std::string IRTranslator::new_temp() {
  static int temp_count = 0;
  return "T" + std::to_string(temp_count++);
}

std::string IRTranslator::new_label() {
  static int label_count = 1;
  return "label" + std::to_string(label_count++);
}

IR::Code IRTranslator::translate(AST::NodePtr node) {
#define TRANSLATE_NODE(type)                                 \
  if (auto n = std::dynamic_pointer_cast<AST::type>(node)) { \
    return translate##type(n);                               \
  }
  // 递归翻译 AST 的每个节点
  // 如果你添加了新的 AST 节点类型，记得在这里添加对应的翻译函数

  TRANSLATE_NODE(CompUnit)
  TRANSLATE_NODE(FuncDef)
  TRANSLATE_NODE(Block)
  TRANSLATE_NODE(VarDecl)
  TRANSLATE_NODE(VarDef)
  TRANSLATE_NODE(AssignStmt)
  TRANSLATE_NODE(ReturnStmt)
  TRANSLATE_NODE(LVal)
  TRANSLATE_NODE(BinaryExp)
  TRANSLATE_NODE(UnaryExp)
  TRANSLATE_NODE(FuncCall)
  TRANSLATE_NODE(IntConst)

  TRANSLATE_NODE(IfStmt)
  TRANSLATE_NODE(WhileStmt)

#warning Add more AST node types if needed

#undef TRANSLATE_NODE

  ASSERT(false,
         "Unknown AST node type " + node->to_string() + " in IR translation");
}

IR::Code IRTranslator::translateExp(AST::NodePtr node,
                                    const std::string &place) {
#define TRANSLATE_EXP_NODE(type)                             \
  if (auto n = std::dynamic_pointer_cast<AST::type>(node)) { \
    return translate##type(n, place);                        \
  }

  TRANSLATE_EXP_NODE(BinaryExp)
  TRANSLATE_EXP_NODE(UnaryExp)
  TRANSLATE_EXP_NODE(FuncCall)
  TRANSLATE_EXP_NODE(IntConst)
  TRANSLATE_EXP_NODE(LVal)

#warning Add more AST node types if needed

#undef TRANSLATE_EXP_NODE

  ASSERT(false, "No translateExp for node " + node->to_string());
}

IR::Code IRTranslator::translateCond(AST::NodePtr node, const std::string &label_true,
                                    const std::string &label_false) {
#define TRANSLATE_COND_NODE(type)                            \
  if (auto n = std::dynamic_pointer_cast<AST::type>(node)) { \
    return translateCond##type(n, label_true, label_false);  \
  }

  TRANSLATE_COND_NODE(BinaryExp)
  TRANSLATE_COND_NODE(UnaryExp)

#undef TRANSLATE_COND_NODE

  // other cases
  return translateCondOther(node, label_true, label_false);
}

IR::Code IRTranslator::translateIfStmt(AST::IfStmtPtr node) {
  IR::Code ir;
  if (!node->false_stmt) {
    auto label_true = new_label();
    auto label_false = new_label();
    auto code1 = translateCond(node->cond, label_true, label_false);
    auto code2 = translate(node->true_stmt);
    // return code1 + [LABEL label1] + code2 + [LABEL label2]
    auto LT = IR::Label::create(label_true);
    auto LF = IR::Label::create(label_false);
    std::move(code1.begin(), code1.end(), std::back_inserter(ir));
    ir.push_back(LT);
    std::move(code2.begin(), code2.end(), std::back_inserter(ir));
    ir.push_back(LF);
  }

  else {
    auto label_1 = new_label();
    auto label_2 = new_label();
    auto label_3 = new_label();
    auto code1 = translateCond(node->cond, label_1, label_2);
    auto code2 = translate(node->true_stmt);
    auto code3 = translate(node->false_stmt);

    auto L1 = IR::Label::create(label_1);
    auto L2 = IR::Label::create(label_2);
    auto L3 = IR::Label::create(label_3);
    auto GOTO3 = IR::Goto::create(label_3);
    std::move(code1.begin(), code1.end(), std::back_inserter(ir));
    ir.push_back(L1);
    std::move(code2.begin(), code2.end(), std::back_inserter(ir));
    ir.push_back(GOTO3);
    ir.push_back(L2);
    std::move(code3.begin(), code3.end(), std::back_inserter(ir));
    ir.push_back(L3);
  }

  return ir;
}

IR::Code IRTranslator::translateWhileStmt(AST::WhileStmtPtr node) {
  IR::Code ir;
  auto label_1 = new_label();
  auto label_2 = new_label();
  auto label_3 = new_label();
  auto code1 = translateCond(node->cond, label_2, label_3);
  auto code2 = translate(node->stmt);

  auto L1 = IR::Label::create(label_1);
  auto L2 = IR::Label::create(label_2);
  auto L3 = IR::Label::create(label_3);
  auto GOTO1 = IR::Goto::create(label_1);
  ir.push_back(L1);
  std::move(code1.begin(), code1.end(), std::back_inserter(ir));
  ir.push_back(L2);
  std::move(code2.begin(), code2.end(), std::back_inserter(ir));
  ir.push_back(GOTO1);
  ir.push_back(L3);
  return ir;
}

IR::Code IRTranslator::translateCondBinaryExp(AST::BinaryExpPtr node,
                                              const std::string &label_true,
                                              const std::string &label_false) {
  IR::Code ir;
  if (node->op == BinaryOp::And) {
    auto label1 = new_label();
    auto code1 = translateCond(node->left, label1, label_false);
    auto code2 = translateCond(node->right, label_true, label_false);
    auto L1 = IR::Label::create(label1);
    std::move(code1.begin(), code1.end(), std::back_inserter(ir));
    ir.push_back(L1);
    std::move(code2.begin(), code2.end(), std::back_inserter(ir));
  }
  else if (node->op == BinaryOp::Or) {
    auto label1 = new_label();
    auto code1 = translateCond(node->left, label_true, label1);
    auto code2 = translateCond(node->right, label_true, label_false);
    auto L1 = IR::Label::create(label1);
    std::move(code1.begin(), code1.end(), std::back_inserter(ir));
    ir.push_back(L1);
    std::move(code2.begin(), code2.end(), std::back_inserter(ir));
  }
  else {
    // RELOP
    auto t1 = new_temp();
    auto t2 = new_temp();
    auto code1 = translateExp(node->left, t1);
    auto code2 = translateExp(node->right, t2);
    auto if_ir = IR::If::create(node->op, t1, t2, label_true);
    std::move(code1.begin(), code1.end(), std::back_inserter(ir));
    std::move(code2.begin(), code2.end(), std::back_inserter(ir));
    ir.push_back(if_ir);
    auto goto_false = IR::Goto::create(label_false);
    ir.push_back(goto_false);
  }
  return ir;
}

IR::Code IRTranslator::translateCondUnaryExp(AST::UnaryExpPtr node,
                                              const std::string &label_true,
                                              const std::string &label_false) {
  IR::Code ir;
  if (node->op == BinaryOp::Not) {
    auto code = translateCond(node->exp, label_false, label_true);
    std::move(code.begin(), code.end(), std::back_inserter(ir));
  }
  else {
    translateCondOther(node, label_true, label_false);
  }
  return ir;
}

IR::Code IRTranslator::translateCondOther(AST::NodePtr node,
                                          const std::string &label_true,
                                          const std::string &label_false) {
  IR::Code ir;
  auto t1 = new_temp();
  auto code1 = translateExp(node, t1);
  auto t2 = new_temp();
  auto code2 = IR::LoadImm::create(t2, 0);
  auto if_ir = IR::If::create(BinaryOp::Ne, t1, t2, label_true);
  std::move(code1.begin(), code1.end(), std::back_inserter(ir));
  ir.push_back(code2);
  ir.push_back(if_ir);
  auto goto_false = IR::Goto::create(label_false);
  ir.push_back(goto_false);
  return ir;
}

IR::Code IRTranslator::translateCompUnit(AST::CompUnitPtr node) {
  IR::Code ir;
  for (auto &unit : node->units) {
    auto unit_ir = translate(unit);
    std::move(unit_ir.begin(), unit_ir.end(), std::back_inserter(ir));
  }
  return ir;
}

IR::Code IRTranslator::translateFuncDef(AST::FuncDefPtr node) {
  IR::Code ir;
  ir.push_back(IR::Function::create(node->name));
  if (node->params) {
    for (auto &param : node->params->params) {
      ir.push_back(IR::Param::create(param->symbol->unique_name, node->name, param->dim.size()));
    }
  }
  
  ++scope_depth;  // Enter function scope
  auto block_ir = translate(node->block);
  --scope_depth;  // Exit function scope
  
  std::move(block_ir.begin(), block_ir.end(), std::back_inserter(ir));
  return ir;
}

IR::Code IRTranslator::translateBlock(AST::BlockPtr node) {
  IR::Code ir;
  ++scope_depth;  // Enter block scope
  for (auto &stmt : node->stmts) {
    auto stmt_ir = translate(stmt);
    std::move(stmt_ir.begin(), stmt_ir.end(), std::back_inserter(ir));
  }
  --scope_depth;  // Exit block scope
  return ir;
}

IR::Code IRTranslator::translateVarDecl(AST::VarDeclPtr node) {
  IR::Code ir;
  for (auto &def : node->defs) {
    auto def_ir = translate(def);
    std::move(def_ir.begin(), def_ir.end(), std::back_inserter(ir));
  }
  return ir;
}

IR::Code IRTranslator::translateVarDef(AST::VarDefPtr node) {
  IR::Code ir;

  // Calculate total size in bytes (4 bytes per int)
  int total_size = 4;
  // treat scalar global variable as 1-dimension array
  if (node->dim.empty()) {
    total_size = 4;
  }
  for (int dim : node->dim) {
    total_size *= dim;
  }

  // Global variable
  if (scope_depth == 0) {
    // Global array
    std::vector<int> values;
    if (node->inits) {
      // Extract initial values
      std::vector<AST::NodePtr> initvals = node->inits->inits;
      if (auto initlist = std::dynamic_pointer_cast<AST::InitList>(node->inits->inits[0])) {
          initvals = initlist->elements;
      }
      for (auto &init : initvals) {
        auto initval = std::dynamic_pointer_cast<AST::InitVal>(init);
        auto int_const = std::dynamic_pointer_cast<AST::IntConst>(initval->inits[0]);
        values.push_back(int_const->value);
      }
    }
    ir.push_back(IR::Global::create(node->symbol->unique_name, total_size, values));    
  }
  // Local variable
  else {
    if (!node->inits) return ir;
    if (node->dim.empty()) {
      // Scalar local variable
      auto init_ir = translateInitVal(node->inits, node->symbol->unique_name);
      std::move(init_ir.begin(), init_ir.end(), std::back_inserter(ir));
    } else {
      // Local array
      ir.push_back(IR::Dec::create(node->symbol->unique_name, total_size));
      if (node->inits) {
        // Initialize array elements
        for (size_t i = 0; i < node->inits->inits.size(); ++i) {
          auto temp = new_temp();
          auto init_ir = translateExp(node->inits->inits[i], temp);
          std::move(init_ir.begin(), init_ir.end(), std::back_inserter(ir));
          ir.push_back(IR::Store::create(node->symbol->unique_name, temp, i * 4));
        }
      }
    }
  }
  return ir;
}

IR::Code IRTranslator::translateAssignStmt(AST::AssignStmtPtr node) {
  IR::Code ir;
  auto lnode = node->lval;
  auto rnode = node->exp;

  if (lnode->indexes.empty()) {
    // Scalar assignment
    auto exp_ir = translateExp(rnode, lnode->symbol->unique_name);
    std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
  } else {
    // Array assignment
    auto value_temp = new_temp();
    auto value_ir = translateExp(rnode, value_temp);
    std::move(value_ir.begin(), value_ir.end(), std::back_inserter(ir));

    if (scope_depth == 0) {
      // Global array assignment
      auto addr_temp = new_temp();
      ir.push_back(IR::LoadAddr::create(addr_temp, lnode->symbol->unique_name));
      
      // Calculate offset
      auto offset_temp = new_temp();
      ir.push_back(IR::LoadImm::create(offset_temp, 0));
      for (size_t i = 0; i < lnode->indexes.size(); ++i) {
        auto index_temp = new_temp();
        auto index_ir = translateExp(lnode->indexes[i], index_temp);
        std::move(index_ir.begin(), index_ir.end(), std::back_inserter(ir));
        
        // Multiply by 4 for int size
        auto mul_temp = new_temp();
        ir.push_back(IR::Binary::create(mul_temp, index_temp, BinaryOp::Mul, "#4"));
        
        // Add to offset
        auto add_temp = new_temp();
        ir.push_back(IR::Binary::create(add_temp, offset_temp, BinaryOp::Add, mul_temp));
        offset_temp = add_temp;
      }
      
      // Store value at offset
      ir.push_back(IR::Store::create(addr_temp, value_temp, 0));
    } else {
      // Local array assignment
      auto offset_temp = new_temp();
      ir.push_back(IR::LoadImm::create(offset_temp, 0));
      for (size_t i = 0; i < lnode->indexes.size(); ++i) {
        auto index_temp = new_temp();
        auto index_ir = translateExp(lnode->indexes[i], index_temp);
        std::move(index_ir.begin(), index_ir.end(), std::back_inserter(ir));
        
        // Multiply by 4 for int size
        auto mul_temp = new_temp();
        ir.push_back(IR::Binary::create(mul_temp, index_temp, BinaryOp::Mul, "#4"));
        
        // Add to offset
        auto add_temp = new_temp();
        ir.push_back(IR::Binary::create(add_temp, offset_temp, BinaryOp::Add, mul_temp));
        offset_temp = add_temp;
      }
      
      // Store value at offset
      ir.push_back(IR::Store::create(lnode->symbol->unique_name, value_temp, 0));
    }
  }

  return ir;
}

IR::Code IRTranslator::translateReturnStmt(AST::ReturnStmtPtr node) {
  IR::Code ir;

  // 翻译返回值
  // 如果有返回值，则：
  // place = new_temp();
  // auto exp_ir = translateExp(node->exp, place);
  // return exp_ir + [RETURN place];
  // 否则：
  // return [RETURN];

  if (!node->exp) {
    ir.push_back(IR::Return::create());
  } else {
    auto place = new_temp();
    auto exp_ir = translateExp(node->exp, place);
    std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));
    ir.push_back(IR::Return::create(place));
  }
  return ir;
}

IR::Code IRTranslator::translateLVal(AST::LValPtr node, const std::string &place) {
  IR::Code ir;
  if (!place.empty()) {
    if (node->indexes.empty()) {
      // Scalar variable
      ir.push_back(IR::Assign::create(place, node->symbol->unique_name));
    } else {
      // Array access
      if (scope_depth == 0) {
        // Global array access
        auto addr_temp = new_temp();
        ir.push_back(IR::LoadAddr::create(addr_temp, node->symbol->unique_name));
        
        // Calculate offset
        auto offset_temp = new_temp();
        ir.push_back(IR::LoadImm::create(offset_temp, 0));
        for (size_t i = 0; i < node->indexes.size(); ++i) {
          auto index_temp = new_temp();
          auto index_ir = translateExp(node->indexes[i], index_temp);
          std::move(index_ir.begin(), index_ir.end(), std::back_inserter(ir));
          
          // Multiply by 4 for int size
          auto mul_temp = new_temp();
          ir.push_back(IR::Binary::create(mul_temp, index_temp, BinaryOp::Mul, "#4"));
          
          // Add to offset
          auto add_temp = new_temp();
          ir.push_back(IR::Binary::create(add_temp, offset_temp, BinaryOp::Add, mul_temp));
          offset_temp = add_temp;
        }
        
        // Load value at offset
        ir.push_back(IR::Load::create(place, addr_temp, 0));
      } else {
        // Local array access
        auto offset_temp = new_temp();
        ir.push_back(IR::LoadImm::create(offset_temp, 0));
        for (size_t i = 0; i < node->indexes.size(); ++i) {
          auto index_temp = new_temp();
          auto index_ir = translateExp(node->indexes[i], index_temp);
          std::move(index_ir.begin(), index_ir.end(), std::back_inserter(ir));
          
          // Multiply by 4 for int size
          auto mul_temp = new_temp();
          ir.push_back(IR::Binary::create(mul_temp, index_temp, BinaryOp::Mul, "#4"));
          
          // Add to offset
          auto add_temp = new_temp();
          ir.push_back(IR::Binary::create(add_temp, offset_temp, BinaryOp::Add, mul_temp));
          offset_temp = add_temp;
        }
        
        // Load value at offset
        ir.push_back(IR::Load::create(place, node->symbol->unique_name, 0));
      }
    }
  }
  return ir;
}

IR::Code IRTranslator::translateBinaryExp(AST::BinaryExpPtr node,
                                          const std::string &place) {
  IR::Code ir;
  auto left_place = new_temp();
  auto right_place = new_temp();

  // 翻译左右子表达式
  auto left_ir = translateExp(node->left, left_place);
  auto right_ir = translateExp(node->right, right_place);

  std::move(left_ir.begin(), left_ir.end(), std::back_inserter(ir));
  std::move(right_ir.begin(), right_ir.end(), std::back_inserter(ir));

  // 添加二元运算指令
  if (!place.empty()) {
    ir.push_back(IR::Binary::create(place, left_place, node->op, right_place));
  }
  return ir;
}

IR::Code IRTranslator::translateUnaryExp(AST::UnaryExpPtr node,
                                         const std::string &place) {
  IR::Code ir;

  // 翻译子表达式
  // 如果 place 不为空，则将 UnaryExp 的值赋给 place

  auto exp_place = new_temp();
  auto exp_ir = translateExp(node->exp, exp_place);
  std::move(exp_ir.begin(), exp_ir.end(), std::back_inserter(ir));

  if (!place.empty()) {
    ir.push_back(IR::Unary::create(place, node->op, exp_place));
  }

  return ir;
}

IR::Code IRTranslator::translateFuncCall(AST::FuncCallPtr node,
                                         const std::string &place) {
  IR::Code ir;
  std::vector<std::string> arg_places;

  // 首先翻译参数表达式，并存在临时变量中
  // 接下来，添加参数传递指令和函数调用指令
  // 如果 place 不为空，则将函数调用的返回值赋给 place
  IR::Code func_args_ir;
  auto func_args = node->args;
  int i = 0;
  for (auto &arg : func_args) {
    auto arg_place = new_temp();
    auto arg_ir = translateExp(arg, arg_place);
    std::move(arg_ir.begin(), arg_ir.end(), std::back_inserter(ir));
    func_args_ir.push_back(IR::Arg::create(arg_place, node->name, i++));
  }

  std::move(func_args_ir.begin(), func_args_ir.end(), std::back_inserter(ir));

  if (!place.empty()) {
    ir.push_back(IR::Call::create(place, node->name));
  } else {
    ir.push_back(IR::Call::create(node->name));
  }

  return ir;
}

IR::Code IRTranslator::translateIntConst(AST::IntConstPtr node,
                                         const std::string &place) {
  IR::Code ir;
  // 添加赋值常量指令
  if (!place.empty()) {
    ir.push_back(IR::LoadImm::create(place, node->value));
  }
  return ir;
}

IR::Code IRTranslator::translateFuncFParam(AST::FuncFParamPtr node,
                                          const std::string &place) {
  IR::Code ir;
  return ir;
}

IR::Code IRTranslator::translateInitVal(AST::InitValPtr node, const std::string &place) {
  IR::Code ir;
  if (node->inits.size() == 1) {
    auto init_exp = translateExp(node->inits[0], place);
    std::move(init_exp.begin(), init_exp.end(), std::back_inserter(ir));
  } else {
    ASSERT(false, "Not implemented: IRTranslator::translateInitVal for multiple initial values");
  }
  

  return ir;
}