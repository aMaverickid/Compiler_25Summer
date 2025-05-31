#ifndef CODEGEN_ASM_HPP
#define CODEGEN_ASM_HPP

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <cassert>
namespace ASM {

class Reg {
public:
    static const Reg zero, ra, sp, gp, tp, t0, t1, t2, fp, s1, a0, a1, a2, a3, a4,
        a5, a6, a7, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, t3, t4, t5, t6;

    std::string name;

    Reg(std::string name) :
        name(name), physical(false) {
        static const std::set<std::string> physical_registers = {
            "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "fp", "s1", "a0",
            "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5",
            "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

        if (physical_registers.find(name) != physical_registers.end()) {
            physical = true;
        }
    }

    bool operator==(const Reg &other) const {
        return name == other.name && physical == other.physical;
    }
    bool operator!=(const Reg &other) const {
        return !(*this == other);
    }
    bool operator<(const Reg &other) const {
        return name < other.name || (name == other.name && physical < other.physical);
    }
    bool is_phys() const {
        return physical;
    }

private:
    bool physical;
    Reg(std::string name, bool physical) :
        name(name), physical(physical) {
    }
};

inline const Reg Reg::zero("zero", true), Reg::ra("ra", true),
    Reg::sp("sp", true), Reg::gp("gp", true), Reg::tp("tp", true),
    Reg::t0("t0", true), Reg::t1("t1", true), Reg::t2("t2", true),
    Reg::fp("fp", true), Reg::s1("s1", true), Reg::a0("a0", true),
    Reg::a1("a1", true), Reg::a2("a2", true), Reg::a3("a3", true),
    Reg::a4("a4", true), Reg::a5("a5", true), Reg::a6("a6", true),
    Reg::a7("a7", true), Reg::s2("s2", true), Reg::s3("s3", true),
    Reg::s4("s4", true), Reg::s5("s5", true), Reg::s6("s6", true),
    Reg::s7("s7", true), Reg::s8("s8", true), Reg::s9("s9", true),
    Reg::s10("s10", true), Reg::s11("s11", true), Reg::t3("t3", true),
    Reg::t4("t4", true), Reg::t5("t5", true), Reg::t6("t6", true);

using RegMap = std::map<Reg, Reg>;

class Inst;
using InstPtr = std::shared_ptr<Inst>;
class Inst {
public:
    virtual std::string to_string() const = 0;
    virtual ~Inst() = default; // make the class polymorphic

    /// @brief 获取指令使用的寄存器
    /// @return 使用的寄存器集合
    virtual std::set<Reg> get_uses() const = 0;

    /// @brief 获取指令定义的寄存器
    /// @return 定义的寄存器集合
    virtual std::set<Reg> get_defs() const = 0;

    /// @brief 替换指令使用的寄存器
    /// @param reg_map 寄存器映射 (old_reg -> new_reg)
    virtual void replace_uses(const RegMap &reg_map) = 0;

    /// @brief 替换指令定义的寄存器
    /// @param reg_map 寄存器映射 (old_reg -> new_reg)
    virtual void replace_defs(const RegMap &reg_map) = 0;

    /// @brief 替换指令使用和定义的寄存器
    /// @param reg_map 寄存器映射 (old_reg -> new_reg)
    virtual void replace_all(const RegMap &reg_map) {
        replace_uses(reg_map);
        replace_defs(reg_map);
    }
};

// 算数指令
class Arith;
using ArithPtr = std::shared_ptr<Arith>;
class Arith : public Inst {
public:
    // enum class BinaryOp { Add, Sub, Not, Mul, Div, Mod, And, Or, Eq, Ne, Lt, Gt, Le, Ge };
    enum class Op {
        Add,
        Sub,
        Not,
        Mul,
        Div,
        Mod,
        And,
        Or,
        Xor,
        Sll, // 左移
        Srl  // 右移
    };

    Reg rd, rs1, rs2;
    Op op;

    Arith(Reg rd, Reg rs1, Reg rs2, Op op) :
        rd(rd), rs1(rs1), rs2(rs2), op(op) {
    }
    static ArithPtr create(Reg rd, Reg rs1, Reg rs2, Op op) {
        return std::make_shared<Arith>(rd, rs1, rs2, op);
    }

    std::string to_string() const override {
        std::string op_str;
        switch (op) {
        case Op::Add:
            op_str = "add";
            break;
        case Op::Sub:
            op_str = "sub";
            break;
        case Op::Mul:
            op_str = "mul";
            break;
        case Op::Div:
            op_str = "div";
            break;
        case Op::Mod:
            op_str = "rem";
            break;
        case Op::And:
            op_str = "and";
            break;
        case Op::Or:
            op_str = "or";
            break;
        case Op::Xor:
            op_str = "xor";
            break;
        case Op::Sll:
            op_str = "sll";
            break;
        case Op::Srl:
            op_str = "srl";
            break;
        default:
            assert(false && "Unknown arithmetic operation");
        }
        return op_str + " " + rd.name + ", " + rs1.name + ", " + rs2.name;
    }

    std::set<Reg> get_uses() const override {
        return {rs1, rs2};
    }
    std::set<Reg> get_defs() const override {
        return {rd};
    }
    void replace_uses(const RegMap &reg_map) override {
        if (reg_map.find(rs1) != reg_map.end()) {
            rs1 = reg_map.at(rs1);
        }
        if (reg_map.find(rs2) != reg_map.end()) {
            rs2 = reg_map.at(rs2);
        }
    }
    void replace_defs(const RegMap &reg_map) override {
        if (reg_map.find(rd) != reg_map.end()) {
            rd = reg_map.at(rd);
        }
    }
};

// 算数立即数指令
class ArithImm;
using ArithImmPtr = std::shared_ptr<ArithImm>;
class ArithImm : public Inst {
public:
    enum class Op {
        Addi,
        Subi,
        // 可以根据需要添加更多操作
    };

    Reg rd, rs1;
    int imm;
    Op op;

    ArithImm(Reg rd, Reg rs1, int imm, Op op) :
        rd(rd), rs1(rs1), imm(imm), op(op) {
    }
    static ArithImmPtr create(Reg rd, Reg rs1, int imm, Op op) {
        return std::make_shared<ArithImm>(rd, rs1, imm, op);
    }

    std::string to_string() const override {
        std::string op_str;
        switch (op) {
        case Op::Addi:
            op_str = "addi";
            break;
        }
        return op_str + " " + rd.name + ", " + rs1.name + ", " + std::to_string(imm);
    }

    std::set<Reg> get_uses() const override {
        return {rs1};
    }
    std::set<Reg> get_defs() const override {
        return {rd};
    }
    void replace_uses(const RegMap &reg_map) override {
        if (reg_map.find(rs1) != reg_map.end()) {
            rs1 = reg_map.at(rs1);
        }
    }
    void replace_defs(const RegMap &reg_map) override {
        if (reg_map.find(rd) != reg_map.end()) {
            rd = reg_map.at(rd);
        }
    }
};

class Mv;
using MvPtr = std::shared_ptr<Mv>;
class Mv : public Inst {
public:
    Reg rd, rs;

    Mv(Reg rd, Reg rs) :
        rd(rd), rs(rs) {
    }
    static MvPtr create(Reg rd, Reg rs) {
        return std::make_shared<Mv>(rd, rs);
    }

    std::string to_string() const override {
        return "mv " + rd.name + ", " + rs.name;
    }

    std::set<Reg> get_uses() const override {
        return {rs};
    }
    std::set<Reg> get_defs() const override {
        return {rd};
    }
    void replace_uses(const RegMap &reg_map) override {
        if (reg_map.find(rs) != reg_map.end()) {
            rs = reg_map.at(rs);
        }
    }
    void replace_defs(const RegMap &reg_map) override {
        if (reg_map.find(rd) != reg_map.end()) {
            rd = reg_map.at(rd);
        }
    }
};

class Li;
using LiPtr = std::shared_ptr<Li>;
class Li : public Inst {
public:
    Reg rd;
    int imm;

    Li(Reg rd, int imm) :
        rd(rd), imm(imm) {
    }
    static LiPtr create(Reg rd, int imm) {
        return std::make_shared<Li>(rd, imm);
    }

    std::string to_string() const override {
        return "li " + rd.name + ", " + std::to_string(imm);
    }

    std::set<Reg> get_uses() const override {
        return {};
    }
    std::set<Reg> get_defs() const override {
        return {rd};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
        if (reg_map.find(rd) != reg_map.end()) {
            rd = reg_map.at(rd);
        }
    }
};

class La;
using LaPtr = std::shared_ptr<La>;
class La : public Inst {
public:
    Reg rd;
    std::string label;

    La(Reg rd, std::string label) :
        rd(rd), label(label) {
    }
    static LaPtr create(Reg rd, std::string label) {
        return std::make_shared<La>(rd, label);
    }

    std::string to_string() const override {
        return "la " + rd.name + ", " + label;
    }

    std::set<Reg> get_uses() const override {
        return {};
    }
    std::set<Reg> get_defs() const override {
        return {rd};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
        if (reg_map.find(rd) != reg_map.end()) {
            rd = reg_map.at(rd);
        }
    }
};

// Memory 相关指令
// rd = M[rs1 + offset]
class Load;
using LoadPtr = std::shared_ptr<Load>;
class Load : public Inst {
public:
    Reg rd, rs1;
    int offset;

    Load(Reg rd, Reg rs1, int offset) :
        rd(rd), rs1(rs1), offset(offset) {
    }
    static LoadPtr create(Reg rd, Reg rs1, int offset) {
        return std::make_shared<Load>(rd, rs1, offset);
    }

    std::string to_string() const override {
        return "lw " + rd.name + ", " + std::to_string(offset) + "(" + rs1.name + ")";
    }

    std::set<Reg> get_uses() const override {
        return {rs1};
    }
    std::set<Reg> get_defs() const override {
        return {rd};
    }
    void replace_uses(const RegMap &reg_map) override {
        if (reg_map.find(rs1) != reg_map.end()) {
            rs1 = reg_map.at(rs1);
        }
    }
    void replace_defs(const RegMap &reg_map) override {
        if (reg_map.find(rd) != reg_map.end()) {
            rd = reg_map.at(rd);
        }
    }
};

// M[rs1 + offset] = rs2
class Store;
using StorePtr = std::shared_ptr<Store>;
class Store : public Inst {
public:
    Reg rs1, rs2;
    int offset;

    Store(Reg rs1, Reg rs2, int offset) :
        rs1(rs1), rs2(rs2), offset(offset) {
    }
    static StorePtr create(Reg rs1, Reg rs2, int offset) {
        return std::make_shared<Store>(rs1, rs2, offset);
    }

    std::string to_string() const override {
        return "sw " + rs2.name + ", " + std::to_string(offset) + "(" + rs1.name + ")";
    }

    std::set<Reg> get_uses() const override {
        return {rs1, rs2};
    }
    std::set<Reg> get_defs() const override {
        return {};
    }
    void replace_uses(const RegMap &reg_map) override {
        if (reg_map.find(rs1) != reg_map.end()) {
            rs1 = reg_map.at(rs1);
        }
        if (reg_map.find(rs2) != reg_map.end()) {
            rs2 = reg_map.at(rs2);
        }
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

// 条件分支指令
class Branch;
using BranchPtr = std::shared_ptr<Branch>;
class Branch : public Inst {
public:
    enum class Op {
        Beq, // ==
        Bne, // !=
        Blt, // <
        Ble, // <=
        Bgt, // >
        Bge  // >=
    };

    Reg rs1, rs2;
    std::string label;
    Op op;

    Branch(Reg rs1, Reg rs2, std::string label, Op op) :
        rs1(rs1), rs2(rs2), label(label), op(op) {
    }
    static BranchPtr create(Reg rs1, Reg rs2, std::string label, Op op) {
        return std::make_shared<Branch>(rs1, rs2, label, op);
    }

    std::string to_string() const override {
        std::string op_str;
        switch (op) {
        case Op::Beq:
            op_str = "beq";
            break;
        case Op::Bne:
            op_str = "bne";
            break;
        case Op::Blt:
            op_str = "blt";
            break;
        case Op::Ble:
            op_str = "ble";
            break;
        case Op::Bgt:
            op_str = "bgt";
            break;
        case Op::Bge:
            op_str = "bge";
            break;
        }
        return op_str + " " + rs1.name + ", " + rs2.name + ", " + label;
    }

    std::set<Reg> get_uses() const override {
        return {rs1, rs2};
    }
    std::set<Reg> get_defs() const override {
        return {};
    }
    void replace_uses(const RegMap &reg_map) override {
        if (reg_map.find(rs1) != reg_map.end()) {
            rs1 = reg_map.at(rs1);
        }
        if (reg_map.find(rs2) != reg_map.end()) {
            rs2 = reg_map.at(rs2);
        }
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

class Jump;
using JumpPtr = std::shared_ptr<Jump>;
class Jump : public Inst {
public:
    std::string label;

    Jump(std::string label) :
        label(label) {
    }
    static JumpPtr create(std::string label) {
        return std::make_shared<Jump>(label);
    }

    std::string to_string() const override {
        return "j " + label;
    }

    std::set<Reg> get_uses() const override {
        return {};
    }
    std::set<Reg> get_defs() const override {
        return {};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

// 函数指令
class Call;
using CallPtr = std::shared_ptr<Call>;
class Call : public Inst {
public:
    std::string func;

    Call(std::string func) :
        func(func) {
    }
    static CallPtr create(std::string func) {
        return std::make_shared<Call>(func);
    }

    std::string to_string() const override {
        return "call " + func;
    }

    std::set<Reg> get_uses() const override {
        return {Reg::a0, Reg::a1, Reg::a2, Reg::a3,
                Reg::a4, Reg::a5, Reg::a6, Reg::a7};
    }
    std::set<Reg> get_defs() const override {
        // 调用者保存寄存器和返回值寄存器
        return {Reg::ra, Reg::t0, Reg::t1, Reg::t2, Reg::t3, Reg::t4,
                Reg::t5, Reg::t6, Reg::a0, Reg::a1, Reg::a2, Reg::a3,
                Reg::a4, Reg::a5, Reg::a6, Reg::a7};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

class Ret;
using RetPtr = std::shared_ptr<Ret>;
class Ret : public Inst {
public:
    Ret() {
    }
    static RetPtr create() {
        return std::make_shared<Ret>();
    }

    std::string to_string() const override {
        return "ret";
    }

    std::set<Reg> get_uses() const override {
        return {Reg::a0, Reg::ra}; // 返回值寄存器和返回地址
    }
    std::set<Reg> get_defs() const override {
        return {};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

// 标签
class Label;
using LabelPtr = std::shared_ptr<Label>;
class Label : public Inst {
public:
    std::string label;

    Label(std::string label) :
        label(label) {
    }
    static LabelPtr create(std::string label) {
        return std::make_shared<Label>(label);
    }

    std::string to_string() const override {
        return label + ":";
    }

    std::set<Reg> get_uses() const override {
        return {};
    }
    std::set<Reg> get_defs() const override {
        return {};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

class GlobalLabel;
using GlobalLabelPtr = std::shared_ptr<GlobalLabel>;
class GlobalLabel : public Inst {
public:
    std::string label;

    GlobalLabel(std::string label) :
        label(label) {
    }
    static GlobalLabelPtr create(std::string label) {
        return std::make_shared<GlobalLabel>(label);
    }

    std::string to_string() const override {
        return ".globl " + label;
    }

    std::set<Reg> get_uses() const override {
        return {};
    }
    std::set<Reg> get_defs() const override {
        return {};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

// 函数入口
class Function;
using FunctionPtr = std::shared_ptr<Function>;
class Function : public Inst {
public:
    std::string function;

    Function(std::string function) :
        function(function) {
    }
    static FunctionPtr create(std::string function) {
        return std::make_shared<Function>(function);
    }

    std::string to_string() const override {
        return function + ":";
    }

    std::set<Reg> get_uses() const override {
        return {};
    }
    std::set<Reg> get_defs() const override {
        return {};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

// .zero 指令
class Zero;
using ZeroPtr = std::shared_ptr<Zero>;
class Zero : public Inst {
public:
    int size;

    Zero(int size) :
        size(size) {
    }
    static ZeroPtr create(int size) {
        return std::make_shared<Zero>(size);
    }

    std::string to_string() const override {
        return ".zero " + std::to_string(size);
    }

    std::set<Reg> get_uses() const override {
        return {};
    }
    std::set<Reg> get_defs() const override {
        return {};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

// .word 指令
class Word;
using WordPtr = std::shared_ptr<Word>;
class Word : public Inst {
public:
    int value;

    Word(int value) :
        value(value) {
    }
    static WordPtr create(int value) {
        return std::make_shared<Word>(value);
    }

    std::string to_string() const override {
        return ".word " + std::to_string(value);
    }

    std::set<Reg> get_uses() const override {
        return {};
    }
    std::set<Reg> get_defs() const override {
        return {};
    }
    void replace_uses(const RegMap &reg_map) override {
    }
    void replace_defs(const RegMap &reg_map) override {
    }
};

using Code = std::list<InstPtr>;

} // namespace ASM

inline std::ostream &operator<<(std::ostream &os, const ASM::Code &code) {
    for (const auto &inst : code) {
        if (auto label = std::dynamic_pointer_cast<ASM::Label>(inst)) {
            os << label->to_string() << std::endl;
        } else {
            os << "    " << inst->to_string() << std::endl;
        }
    }
    return os;
}

#endif // CODEGEN_ASM_HPP