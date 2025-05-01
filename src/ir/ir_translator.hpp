#ifndef IR_IR_TRANSLATOR_HPP
#define IR_IR_TRANSLATOR_HPP

#include <memory>
#include <vector>

#include "ast/tree.hpp"
#include "ir/ir.hpp"

class IRTranslator {
 public:
  IR::Code translate(AST::NodePtr node);
  IR::Code translateExp(AST::NodePtr node, const std::string &place = "");
  IR::Code translateCond(AST::NodePtr node, const std::string &label_true, 
                                            const std::string &label_false);
 private:
  IR::Code translateCompUnit(AST::CompUnitPtr node);
  IR::Code translateFuncDef(AST::FuncDefPtr node);
  IR::Code translateBlock(AST::BlockPtr node);
  IR::Code translateVarDecl(AST::VarDeclPtr node);
  IR::Code translateVarDef(AST::VarDefPtr node);
  IR::Code translateAssignStmt(AST::AssignStmtPtr node);
  IR::Code translateReturnStmt(AST::ReturnStmtPtr node);
  IR::Code translateIfStmt(AST::IfStmtPtr node);
  IR::Code translateWhileStmt(AST::WhileStmtPtr node);
  IR::Code translateLVal(AST::LValPtr node, const std::string &place = "");
  IR::Code translateBinaryExp(AST::BinaryExpPtr node,
                              const std::string &place = "");
  IR::Code translateUnaryExp(AST::UnaryExpPtr node,
                             const std::string &place = "");
  IR::Code translateFuncCall(AST::FuncCallPtr node,
                             const std::string &place = "");
  IR::Code translateIntConst(AST::IntConstPtr node,
                             const std::string &place = "");

  // 翻译函数参数
  IR::Code translateFuncFParam(AST::FuncFParamPtr node,
                               const std::string &place = "");

  IR::Code translateCondBinaryExp(AST::BinaryExpPtr node,
                                  const std::string &label_true,
                                  const std::string &label_false);

  IR::Code translateCondUnaryExp(AST::UnaryExpPtr node,
                                 const std::string &label_true,
                                 const std::string &label_false);

  IR::Code translateCondOther(AST::NodePtr node,
                              const std::string &label_true,
                              const std::string &label_false);

  std::string new_temp();
  std::string new_label();
};

#endif  // IR_IR_TRANSLATOR_HPP