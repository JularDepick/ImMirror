#include "compiler.h"

#include <cstring>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "lexer.h"
#include "parser.h"

namespace immirror::script {

namespace {

/* 单个函数内允许累计的编译错误上限 */
const size_t kMaxCompileErrors = 8;

/* 局部变量: 名字, 寄存器, 作用域深度, 是否只读 */
struct LocalVar {
    std::string name;
    int reg = 0;
    int depth = 0;
    bool isConst = false;
};

/* 循环上下文, 记录待回填的 break 与 continue 跳转 */
struct LoopContext {
    std::vector<uint32_t> breaks;
    std::vector<uint32_t> continues;
};

/* 赋值目标地址, 先求地址再求右值 */
struct TargetRef {
    enum class Kind {
        Local = 0,
        Global,
        Indexed,
        Field,
    };
    Kind kind = Kind::Local;
    int localReg = 0;
    bool isConst = false;
    uint32_t nameConstant = 0;
    int tableReg = 0;
    int keyReg = 0;
    uint32_t fieldConstant = 0;
};

/* 函数编译状态, 与 Proto 一一对应 */
struct FuncState {
    Proto* proto = nullptr;
    FuncState* parent = nullptr;
    int numActive = 0;
    int maxRegs = 0;
    int scopeDepth = 1;
    std::vector<LocalVar> locals;
    std::vector<LoopContext> loops;

    /* 常量去重表 */
    std::unordered_map<std::string, int> stringConstants;
    std::unordered_map<int64_t, int> intConstants;
    std::unordered_map<uint64_t, int> doubleConstants;
    std::unordered_map<uint32_t, int> protoConstants;
    int nilConstant = -1;
    int trueConstant = -1;
    int falseConstant = -1;
};

/* 表达式编译期间临时占用的寄存器在离开作用域时自动归还 */
struct RegScope {
    FuncState& state;
    int saved;
    explicit RegScope(FuncState& input) : state(input), saved(input.numActive) {}
    ~RegScope() { state.numActive = saved; }
};

class Compiler {
public:
    CompileResult Run(const ast::Module& module, const std::string& chunkName,
                      const CompileOptions& options);

private:
    FuncState& Fs() { return *stack_.back(); }
    const FuncState& Fs() const { return *stack_.back(); }

    bool Error(uint32_t line, const std::string& message);

    uint32_t EmitABC(Op op, uint32_t a, uint32_t b, uint32_t c, uint32_t line);
    uint32_t EmitABx(Op op, uint32_t a, uint32_t bx, uint32_t line);
    uint32_t EmitAsBx(Op op, uint32_t a, int32_t sbx, uint32_t line);
    uint32_t EmitJumpPlaceholder(Op op, uint32_t a, uint32_t line);
    void PatchJumpTo(uint32_t at, uint32_t target);
    uint32_t Here() const;

    int Reserve(int count);
    void ReleaseTo(int reg);
    void MoveRange(int dst, int src, int count, uint32_t line);

    void BeginScope();
    void EndScope();
    void DeclareLocalAt(const std::string& name, int reg, bool isConst);
    const LocalVar* FindLocalVar(const FuncState& state, const std::string& name) const;
    bool IsCapturedFromParent(const std::string& name) const;

    uint32_t AddConstant(const Constant& constant);
    uint32_t AddStringConstant(const std::string& text);
    uint32_t ReserveProto();

    bool CompileStatements(const std::vector<ast::StmtPtr>& body);
    bool CompileStatement(const ast::Stmt& stmt);
    bool CompileVarDecl(const ast::Stmt& stmt);
    bool CompileAssign(const ast::Stmt& stmt);
    bool CompileFunctionDecl(const ast::Stmt& stmt);
    bool CompileIf(const ast::Stmt& stmt);
    bool CompileWhile(const ast::Stmt& stmt);
    bool CompileFor(const ast::Stmt& stmt);
    bool CompileReturn(const ast::Stmt& stmt);
    bool CompileLoopJump(Op op, uint32_t line);

    int CompileExpr(const ast::Expr& expr);
    void CompileExprInto(const ast::Expr& expr, int target);
    int CompileCall(const ast::Expr& call, int nresults);
    void CompileTable(const ast::Expr& expr, int target);
    bool CompileFunctionInto(uint32_t protoIndex, const ast::FunctionDef& def,
                             const std::string& name);

    std::deque<Proto> protos_;
    std::vector<std::unique_ptr<FuncState>> owned_;
    std::vector<FuncState*> stack_;
    std::vector<Diagnostic> errors_;
    CompileOptions options_;
    std::string chunkName_;
    bool failed_ = false;
    uint32_t currentLine_ = 1;
};

bool Compiler::Error(uint32_t line, const std::string& message) {
    if (errors_.size() < kMaxCompileErrors) {
        Diagnostic diagnostic;
        diagnostic.line = line;
        diagnostic.column = 0;
        diagnostic.message = message;
        errors_.push_back(diagnostic);
    }
    failed_ = true;
    return false;
}

uint32_t Compiler::EmitABC(Op op, uint32_t a, uint32_t b, uint32_t c, uint32_t line) {
    Proto& proto = *Fs().proto;
    proto.code.push_back(EncodeABC(op, a, b, c));
    if (!options_.stripLineInfo) {
        proto.lines.push_back(line);
    }
    currentLine_ = line;
    return static_cast<uint32_t>(proto.code.size() - 1);
}

uint32_t Compiler::EmitABx(Op op, uint32_t a, uint32_t bx, uint32_t line) {
    Proto& proto = *Fs().proto;
    proto.code.push_back(EncodeABx(op, a, bx));
    if (!options_.stripLineInfo) {
        proto.lines.push_back(line);
    }
    currentLine_ = line;
    return static_cast<uint32_t>(proto.code.size() - 1);
}

uint32_t Compiler::EmitAsBx(Op op, uint32_t a, int32_t sbx, uint32_t line) {
    Proto& proto = *Fs().proto;
    proto.code.push_back(EncodeAsBx(op, a, sbx));
    if (!options_.stripLineInfo) {
        proto.lines.push_back(line);
    }
    currentLine_ = line;
    return static_cast<uint32_t>(proto.code.size() - 1);
}

uint32_t Compiler::EmitJumpPlaceholder(Op op, uint32_t a, uint32_t line) {
    return EmitAsBx(op, a, 0, line);
}

uint32_t Compiler::Here() const { return static_cast<uint32_t>(Fs().proto->code.size()); }

void Compiler::PatchJumpTo(uint32_t at, uint32_t target) {
    Proto& proto = *Fs().proto;
    if (at >= proto.code.size()) {
        Error(currentLine_, "内部错误: 跳转回填位置越界");
        return;
    }
    const int64_t offset = static_cast<int64_t>(target) - static_cast<int64_t>(at) - 1;
    if (offset > kMaxJumpOffset || offset < -kMaxJumpOffset) {
        Error(proto.lines[at], "跳转距离超出 16 位有符号范围, 函数体过大");
        return;
    }
    const uint32_t word = proto.code[at];
    proto.code[at] = EncodeAsBx(InstructionOp(word), OperandA(word), static_cast<int32_t>(offset));
}

int Compiler::Reserve(int count) {
    FuncState& fs = Fs();
    const int base = fs.numActive;
    fs.numActive += count;
    if (fs.numActive > fs.maxRegs) {
        fs.maxRegs = fs.numActive;
    }
    if (fs.numActive > static_cast<int>(kMaxRegisters)) {
        Error(currentLine_, "寄存器用量超出上限, 请拆分函数或表达式");
    }
    return base;
}

void Compiler::ReleaseTo(int reg) {
    if (reg < 0) {
        reg = 0;
    }
    Fs().numActive = reg;
}

void Compiler::MoveRange(int dst, int src, int count, uint32_t line) {
    if (dst < src) {
        for (int i = 0; i < count; ++i) {
            if (dst + i != src + i) {
                EmitABC(Op::Move, static_cast<uint32_t>(dst + i), static_cast<uint32_t>(src + i), 0,
                        line);
            }
        }
        return;
    }
    for (int i = count - 1; i >= 0; --i) {
        if (dst + i != src + i) {
            EmitABC(Op::Move, static_cast<uint32_t>(dst + i), static_cast<uint32_t>(src + i), 0, line);
        }
    }
}

void Compiler::BeginScope() { ++Fs().scopeDepth; }

void Compiler::EndScope() {
    FuncState& fs = Fs();
    int newTop = fs.numActive;
    while (!fs.locals.empty() && fs.locals.back().depth >= fs.scopeDepth) {
        newTop = fs.locals.back().reg;
        fs.locals.pop_back();
    }
    fs.numActive = newTop;
    if (fs.scopeDepth > 1) {
        --fs.scopeDepth;
    }
}

void Compiler::DeclareLocalAt(const std::string& name, int reg, bool isConst) {
    LocalVar local;
    local.name = name;
    local.reg = reg;
    local.depth = Fs().scopeDepth;
    local.isConst = isConst;
    Fs().locals.push_back(std::move(local));
}

const LocalVar* Compiler::FindLocalVar(const FuncState& state, const std::string& name) const {
    for (auto it = state.locals.rbegin(); it != state.locals.rend(); ++it) {
        if (it->name == name) {
            return &*it;
        }
    }
    return nullptr;
}

bool Compiler::IsCapturedFromParent(const std::string& name) const {
    const FuncState* state = Fs().parent;
    while (state != nullptr) {
        if (FindLocalVar(*state, name) != nullptr) {
            return true;
        }
        state = state->parent;
    }
    return false;
}

uint32_t Compiler::AddConstant(const Constant& constant) {
    FuncState& fs = Fs();
    Proto& proto = *fs.proto;
    int existing = -1;
    switch (constant.kind) {
        case ConstantKind::Nil:
            existing = fs.nilConstant;
            break;
        case ConstantKind::Bool:
            existing = constant.boolValue ? fs.trueConstant : fs.falseConstant;
            break;
        case ConstantKind::Int: {
            const auto it = fs.intConstants.find(constant.intValue);
            if (it != fs.intConstants.end()) {
                existing = it->second;
            }
            break;
        }
        case ConstantKind::Double: {
            uint64_t bits = 0;
            std::memcpy(&bits, &constant.doubleValue, sizeof(bits));
            const auto it = fs.doubleConstants.find(bits);
            if (it != fs.doubleConstants.end()) {
                existing = it->second;
            }
            break;
        }
        case ConstantKind::String: {
            const auto it = fs.stringConstants.find(constant.stringValue);
            if (it != fs.stringConstants.end()) {
                existing = it->second;
            }
            break;
        }
        case ConstantKind::Proto: {
            const auto it = fs.protoConstants.find(constant.protoIndex);
            if (it != fs.protoConstants.end()) {
                existing = it->second;
            }
            break;
        }
    }
    if (existing >= 0) {
        return static_cast<uint32_t>(existing);
    }
    if (proto.constants.size() >= kMaxConstants) {
        Error(currentLine_, "单个函数的常量数量超出上限");
        return 0;
    }
    const int index = static_cast<int>(proto.constants.size());
    proto.constants.push_back(constant);
    switch (constant.kind) {
        case ConstantKind::Nil:
            fs.nilConstant = index;
            break;
        case ConstantKind::Bool:
            if (constant.boolValue) {
                fs.trueConstant = index;
            } else {
                fs.falseConstant = index;
            }
            break;
        case ConstantKind::Int:
            fs.intConstants[constant.intValue] = index;
            break;
        case ConstantKind::Double: {
            uint64_t bits = 0;
            std::memcpy(&bits, &constant.doubleValue, sizeof(bits));
            fs.doubleConstants[bits] = index;
            break;
        }
        case ConstantKind::String:
            fs.stringConstants[constant.stringValue] = index;
            break;
        case ConstantKind::Proto:
            fs.protoConstants[constant.protoIndex] = index;
            break;
    }
    return static_cast<uint32_t>(index);
}

uint32_t Compiler::AddStringConstant(const std::string& text) {
    return AddConstant(Constant::MakeString(text));
}

uint32_t Compiler::ReserveProto() {
    protos_.emplace_back();
    return static_cast<uint32_t>(protos_.size() - 1);
}

int Compiler::CompileExpr(const ast::Expr& expr) {
    const int reg = Reserve(1);
    CompileExprInto(expr, reg);
    return reg;
}

int Compiler::CompileCall(const ast::Expr& call, int nresults) {
    const int nargs = static_cast<int>(call.args.size());
    const int base = Reserve(1 + nargs);
    CompileExprInto(*call.a, base);
    for (int i = 0; i < nargs; ++i) {
        CompileExprInto(*call.args[static_cast<size_t>(i)], base + 1 + i);
    }
    EmitABC(Op::Call, static_cast<uint32_t>(base), static_cast<uint32_t>(nargs),
            static_cast<uint32_t>(nresults), call.line);
    if (nresults > 0) {
        ReleaseTo(base + nresults);
    } else {
        ReleaseTo(base);
    }
    return base;
}

void Compiler::CompileTable(const ast::Expr& expr, int target) {
    size_t arrayHints = 0;
    size_t hashHints = 0;
    for (const ast::TableField& field : expr.fields) {
        if (field.kind == ast::TableField::Kind::Positional) {
            ++arrayHints;
        } else {
            ++hashHints;
        }
    }
    EmitABC(Op::NewTable, static_cast<uint32_t>(target), static_cast<uint32_t>(arrayHints & 0xFFu),
            static_cast<uint32_t>(hashHints & 0xFFu), expr.line);

    int nextIndex = 1;
    for (const ast::TableField& field : expr.fields) {
        if (failed_) {
            return;
        }
        switch (field.kind) {
            case ast::TableField::Kind::Positional: {
                const int keyReg = Reserve(1);
                if (nextIndex >= -32768 && nextIndex <= 32767) {
                    EmitAsBx(Op::LoadInt, static_cast<uint32_t>(keyReg), nextIndex, field.value->line);
                } else {
                    EmitABx(Op::LoadK, static_cast<uint32_t>(keyReg),
                            AddConstant(Constant::MakeInt(nextIndex)), field.value->line);
                }
                const int valueReg = CompileExpr(*field.value);
                EmitABC(Op::SetTable, static_cast<uint32_t>(target), static_cast<uint32_t>(keyReg),
                        static_cast<uint32_t>(valueReg), field.value->line);
                ReleaseTo(keyReg);
                ++nextIndex;
                break;
            }
            case ast::TableField::Kind::Named: {
                const uint32_t keyConstant = AddStringConstant(field.name);
                const int valueReg = CompileExpr(*field.value);
                if (keyConstant <= 0xFFu) {
                    EmitABC(Op::SetField, static_cast<uint32_t>(target),
                            static_cast<uint32_t>(valueReg), keyConstant, field.value->line);
                } else {
                    const int keyReg = Reserve(1);
                    EmitABx(Op::LoadK, static_cast<uint32_t>(keyReg), keyConstant, field.value->line);
                    EmitABC(Op::SetTable, static_cast<uint32_t>(target),
                            static_cast<uint32_t>(keyReg), static_cast<uint32_t>(valueReg),
                            field.value->line);
                    ReleaseTo(keyReg);
                }
                break;
            }
            case ast::TableField::Kind::Computed: {
                const int keyReg = CompileExpr(*field.key);
                const int valueReg = CompileExpr(*field.value);
                EmitABC(Op::SetTable, static_cast<uint32_t>(target), static_cast<uint32_t>(keyReg),
                        static_cast<uint32_t>(valueReg), field.value->line);
                break;
            }
        }
    }
}

void Compiler::CompileExprInto(const ast::Expr& expr, int target) {
    if (failed_) {
        return;
    }
    RegScope scope(Fs());
    const uint32_t line = expr.line;
    switch (expr.kind) {
        case ast::ExprKind::Nil:
            EmitABC(Op::LoadNil, static_cast<uint32_t>(target), 0, 0, line);
            break;
        case ast::ExprKind::Bool:
            EmitABC(Op::LoadBool, static_cast<uint32_t>(target), expr.boolValue ? 1u : 0u, 0, line);
            break;
        case ast::ExprKind::Int: {
            if (expr.intValue >= -32768 && expr.intValue <= 32767) {
                EmitAsBx(Op::LoadInt, static_cast<uint32_t>(target), static_cast<int32_t>(expr.intValue),
                         line);
            } else {
                const uint32_t index = AddConstant(Constant::MakeInt(expr.intValue));
                EmitABx(Op::LoadK, static_cast<uint32_t>(target), index, line);
            }
            break;
        }
        case ast::ExprKind::Float: {
            const uint32_t index = AddConstant(Constant::MakeDouble(expr.floatValue));
            EmitABx(Op::LoadK, static_cast<uint32_t>(target), index, line);
            break;
        }
        case ast::ExprKind::String: {
            const uint32_t index = AddStringConstant(expr.stringValue);
            EmitABx(Op::LoadK, static_cast<uint32_t>(target), index, line);
            break;
        }
        case ast::ExprKind::Name: {
            const LocalVar* local = FindLocalVar(Fs(), expr.stringValue);
            if (local != nullptr) {
                if (local->reg != target) {
                    EmitABC(Op::Move, static_cast<uint32_t>(target),
                            static_cast<uint32_t>(local->reg), 0, line);
                }
                break;
            }
            if (IsCapturedFromParent(expr.stringValue)) {
                Error(line, "嵌套函数暂不支持捕获外层局部变量: " + expr.stringValue);
                break;
            }
            const uint32_t index = AddStringConstant(expr.stringValue);
            EmitABx(Op::GetGlobal, static_cast<uint32_t>(target), index, line);
            break;
        }
        case ast::ExprKind::Unary: {
            const int operand = CompileExpr(*expr.a);
            EmitABC(expr.unaryOp == ast::UnaryOp::Neg ? Op::Unm : Op::Not,
                    static_cast<uint32_t>(target), static_cast<uint32_t>(operand), 0, line);
            break;
        }
        case ast::ExprKind::Binary: {
            if (expr.binaryOp == ast::BinaryOp::And || expr.binaryOp == ast::BinaryOp::Or) {
                CompileExprInto(*expr.a, target);
                const Op jumpOp = expr.binaryOp == ast::BinaryOp::And ? Op::JmpIfNot : Op::JmpIf;
                const uint32_t jump =
                    EmitJumpPlaceholder(jumpOp, static_cast<uint32_t>(target), line);
                CompileExprInto(*expr.b, target);
                PatchJumpTo(jump, Here());
                break;
            }
            Op op = Op::Add;
            switch (expr.binaryOp) {
                case ast::BinaryOp::Add: op = Op::Add; break;
                case ast::BinaryOp::Sub: op = Op::Sub; break;
                case ast::BinaryOp::Mul: op = Op::Mul; break;
                case ast::BinaryOp::Div: op = Op::Div; break;
                case ast::BinaryOp::Mod: op = Op::Mod; break;
                case ast::BinaryOp::Eq: op = Op::Eq; break;
                case ast::BinaryOp::Ne: op = Op::Ne; break;
                case ast::BinaryOp::Lt: op = Op::Lt; break;
                case ast::BinaryOp::Le: op = Op::Le; break;
                case ast::BinaryOp::Gt: op = Op::Gt; break;
                case ast::BinaryOp::Ge: op = Op::Ge; break;
                case ast::BinaryOp::And:
                case ast::BinaryOp::Or:
                    break;
            }
            const int left = CompileExpr(*expr.a);
            const int right = CompileExpr(*expr.b);
            EmitABC(op, static_cast<uint32_t>(target), static_cast<uint32_t>(left),
                    static_cast<uint32_t>(right), line);
            break;
        }
        case ast::ExprKind::Index: {
            const int object = CompileExpr(*expr.a);
            const int key = CompileExpr(*expr.b);
            EmitABC(Op::GetTable, static_cast<uint32_t>(target), static_cast<uint32_t>(object),
                    static_cast<uint32_t>(key), line);
            break;
        }
        case ast::ExprKind::Field: {
            const int object = CompileExpr(*expr.a);
            const uint32_t index = AddStringConstant(expr.stringValue);
            if (index <= 0xFFu) {
                EmitABC(Op::GetField, static_cast<uint32_t>(target), static_cast<uint32_t>(object),
                        index, line);
            } else {
                const int keyReg = Reserve(1);
                EmitABx(Op::LoadK, static_cast<uint32_t>(keyReg), index, line);
                EmitABC(Op::GetTable, static_cast<uint32_t>(target), static_cast<uint32_t>(object),
                        static_cast<uint32_t>(keyReg), line);
            }
            break;
        }
        case ast::ExprKind::Call: {
            const int base = CompileCall(expr, 1);
            if (base != target) {
                EmitABC(Op::Move, static_cast<uint32_t>(target), static_cast<uint32_t>(base), 0, line);
            }
            break;
        }
        case ast::ExprKind::Table:
            CompileTable(expr, target);
            break;
        case ast::ExprKind::Function: {
            const uint32_t protoIndex = ReserveProto();
            const uint32_t constantIndex = AddConstant(Constant::MakeProto(protoIndex));
            const std::string name = expr.func->name.empty() ? "<匿名>" : expr.func->name;
            if (!CompileFunctionInto(protoIndex, *expr.func, name)) {
                break;
            }
            EmitABx(Op::Closure, static_cast<uint32_t>(target), constantIndex, line);
            break;
        }
    }
}

bool Compiler::CompileStatements(const std::vector<ast::StmtPtr>& body) {
    for (const ast::StmtPtr& statement : body) {
        if (failed_) {
            return false;
        }
        if (statement && !CompileStatement(*statement)) {
            return false;
        }
    }
    return !failed_;
}

bool Compiler::CompileStatement(const ast::Stmt& stmt) {
    if (failed_) {
        return false;
    }
    switch (stmt.kind) {
        case ast::StmtKind::Expr: {
            const int reg = CompileExpr(*stmt.exprs[0]);
            ReleaseTo(reg);
            return !failed_;
        }
        case ast::StmtKind::VarDecl:
            return CompileVarDecl(stmt);
        case ast::StmtKind::Assign:
            return CompileAssign(stmt);
        case ast::StmtKind::FunctionDecl:
            return CompileFunctionDecl(stmt);
        case ast::StmtKind::If:
            return CompileIf(stmt);
        case ast::StmtKind::While:
            return CompileWhile(stmt);
        case ast::StmtKind::For:
            return CompileFor(stmt);
        case ast::StmtKind::Break:
            return CompileLoopJump(Op::Jmp, stmt.line);
        case ast::StmtKind::Continue:
            return CompileLoopJump(Op::Jmp, stmt.line);
        case ast::StmtKind::Return:
            return CompileReturn(stmt);
        case ast::StmtKind::Block: {
            BeginScope();
            const bool ok = CompileStatements(stmt.body);
            EndScope();
            return ok;
        }
    }
    return Error(stmt.line, "未知的语句种类");
}

bool Compiler::CompileLoopJump(Op op, uint32_t line) {
    (void)op;
    return Error(line, "内部错误: 循环跳转未指定类型");
}

bool Compiler::CompileVarDecl(const ast::Stmt& stmt) {
    const int count = static_cast<int>(stmt.names.size());
    const int base = Reserve(count);
    const size_t initCount = stmt.exprs.size();

    if (initCount == 0) {
        for (int i = 0; i < count; ++i) {
            EmitABC(Op::LoadNil, static_cast<uint32_t>(base + i), 0, 0, stmt.line);
        }
    } else if (initCount == 1 && count > 1) {
        if (stmt.exprs[0]->kind == ast::ExprKind::Call) {
            const int callBase = CompileCall(*stmt.exprs[0], count);
            MoveRange(base, callBase, count, stmt.line);
            ReleaseTo(base + count);
        } else {
            CompileExprInto(*stmt.exprs[0], base);
            for (int i = 1; i < count; ++i) {
                EmitABC(Op::LoadNil, static_cast<uint32_t>(base + i), 0, 0, stmt.line);
            }
        }
    } else if (static_cast<int>(initCount) == count) {
        for (int i = 0; i < count; ++i) {
            CompileExprInto(*stmt.exprs[static_cast<size_t>(i)], base + i);
        }
    } else {
        return Error(stmt.line, "变量声明的名字与初始化表达式数量不一致");
    }

    for (int i = 0; i < count; ++i) {
        DeclareLocalAt(stmt.names[static_cast<size_t>(i)], base + i, stmt.isConst);
    }
    ReleaseTo(base + count);
    return !failed_;
}

bool Compiler::CompileAssign(const ast::Stmt& stmt) {
    const int saved = Fs().numActive;
    const int count = static_cast<int>(stmt.targets.size());
    std::vector<TargetRef> refs;
    refs.reserve(static_cast<size_t>(count));

    /* 先求全部目标地址, 保证 a[i], a[j] = ... 这类写法读到的是赋值前的下标 */
    for (int i = 0; i < count; ++i) {
        const ast::Expr& target = *stmt.targets[static_cast<size_t>(i)];
        TargetRef ref;
        if (target.kind == ast::ExprKind::Name) {
            const LocalVar* local = FindLocalVar(Fs(), target.stringValue);
            if (local != nullptr) {
                ref.kind = TargetRef::Kind::Local;
                ref.localReg = local->reg;
                ref.isConst = local->isConst;
            } else if (IsCapturedFromParent(target.stringValue)) {
                Error(target.line, "嵌套函数暂不支持捕获外层局部变量: " + target.stringValue);
                return false;
            } else {
                ref.kind = TargetRef::Kind::Global;
                ref.nameConstant = AddStringConstant(target.stringValue);
            }
        } else if (target.kind == ast::ExprKind::Index) {
            ref.kind = TargetRef::Kind::Indexed;
            ref.tableReg = CompileExpr(*target.a);
            ref.keyReg = CompileExpr(*target.b);
        } else if (target.kind == ast::ExprKind::Field) {
            ref.kind = TargetRef::Kind::Field;
            ref.tableReg = CompileExpr(*target.a);
            ref.fieldConstant = AddStringConstant(target.stringValue);
        } else {
            return Error(target.line, "赋值目标只能是变量, 字段或下标");
        }
        refs.push_back(ref);
    }

    const int rhsBase = Reserve(count);
    if (stmt.exprs.size() == 1 && count > 1 && stmt.exprs[0]->kind == ast::ExprKind::Call) {
        const int callBase = CompileCall(*stmt.exprs[0], count);
        MoveRange(rhsBase, callBase, count, stmt.line);
    } else if (static_cast<int>(stmt.exprs.size()) == count) {
        for (int i = 0; i < count; ++i) {
            CompileExprInto(*stmt.exprs[static_cast<size_t>(i)], rhsBase + i);
        }
    } else {
        return Error(stmt.line, "赋值左右两边的数量不一致");
    }

    for (int i = 0; i < count; ++i) {
        const int valueReg = rhsBase + i;
        const TargetRef& ref = refs[static_cast<size_t>(i)];
        switch (ref.kind) {
            case TargetRef::Kind::Local:
                if (ref.isConst) {
                    return Error(stmt.line, "不能给 const 变量赋值");
                }
                if (ref.localReg != valueReg) {
                    EmitABC(Op::Move, static_cast<uint32_t>(ref.localReg),
                            static_cast<uint32_t>(valueReg), 0, stmt.line);
                }
                break;
            case TargetRef::Kind::Global:
                EmitABx(Op::SetGlobal, static_cast<uint32_t>(valueReg), ref.nameConstant, stmt.line);
                break;
            case TargetRef::Kind::Indexed:
                EmitABC(Op::SetTable, static_cast<uint32_t>(ref.tableReg),
                        static_cast<uint32_t>(ref.keyReg), static_cast<uint32_t>(valueReg), stmt.line);
                break;
            case TargetRef::Kind::Field:
                if (ref.fieldConstant <= 0xFFu) {
                    EmitABC(Op::SetField, static_cast<uint32_t>(ref.tableReg),
                            static_cast<uint32_t>(valueReg), ref.fieldConstant, stmt.line);
                } else {
                    const int keyReg = Reserve(1);
                    EmitABx(Op::LoadK, static_cast<uint32_t>(keyReg), ref.fieldConstant, stmt.line);
                    EmitABC(Op::SetTable, static_cast<uint32_t>(ref.tableReg),
                            static_cast<uint32_t>(keyReg), static_cast<uint32_t>(valueReg),
                            stmt.line);
                    ReleaseTo(keyReg);
                }
                break;
        }
    }
    ReleaseTo(saved);
    return !failed_;
}

/*
 * 函数声明. 主代码块顶层的函数登记为全局, 便于函数之间相互调用;
 * 函数体内部的嵌套函数仍是局部闭包.
 */
bool Compiler::CompileFunctionDecl(const ast::Stmt& stmt) {
    const bool atChunkTop = Fs().parent == nullptr;
    const uint32_t protoIndex = ReserveProto();
    const uint32_t constantIndex = AddConstant(Constant::MakeProto(protoIndex));
    if (!CompileFunctionInto(protoIndex, *stmt.func, stmt.func->name)) {
        return false;
    }
    const int reg = Reserve(1);
    EmitABx(Op::Closure, static_cast<uint32_t>(reg), constantIndex, stmt.line);
    if (atChunkTop) {
        const uint32_t nameConstant = AddStringConstant(stmt.func->name);
        EmitABx(Op::SetGlobal, static_cast<uint32_t>(reg), nameConstant, stmt.line);
    } else {
        DeclareLocalAt(stmt.func->name, reg, false);
    }
    ReleaseTo(reg);
    return true;
}

bool Compiler::CompileIf(const ast::Stmt& stmt) {
    std::vector<uint32_t> endJumps;
    for (size_t i = 0; i < stmt.branches.size(); ++i) {
        const ast::IfBranch& branch = stmt.branches[i];
        const int condReg = CompileExpr(*branch.cond);
        const uint32_t skip = EmitJumpPlaceholder(Op::JmpIfNot, static_cast<uint32_t>(condReg),
                                                  branch.cond->line);
        ReleaseTo(condReg);
        BeginScope();
        const bool ok = CompileStatements(branch.body);
        EndScope();
        if (!ok) {
            return false;
        }
        const bool isLast = (i + 1 == stmt.branches.size()) && stmt.elseBody.empty();
        if (!isLast) {
            endJumps.push_back(EmitJumpPlaceholder(Op::Jmp, 0, branch.cond->line));
        }
        PatchJumpTo(skip, Here());
    }
    if (!stmt.elseBody.empty()) {
        BeginScope();
        const bool ok = CompileStatements(stmt.elseBody);
        EndScope();
        if (!ok) {
            return false;
        }
    }
    const uint32_t end = Here();
    for (uint32_t jump : endJumps) {
        PatchJumpTo(jump, end);
    }
    return !failed_;
}

bool Compiler::CompileWhile(const ast::Stmt& stmt) {
    const uint32_t loopStart = Here();
    const int condReg = CompileExpr(*stmt.cond);
    const uint32_t exitJump =
        EmitJumpPlaceholder(Op::JmpIfNot, static_cast<uint32_t>(condReg), stmt.cond->line);
    ReleaseTo(condReg);

    Fs().loops.emplace_back();
    BeginScope();
    const bool ok = CompileStatements(stmt.body);
    EndScope();
    if (!ok) {
        Fs().loops.pop_back();
        return false;
    }
    LoopContext loop = std::move(Fs().loops.back());
    Fs().loops.pop_back();

    for (uint32_t jump : loop.continues) {
        PatchJumpTo(jump, loopStart);
    }
    const uint32_t backJump = EmitJumpPlaceholder(Op::Jmp, 0, stmt.line);
    PatchJumpTo(backJump, loopStart);
    PatchJumpTo(exitJump, Here());
    for (uint32_t jump : loop.breaks) {
        PatchJumpTo(jump, Here());
    }
    return !failed_;
}

bool Compiler::CompileFor(const ast::Stmt& stmt) {
    BeginScope();
    if (stmt.forInit && !CompileStatement(*stmt.forInit)) {
        EndScope();
        return false;
    }
    const uint32_t loopStart = Here();
    uint32_t exitJump = 0;
    bool hasExit = false;
    if (stmt.forCond) {
        const int condReg = CompileExpr(*stmt.forCond);
        exitJump = EmitJumpPlaceholder(Op::JmpIfNot, static_cast<uint32_t>(condReg), stmt.forCond->line);
        ReleaseTo(condReg);
        hasExit = true;
    }

    Fs().loops.emplace_back();
    BeginScope();
    const bool ok = CompileStatements(stmt.body);
    EndScope();
    if (!ok) {
        Fs().loops.pop_back();
        EndScope();
        return false;
    }

    const uint32_t continueTarget = Here();
    if (stmt.forStep) {
        const int saved = Fs().numActive;
        if (!CompileStatement(*stmt.forStep)) {
            Fs().loops.pop_back();
            EndScope();
            return false;
        }
        ReleaseTo(saved);
    }
    LoopContext loop = std::move(Fs().loops.back());
    Fs().loops.pop_back();

    for (uint32_t jump : loop.continues) {
        PatchJumpTo(jump, continueTarget);
    }
    const uint32_t backJump = EmitJumpPlaceholder(Op::Jmp, 0, stmt.line);
    PatchJumpTo(backJump, loopStart);
    if (hasExit) {
        PatchJumpTo(exitJump, Here());
    }
    for (uint32_t jump : loop.breaks) {
        PatchJumpTo(jump, Here());
    }
    EndScope();
    return !failed_;
}

bool Compiler::CompileReturn(const ast::Stmt& stmt) {
    if (stmt.exprs.empty()) {
        EmitABC(Op::ReturnZero, 0, 0, 0, stmt.line);
        return true;
    }
    if (stmt.exprs.size() == 1 && stmt.exprs[0]->kind == ast::ExprKind::Call) {
        const int base = CompileCall(*stmt.exprs[0], 0);
        EmitABC(Op::ReturnMulti, static_cast<uint32_t>(base), 0, 0, stmt.line);
        ReleaseTo(base);
        return !failed_;
    }
    const int count = static_cast<int>(stmt.exprs.size());
    const int base = Reserve(count);
    for (int i = 0; i < count; ++i) {
        CompileExprInto(*stmt.exprs[static_cast<size_t>(i)], base + i);
    }
    EmitABC(Op::Return, static_cast<uint32_t>(base), static_cast<uint32_t>(count), 0, stmt.line);
    ReleaseTo(base);
    return !failed_;
}

bool Compiler::CompileFunctionInto(uint32_t protoIndex, const ast::FunctionDef& def,
                                   const std::string& name) {
    if (def.params.size() > kMaxParams) {
        return Error(def.line, "函数参数个数超出上限");
    }
    Proto& proto = protos_[protoIndex];
    proto.name = name;
    proto.line = def.line;
    proto.numParams = static_cast<uint32_t>(def.params.size());

    auto state = std::unique_ptr<FuncState>(new FuncState());
    state->proto = &proto;
    state->parent = Fs().parent == nullptr && stack_.empty() ? nullptr : stack_.back();
    state->numActive = static_cast<int>(def.params.size());
    state->maxRegs = state->numActive;
    state->scopeDepth = 1;
    owned_.push_back(std::move(state));
    FuncState* fs = owned_.back().get();
    stack_.push_back(fs);

    for (size_t i = 0; i < def.params.size(); ++i) {
        LocalVar local;
        local.name = def.params[i];
        local.reg = static_cast<int>(i);
        local.depth = 1;
        local.isConst = false;
        fs->locals.push_back(std::move(local));
    }

    for (const ast::StmtPtr& statement : def.body) {
        if (failed_) {
            break;
        }
        if (statement && !CompileStatement(*statement)) {
            break;
        }
    }
    if (!options_.stripLineInfo) {
        proto.lines.push_back(def.line);
    }
    proto.code.push_back(EncodeABC(Op::ReturnZero, 0, 0, 0));
    if (fs->maxRegs < 1) {
        fs->maxRegs = 1;
    }
    proto.numRegs = static_cast<uint32_t>(fs->maxRegs);
    stack_.pop_back();
    Fs().proto->children.push_back(protoIndex);
    return !failed_;
}

CompileResult Compiler::Run(const ast::Module& module, const std::string& chunkName,
                            const CompileOptions& options) {
    options_ = options;
    chunkName_ = chunkName;
    protos_.clear();
    owned_.clear();
    stack_.clear();
    errors_.clear();
    failed_ = false;
    currentLine_ = 1;

    protos_.emplace_back();
    Proto& mainProto = protos_[0];
    mainProto.name = "main";
    mainProto.line = 1;
    mainProto.numParams = 0;
    mainProto.numRegs = 1;

    auto mainState = std::unique_ptr<FuncState>(new FuncState());
    mainState->proto = &mainProto;
    mainState->parent = nullptr;
    mainState->numActive = 0;
    mainState->maxRegs = 0;
    mainState->scopeDepth = 1;
    owned_.push_back(std::move(mainState));
    stack_.push_back(owned_.back().get());

    for (const ast::StmtPtr& statement : module.statements) {
        if (failed_) {
            break;
        }
        if (statement && !CompileStatement(*statement)) {
            break;
        }
    }

    if (!options_.stripLineInfo) {
        mainProto.lines.push_back(1);
    }
    mainProto.code.push_back(EncodeABC(Op::ReturnZero, 0, 0, 0));
    mainProto.numRegs = static_cast<uint32_t>(Fs().maxRegs < 1 ? 1 : Fs().maxRegs);
    stack_.clear();

    CompileResult result;
    result.diagnostics = errors_;
    if (failed_ || !errors_.empty()) {
        result.ok = false;
        return result;
    }

    result.program.chunkName = chunkName;
    result.program.protos.assign(protos_.begin(), protos_.end());
    result.program.entry = 0;

    BytecodeStatus status;
    if (!VerifyProgram(result.program, status)) {
        Diagnostic diagnostic;
        diagnostic.line = 0;
        diagnostic.column = 0;
        diagnostic.message = std::string("编译器生成的字节码未通过校验: ") + status.message;
        result.diagnostics.push_back(diagnostic);
        result.ok = false;
        return result;
    }
    result.ok = true;
    return result;
}

}  /* namespace */

CompileResult CompileModule(const ast::Module& module, const std::string& chunkName,
                            const CompileOptions& options) {
    Compiler compiler;
    return compiler.Run(module, chunkName, options);
}

CompileResult CompileSource(const std::string& source, const std::string& chunkName,
                            const CompileOptions& options) {
    CompileResult result;
    Lexer lexer(source);
    std::vector<Token> tokens;
    Diagnostic error;
    if (!lexer.Tokenize(tokens, error)) {
        result.diagnostics.push_back(std::move(error));
        return result;
    }
    Parser parser(std::move(tokens), chunkName);
    ast::Module module;
    std::vector<Diagnostic> parseErrors;
    if (!parser.ParseModule(module, parseErrors)) {
        result.diagnostics = std::move(parseErrors);
        return result;
    }
    return CompileModule(module, chunkName, options);
}

}  /* namespace immirror::script */
