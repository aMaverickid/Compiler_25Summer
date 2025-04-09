#ifndef AST_TREE_HPP
#define AST_TREE_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "common.hpp"
#include "semantic/symbol_table.hpp"

extern int yylineno;

namespace AST {

class Node;
using NodePtr = std::shared_ptr<Node>;
class Node {
 public:
  int lineno;

  SymbolPtr symbol;  // for semantic analysis
  virtual std::vector<NodePtr> get_children() { return std::vector<NodePtr>(); }
  void print_tree(std::string prefix = "", std::string info_prefix = "");
  virtual std::string to_string() = 0;

  Node() : lineno(yylineno) {}
  virtual ~Node() = default;
};

class IntConst;
using IntConstPtr = std::shared_ptr<IntConst>;
class IntConst : public Node {
 public:
  int value;
  IntConst(int value) : value(value) {}
  std::string to_string() override {
    return "IntConst <value: " + std::to_string(value) + ">";
  }
};

class LVal;
using LValPtr = std::shared_ptr<LVal>;
class LVal : public Node {
 public:
  std::string ident;
#warning Have not support array yet
  LVal(std::string ident) : ident(ident) {}
  std::string to_string() override { return "LVal <ident: " + ident + ">"; }
};

class UnaryExp;
using UnaryExpPtr = std::shared_ptr<UnaryExp>;
class UnaryExp : public Node {
 public:
  BinaryOp op;
  NodePtr exp;
  UnaryExp(BinaryOp op, NodePtr exp) : op(op), exp(exp) {}
  std::string to_string() override {
    return "UnaryExp <op: " + std::string(op_to_string(op)) + ">";
  }
  std::vector<NodePtr> get_children() override { return {exp}; }
};

class BinaryExp;
using BinaryExpPtr = std::shared_ptr<BinaryExp>;
class BinaryExp : public Node {
 public:
  BinaryOp op;
  NodePtr left, right;

  BinaryExp(BinaryOp op, NodePtr left, NodePtr right)
      : op(op), left(left), right(right) {}
  std::string to_string() override {
    return "BinaryExp <op: " + std::string(op_to_string(op)) + ">";
  }
  std::vector<NodePtr> get_children() override { return {left, right}; }
};

class FuncCall;
using FuncCallPtr = std::shared_ptr<FuncCall>;
class FuncCall : public Node {
 public:
  std::string name;
  std::vector<NodePtr> args;
  FuncCall(char const *name) : name(name) {}
  FuncCall(NodePtr exp) { add_arg(exp); }
  void add_arg(NodePtr exp) { args.push_back(exp); }
  std::string to_string() override { return "FuncCall <name: " + name + ">"; }
  std::vector<NodePtr> get_children() override { return args; }
};

class Block;
using BlockPtr = std::shared_ptr<Block>;
class Block : public Node {
 public:
  std::vector<NodePtr> stmts;
  Block() {}
  Block(NodePtr stmt) { add_stmt(stmt); }
  void add_stmt(NodePtr stmt) { stmts.push_back(stmt); }
  std::string to_string() override { return "Block"; }
  std::vector<NodePtr> get_children() override { return stmts; }
};

class AssignStmt;
using AssignStmtPtr = std::shared_ptr<AssignStmt>;
class AssignStmt : public Node {
 public:
  LValPtr lval;
  NodePtr exp;
  AssignStmt(LValPtr lval, NodePtr exp) : lval(lval), exp(exp) {}
  std::string to_string() override { return "AssignStmt"; }
  std::vector<NodePtr> get_children() override { return {lval, exp}; }
};

class ReturnStmt;
using ReturnStmtPtr = std::shared_ptr<ReturnStmt>;
class ReturnStmt : public Node {
 public:
  NodePtr exp;
  ReturnStmt() : exp(nullptr) {}
  ReturnStmt(NodePtr exp) : exp(exp) {}
  std::string to_string() override { return "ReturnStmt"; }
  std::vector<NodePtr> get_children() override {
    return exp ? std::vector<NodePtr>{exp} : std::vector<NodePtr>();
  }
};

class EmptyStmt;
using EmptyStmtPtr = std::shared_ptr<EmptyStmt>;
class EmptyStmt : public Node {
 public:
  std::string to_string() override { return "EmptyStmt"; }
};

class IfStmt;
using IfStmtPtr = std::shared_ptr<IfStmt>;
class IfStmt : public Node {
 public:
  NodePtr cond, true_stmt, false_stmt;
  IfStmt(NodePtr cond, NodePtr true_stmt, NodePtr false_stmt=nullptr)
      : cond(cond), true_stmt(true_stmt), false_stmt(false_stmt) {}
  std::string to_string() override { return "IfStmt"; }
  std::vector<NodePtr> get_children() override {
    if (false_stmt) {
      return {cond, true_stmt, false_stmt};
    }
    return {cond, true_stmt};
  }
};

class WhileStmt;
using WhileStmtPtr = std::shared_ptr<WhileStmt>;
class WhileStmt : public Node {
 public:
  NodePtr cond, stmt;
  WhileStmt(NodePtr cond, NodePtr stmt) : cond(cond), stmt(stmt) {}
  std::string to_string() override { return "WhileStmt"; }
  std::vector<NodePtr> get_children() override { return {cond, stmt}; }
};

// class InitElements;
// using InitElementsPtr = std::shared_ptr<InitElements>;
// class InitElements : public Node {
//  public:
//   std::vector<NodePtr> elements;
//   InitElements(NodePtr element) { add_element(element); }
//   void add_element(NodePtr element) { elements.push_back(element); }
//   std::string to_string() override { return "InitElements"; }
//   std::vector<NodePtr> get_children() override { return elements; }
// };

class InitList;
using InitListPtr = std::shared_ptr<InitList>;
class InitList : public Node {
 public:
  std::vector<NodePtr> inits;
  InitList() {}
  InitList(NodePtr init) { add_init(init); }
  void add_init(NodePtr init) { inits.push_back(init); }
  std::string to_string() override { return "InitList"; }
  std::vector<NodePtr> get_children() override { return inits; }
};

class VarDef;
using VarDefPtr = std::shared_ptr<VarDef>;
class VarDef : public Node {
 public:
  std::string ident;
  std::vector<int> dim;
  // init list
  InitListPtr inits;
  
  VarDef(char const *ident) : ident(ident) {}
  VarDef(VarDefPtr var, int d) : ident(var->ident), dim(var->dim) { add_dim(d); }
  VarDef(char const *ident, InitListPtr inits) : ident(ident), inits(inits) {};
  VarDef(VarDefPtr var, int d, InitListPtr inits) : ident(var->ident), dim(var->dim), inits(inits) { add_dim(d); }
  void add_dim (int d) { dim.push_back(d); }
  std::string to_string() override { 
    if (dim.size() > 0) {
      std::string dim_str = "dim: (";
      for (int d : dim) {
        dim_str += std::to_string(d) + ",";
      }
      dim_str.pop_back();
      dim_str += ")";
      return "VarDef <ident: " + ident + ", " + dim_str + ">";
    }
    return "VarDef <ident: " + ident + ">"; 
  }
  std::vector<NodePtr> get_children() override {
    if (inits) {
      return {inits};
    }
    return {};
  }
};

class VarDecl;
using VarDeclPtr = std::shared_ptr<VarDecl>;
class VarDecl : public Node {
 public:
  BasicType btype;
  std::vector<VarDefPtr> defs;
  VarDecl(VarDefPtr def) : btype(BasicType::Unknown) { add_def(def); }
  void add_def(VarDefPtr def) { defs.push_back(def); }
  std::string to_string() override {
    return "VarDecl <btype: " + std::string(type_to_string(btype)) + ">";
  }
  std::vector<NodePtr> get_children() override {
    return std::vector<NodePtr>(defs.begin(), defs.end());
  }
};

class ArrayDims;
using ArrayDimsPtr = std::shared_ptr<ArrayDims>;
class ArrayDims : public Node {
  public:
    std::vector<int> dims;
    ArrayDims(int d) { add_dim(d); }
    void add_dim(int dim) { dims.push_back(dim); }
    std::string to_string() override {
      std::string dim_str = "dims: (";
      for (int d : dims) {
        dim_str += std::to_string(d) + ",";
      }
      dim_str.pop_back();
      dim_str += ")";
      return "ArrayDims <" + dim_str + ">";
    }
};

class FuncFParam;
using FuncFParamPtr = std::shared_ptr<FuncFParam>;
class FuncFParam : public Node {
  public:    
    std::string ident;
    std::vector<int> dim;
    FuncFParam(char const *ident) : ident(ident) {}
    FuncFParam(char const *ident, ArrayDimsPtr dims) : ident(ident), dim(dims->dims) {}
    std::string to_string() override {       
      if (dim.size() > 0) {
        std::string dim_str = "dim: ( , ";        
        for (int d : dim) {
          dim_str += std::to_string(d) + ",";
        }
        dim_str.pop_back();
        dim_str += ")";
        return "<btype: int, ident: " + ident + ", " + dim_str + ">";
      }
      return "<bytype: int, ident: " + ident + ">"; 
    }    
};

class FuncFParams;
using FuncFParamsPtr = std::shared_ptr<FuncFParams>;
class FuncFParams : public Node {
  public:
    std::vector<FuncFParamPtr> params;
    FuncFParams(FuncFParamPtr param) { add_param(param); }
    void add_param(FuncFParamPtr param) { params.push_back(param); }    
    std::string to_string() override {
      std::string params_str = "";
      for (FuncFParamPtr param : params) {
        params_str += param->to_string() + ", ";
      }
      params_str.pop_back();
      params_str.pop_back();
      params_str += " }";
      return "Params { " + params_str + ">";
    }
};

class FuncDef;
using FuncDefPtr = std::shared_ptr<FuncDef>;
class FuncDef : public Node {
 public:
  BasicType return_btype;
  std::string name;
  BlockPtr block;
  // to support params:
  FuncFParamsPtr params;

  FuncDef(BasicType return_btype, char const *name, BlockPtr block)
      : return_btype(return_btype), name(name), block(block) {}
  
  FuncDef(BasicType return_btype, char const *name, BlockPtr block, FuncFParamsPtr params)  
      : return_btype(return_btype), name(name), block(block), params(params) {}

  std::string to_string() override {
    // add params
    return "FuncDef <return_btype: " + std::string(type_to_string(return_btype)) + ", name: " + name + ">";
  }
  std::vector<NodePtr> get_children() override { 
    if (params) {
      return {params, block};
    }
    return {block};
  }
};

class CompUnit;
using CompUnitPtr = std::shared_ptr<CompUnit>;
class CompUnit : public Node {
 public:
  std::vector<NodePtr> units;  // FuncDef or VarDecl
  CompUnit(NodePtr unit) { add_unit(unit); }
  void add_unit(NodePtr unit) { units.push_back(unit); }
  std::string to_string() override { return "CompUnit"; }
  std::vector<NodePtr> get_children() override { return units; }
};

#warning More AST nodes are needed

}  // namespace AST

#endif  // AST_TREE_HPP