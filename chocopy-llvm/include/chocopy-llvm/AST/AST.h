#ifndef CHOCOPY_LLVM_AST_AST_H
#define CHOCOPY_LLVM_AST_AST_H

#include "chocopy-llvm/Basic/LLVM.h"
#include "chocopy-llvm/Basic/SymbolTable.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/SMLoc.h>

namespace chocopy {

using llvm::SMRange;

class ASTContext;

class Identifier;
class Declaration;
class Program;
class ClassDef;
class FuncDef;
class GlobalDecl;
class NonLocalDecl;
class ParamDecl;
class VarDef;
class TypeAnnotation;
class ClassType;
class ListType;
class Stmt;
class AssignStmt;
class ExprStmt;
class ForStmt;
class IfStmt;
class ReturnStmt;
class WhileStmt;
class Expr;
class BinaryExpr;
class CallExpr;
class DeclRef;
class IfExpr;
class IndexExpr;
class ListExpr;
class Literal;
class LiteralType;
class BooleanLiteral;
class IntegerLiteral;
class NoneLiteral;
class StringLiteral;
class MemberExpr;
class MethodCallExpr;
class UnaryExpr;

class Type;

using DeclList = SmallVector<Declaration *>;
using ExprList = SmallVector<Expr *>;
using StmtList = SmallVector<Stmt *>;
using ParamDeclList = SmallVector<ParamDecl *>;

class alignas(void *) Identifier {
  friend ASTContext;

public:
  SMRange getLocation() const { return Loc; }

  SymbolInfo *getSymbolInfo() const { return Name; }
  StringRef getName() const { return Name->getName(); }

private:
  Identifier(SMRange Loc, SymbolInfo *Name) : Name(Name), Loc(Loc) {}

private:
  void operator delete(void *) {
    llvm_unreachable("AST nodes deletion is prohibited");
  }

private:
  SymbolInfo *Name;
  SMRange Loc;
};

class alignas(void *) Declaration {
public:
  enum class DeclKind {
    ClassDef,
    FuncDef,
    GlobalDecl,
    NonLocalDecl,
    ParamDecl,
    VarDef
  };

  DeclKind getKind() const { return Kind; }

  SMRange getLocation() const { return Loc; }

  Identifier *getNameId() const { return Name; }
  SymbolInfo *getSymbolInfo() const { return Name->getSymbolInfo(); }
  StringRef getName() const { return Name->getName(); }

  void dump(ASTContext &C) const;

protected:
  Declaration(SMRange Loc, DeclKind Kind, Identifier *Name)
      : Kind(Kind), Loc(Loc), Name(Name) {}

private:
  void operator delete(void *) {
    llvm_unreachable("AST nodes deletion is prohibited");
  }

private:
  DeclKind Kind;
  SMRange Loc;

  /* Defined name. */
  Identifier *Name;
};

class alignas(void *) Program {
  friend ASTContext;

public:
  ArrayRef<Declaration *> getDeclarations() const { return Declarations; }
  ArrayRef<Stmt *> getStatements() const { return Statements; }

public:
  void dump(ASTContext &C) const;

private:
  void operator delete(void *) {
    llvm_unreachable("AST nodes deletion is prohibited");
  }

private:
  Program(ArrayRef<Declaration *> Decls, ArrayRef<Stmt *> Stmts)
      : Declarations(Decls), Statements(Stmts) {}

  /** Initial variable, class, and function declarations. */
  DeclList Declarations;
  /** Trailing statements. */
  StmtList Statements;
};

class ClassDef : public Declaration {
  friend ASTContext;

public:
  Identifier *getSuperClass() const { return SuperClass; }
  ArrayRef<Declaration *> getDeclarations() const { return Declarations; }

public:
  static bool classof(const Declaration *D) {
    return D->getKind() == DeclKind::ClassDef;
  }

private:
  ClassDef(SMRange Loc, Identifier *Name, Identifier *SuperClass,
           ArrayRef<Declaration *> Declarations)
      : Declaration(Loc, DeclKind::ClassDef, Name), SuperClass(SuperClass),
        Declarations(Declarations) {}

private:
  /** Name of the parent class. */
  Identifier *SuperClass;
  /** Body of the class. */
  DeclList Declarations;
};

class FuncDef : public Declaration {
  friend ASTContext;

public:
  ArrayRef<ParamDecl *> getParams() const { return Params; };
  TypeAnnotation *getReturnType() const { return ReturnType; };
  ArrayRef<Declaration *> getDeclarations() const { return Declarations; };
  ArrayRef<Stmt *> getStatements() const { return Statements; };

public:
  static bool classof(const Declaration *D) {
    return D->getKind() == DeclKind::FuncDef;
  }

private:
  FuncDef(SMRange Loc, Identifier *Name, ArrayRef<ParamDecl *> Params,
          TypeAnnotation *ReturnType, ArrayRef<Declaration *> Declarations,
          ArrayRef<Stmt *> Statements)
      : Declaration(Loc, DeclKind::FuncDef, Name), Params(Params),
        ReturnType(ReturnType), Declarations(Declarations),
        Statements(Statements) {}

private:
  /** Formal parameters. */
  ParamDeclList Params;
  /** Return type annotation. */
  TypeAnnotation *ReturnType;
  /** Local-variable,inner-function, global, and nonlocal declarations. */
  DeclList Declarations;
  /** Other statements. */
  StmtList Statements;
};

class GlobalDecl : public Declaration {
  friend ASTContext;

public:
  static bool classof(const Declaration *D) {
    return D->getKind() == DeclKind::GlobalDecl;
  }

private:
  GlobalDecl(SMRange Loc, Identifier *Name)
      : Declaration(Loc, DeclKind::GlobalDecl, Name) {}
};

class NonLocalDecl : public Declaration {
  friend ASTContext;

public:
  static bool classof(const Declaration *D) {
    return D->getKind() == DeclKind::NonLocalDecl;
  }

private:
  NonLocalDecl(SMRange Loc, Identifier *Name)
      : Declaration(Loc, DeclKind::NonLocalDecl, Name) {}
};

class ParamDecl : public Declaration {
  friend ASTContext;

public:
  TypeAnnotation *getType() const { return Type; }

public:
  static bool classof(const Declaration *D) {
    return D->getKind() == DeclKind::ParamDecl;
  }

private:
  ParamDecl(SMRange Loc, Identifier *Name, TypeAnnotation *Type)
      : Declaration(Loc, DeclKind::ParamDecl, Name), Type(Type) {}

private:
  /** The declared type. */
  TypeAnnotation *Type;
};

class VarDef : public Declaration {
  friend ASTContext;

public:
  TypeAnnotation *getType() const { return Type; }
  Literal *getValue() const { return Value; }

public:
  static bool classof(const Declaration *D) {
    return D->getKind() == DeclKind::VarDef;
  }

private:
  VarDef(SMRange Loc, Identifier *Name, TypeAnnotation *Type, Literal *Value)
      : Declaration(Loc, DeclKind::VarDef, Name), Type(Type), Value(Value) {}

private:
  TypeAnnotation *Type;
  Literal *Value;
};

class alignas(void *) TypeAnnotation {
public:
  enum class Kind {
    Class,
    List,
  };

public:
  Kind getKind() const { return Kind; }
  SMRange getLocation() const { return Loc; }

protected:
  TypeAnnotation(SMRange Loc, Kind Kind) : Kind(Kind), Loc(Loc) {}

private:
  void operator delete(void *) {
    llvm_unreachable("AST nodes deletion is prohibited");
  }

private:
  Kind Kind;
  SMRange Loc;
};

class ClassType : public TypeAnnotation {
  friend ASTContext;

public:
  StringRef getClassName() const { return StringRef(getStrData(), NameLength); }

public:
  static bool classof(const TypeAnnotation *T) {
    return T->getKind() == Kind::Class;
  }

private:
  ClassType(SMRange Loc) : TypeAnnotation(Loc, Kind::Class) {}

  /// getStrData - Return the start of the string data that is the name for this
  /// class. The string data is always stored immediately after the
  /// ClassType object.
  const char *getStrData() const {
    return reinterpret_cast<const char *>(this + 1);
  }

  size_t NameLength;
};

class ListType : public TypeAnnotation {
  friend ASTContext;

public:
  TypeAnnotation *getElementType() const { return ElementType; }

public:
  static bool classof(const TypeAnnotation *T) {
    return T->getKind() == Kind::List;
  }

private:
  ListType(SMRange Loc, TypeAnnotation *ElType)
      : TypeAnnotation(Loc, Kind::List), ElementType(ElType) {}

private:
  TypeAnnotation *ElementType;
};

class alignas(void *) Stmt {
public:
  enum class StmtKind {
    AssignStmt,
    ExprStmt,
    ForStmt,
    IfStmt,
    ReturnStmt,
    WhileStmt
  };

  StmtKind getKind() const { return Kind; }

  SMRange getLocation() const { return Loc; }

protected:
  Stmt(SMRange Loc, StmtKind Kind) : Kind(Kind), Loc(Loc) {}

private:
  void operator delete(void *) {
    llvm_unreachable("AST nodes deletion is prohibited");
  }

protected:
  StmtKind Kind;
  SMRange Loc;
};

/** Single and multiple assignments. */
class AssignStmt : public Stmt {
  friend ASTContext;

public:
  ArrayRef<Expr *> getTargets() const { return Targets; }
  Expr *getValue() const { return Value; }

public:
  static bool classof(const Stmt *S) {
    return S->getKind() == Stmt::StmtKind::AssignStmt;
  }

private:
  AssignStmt(SMRange Loc, ExprList Targets, Expr *Value)
      : Stmt(Loc, StmtKind::AssignStmt), Targets(Targets), Value(Value) {}

private:
  /** List of left-hand sides. */
  ExprList Targets;
  /** Right-hand-side value to be assigned. */
  Expr *Value;
};

class ExprStmt : public Stmt {
  friend ASTContext;

public:
  Expr *getExpr() const { return TheExpr; }

public:
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::ExprStmt;
  }

private:
  ExprStmt(SMRange Loc, Expr *E) : Stmt(Loc, StmtKind::ExprStmt), TheExpr(E) {}

private:
  Expr *TheExpr;
};

/** For statements. */
class ForStmt : public Stmt {
  friend ASTContext;

public:
  DeclRef *getTarget() const { return Target; }
  Expr *getIterable() const { return Iterable; }
  ArrayRef<Stmt *> getBody() const { return Body; }

public:
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::ForStmt;
  }

private:
  ForStmt(SMRange Loc, DeclRef *Target, Expr *Iterable, ArrayRef<Stmt *> Body)
      : Stmt(Loc, StmtKind::ForStmt), Target(Target), Iterable(Iterable),
        Body(Body) {}

private:
  /** Control variable. */
  DeclRef *Target;
  /** Source of values of control statement. */
  Expr *Iterable;
  /** Repeated statements. */
  StmtList Body;
};

/** Conditional statement. */
class IfStmt : public Stmt {
  friend ASTContext;

public:
  Expr *getCondition() const { return Condition; };
  ArrayRef<Stmt *> getThenBody() const { return ThenBody; };
  ArrayRef<Stmt *> getElseBody() const { return ElseBody; };

public:
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::IfStmt;
  }

private:
  IfStmt(SMRange Loc, Expr *Condition, ArrayRef<Stmt *> ThenBody,
         ArrayRef<Stmt *> ElseBody)
      : Stmt(Loc, StmtKind::IfStmt), Condition(Condition), ThenBody(ThenBody),
        ElseBody(ElseBody) {}

private:
  /** Test condition. */
  Expr *Condition;
  /** "True" branch. */
  StmtList ThenBody;
  /** "False" branch. */
  StmtList ElseBody;
};

/** Return from function. */
class ReturnStmt : public Stmt {
  friend ASTContext;

public:
  Expr *getValue() const { return Value; }

public:
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::ReturnStmt;
  }

private:
  ReturnStmt(SMRange Loc, Expr *Value)
      : Stmt(Loc, StmtKind::ReturnStmt), Value(Value) {}

private:
  /** Returned value. */
  Expr *Value;
};

/** Indefinite repetition construct. */
class WhileStmt : public Stmt {
  friend ASTContext;

public:
  Expr *getCondition() const { return Condition; }
  ArrayRef<Stmt *> getBody() const { return Body; }

public:
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::WhileStmt;
  }

private:
  WhileStmt(SMRange Loc, Expr *Condition, ArrayRef<Stmt *> Body)
      : Stmt(Loc, StmtKind::WhileStmt), Condition(Condition), Body(Body) {}

private:
  /** Test for whether to continue. */
  Expr *Condition;
  /** Loop body. */
  StmtList Body;
};

class alignas(void *) Expr {
public:
  enum class Kind {
    BinaryExpr,
    CallExpr,
    DeclRef,
    IfExpr,
    IndexExpr,
    ListExpr,
    Literal,
    MemberExpr,
    MethodCallExpr,
    UnaryExpr,
  };

  Type *getInferredType() const { return InferredType; }
  void setInferredType(Type *T) { InferredType = T; }

  Kind getKind() const { return TheKind; }

  SMRange getLocation() const { return Loc; }

protected:
  Expr(SMRange Loc, Kind TheKind) : TheKind(TheKind), Loc(Loc) {}

protected:
  Type *InferredType = nullptr;
  Kind TheKind;
  SMRange Loc;

private:
  void operator delete(void *) {
    llvm_unreachable("AST nodes deletion is prohibited");
  }
};

/** <operand> <operator> <operand>. */
class BinaryExpr : public Expr {
  friend ASTContext;

public:
  enum class OpKind {
    And,
    Or,
    Add,
    Sub,
    Mul,
    FloorDiv,
    Mod,
    EqCmp,
    NEqCmp,
    LEqCmp,
    GEqCmp,
    LCmp,
    GCmp,
    Is,
  };

public:
  Expr *getLeft() const { return Left; }
  OpKind getOpKind() const { return Kind; }
  Expr *getRight() const { return Right; }

  StringRef getOpKindStr() const { return getOpKindStr(Kind); }

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Kind::BinaryExpr;
  }

  static StringRef getOpKindStr(OpKind K);

private:
  BinaryExpr(SMRange Loc, Expr *Left, OpKind Kind, Expr *Right)
      : Expr(Loc, Kind::BinaryExpr), Left(Left), Kind(Kind), Right(Right) {}

private:
  /** Left operand. */
  Expr *Left;
  /** Operator name. */
  OpKind Kind;
  /** Right operand. */
  Expr *Right;
};

/** A function call. */
class CallExpr : public Expr {
  friend ASTContext;

public:
  Expr *getFunction() const { return Function; }
  ArrayRef<Expr *> getArgs() const { return Args; }

public:
  static bool classof(const Expr *E) { return E->getKind() == Kind::CallExpr; }

private:
  CallExpr(SMRange Loc, Expr *Function, ArrayRef<Expr *> Args)
      : Expr(Loc, Kind::CallExpr), Function(Function), Args(Args) {}

private:
  /** The called function. */
  Expr *Function;
  /** The actual parameter expressions. */
  ExprList Args;
};

class DeclRef : public Expr {
  friend ASTContext;

public:
  StringRef getName() const { return Name->getName(); }
  SymbolInfo *getSymbolInfo() const { return Name; }

  void setDeclInfo(Declaration *D) { DeclInfo = D; }
  Declaration *getDeclInfo() const { return DeclInfo; }

public:
  static bool classof(const Expr *E) { return E->getKind() == Kind::DeclRef; }

private:
  DeclRef(SMRange Loc, SymbolInfo *SI) : Expr(Loc, Kind::DeclRef), Name(SI) {}

  /** Text of the identifier. */
  SymbolInfo *Name;

  // @todo: Redesign declaration info
  Declaration *DeclInfo;
};

/** Conditional expressions. */
class IfExpr : public Expr {
  friend ASTContext;

public:
  Expr *getCondExpr() const { return CondExpr; }
  /** True branch. */
  Expr *getThenExpr() const { return ThenExpr; }
  /** False branch. */
  Expr *getElseExpr() const { return ElseExpr; }

public:
  static bool classof(const Expr *E) { return E->getKind() == Kind::IfExpr; }

private:
  IfExpr(SMRange Loc, Expr *CondExpr, Expr *ThenExpr, Expr *ElseExpr)
      : Expr(Loc, Kind::IfExpr), CondExpr(CondExpr), ThenExpr(ThenExpr),
        ElseExpr(ElseExpr) {}

  /** Boolean condition. */
  Expr *CondExpr;
  /** True branch. */
  Expr *ThenExpr;
  /** False branch. */
  Expr *ElseExpr;
};

/** List-indexing expression. */
class IndexExpr : public Expr {
  friend ASTContext;

public:
  Expr *getList() const { return List; }
  Expr *getIndex() const { return Index; }

public:
  static bool classof(const Expr *E) { return E->getKind() == Kind::IndexExpr; }

private:
  IndexExpr(SMRange Loc, Expr *List, Expr *Index)
      : Expr(Loc, Kind::IndexExpr), List(List), Index(Index) {}

private:
  /** Indexed list. */
  Expr *List;
  /** Expression for index value. */
  Expr *Index;
};

/** List displays. */
class ListExpr : public Expr {
  friend ASTContext;

public:
  ArrayRef<Expr *> getElements() const { return Elements; }

public:
  static bool classof(const Expr *E) { return E->getKind() == Kind::ListExpr; }

private:
  ListExpr(SMRange Loc, ArrayRef<Expr *> Elts)
      : Expr(Loc, Kind::ListExpr), Elements(Elts) {}

private:
  /** List of element expressions. */
  ExprList Elements;
};

/**
 * Base of all the literal nodes.
 *
 * There is nothing in this class, but it is useful to isolate
 * expressions that are constant literals.
 */
class Literal : public Expr {
public:
  enum class LiteralType {
    Bool,
    Integer,
    None,
    String,
  };

public:
  LiteralType getLiteralType() const { return Type; }

public:
  static bool classof(const Expr *E) { return E->getKind() == Kind::Literal; }

protected:
  Literal(SMRange Loc, LiteralType Type)
      : Expr(Loc, Kind::Literal), Type(Type) {}

private:
  LiteralType Type;
};

/** Literals True or False. */
class BooleanLiteral : public Literal {
  friend ASTContext;

public:
  bool getValue() const { return Value; }

public:
  static bool classof(const Literal *L) {
    return L->getLiteralType() == LiteralType::Bool;
  }

  static bool classof(const Expr *E) {
    if (const Literal *L = dyn_cast<Literal>(E))
      return classof(L);
    return false;
  }

private:
  BooleanLiteral(SMRange Loc, bool Value)
      : Literal(Loc, LiteralType::Bool), Value(Value) {}

private:
  /** True iff I represent True. */
  bool Value;
};

/** Integer numerals. */
class IntegerLiteral : public Literal {
  friend ASTContext;

public:
  int64_t getValue() const { return Value; }

public:
  static bool classof(const Literal *L) {
    return L->getLiteralType() == LiteralType::Integer;
  }

  static bool classof(const Expr *E) {
    if (const Literal *L = dyn_cast<Literal>(E))
      return classof(L);
    return false;
  }

private:
  IntegerLiteral(SMRange Loc, int64_t Value)
      : Literal(Loc, LiteralType::Integer), Value(Value) {}

private:
  int64_t Value;
};

class NoneLiteral : public Literal {
  friend ASTContext;

public:
  static bool classof(const Literal *L) {
    return L->getLiteralType() == LiteralType::None;
  }

  static bool classof(const Expr *E) {
    if (const Literal *L = dyn_cast<Literal>(E))
      return classof(L);
    return false;
  }

private:
  NoneLiteral(SMRange Loc) : Literal(Loc, LiteralType::None) {}
};

/** String constants. */
class StringLiteral : public Literal {
  friend ASTContext;

public:
  StringRef getValue() const { return Value; }

public:
  static bool classof(const Literal *L) {
    return L->getLiteralType() == LiteralType::String;
  }

  static bool classof(const Expr *E) {
    if (const Literal *L = dyn_cast<Literal>(E))
      return classof(L);
    return false;
  }

private:
  StringLiteral(SMRange Loc, StringRef Value)
      : Literal(Loc, LiteralType::String), Value(Value) {}

private:
  StringRef Value;
};

/** Attribute accessor. */
class MemberExpr : public Expr {
  friend ASTContext;

public:
  Expr *getObject() const { return Object; }
  DeclRef *getMember() const { return Member; }

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Kind::MemberExpr;
  }

private:
  MemberExpr(SMRange Loc, Expr *Obj, DeclRef *Member)
      : Expr(Loc, Kind::MemberExpr), Object(Obj), Member(Member) {}

private:
  /** Object selected from. */
  Expr *Object;
  /** Name of attribute (instance variable or method). */
  DeclRef *Member;
};

/** Method calls. */
class MethodCallExpr : public Expr {
  friend ASTContext;

public:
  MemberExpr *getMethod() const { return Method; }
  ArrayRef<Expr *> getArgs() const { return Args; }

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Kind::MethodCallExpr;
  }

private:
  MethodCallExpr(SMRange Loc, MemberExpr *Method, ArrayRef<Expr *> Args)
      : Expr(Loc, Kind::MethodCallExpr), Method(Method), Args(Args) {}

private:
  /** Expression for the bound method to be called. */
  MemberExpr *Method;
  /** Actual parameters. */
  ExprList Args;
};

/** An expression applying a unary operator. */
class UnaryExpr : public Expr {
  friend ASTContext;

public:
  enum class OpKind { Not, Minus };

public:
  OpKind getOpKind() const { return Kind; }
  Expr *getOperand() const { return Operand; }

public:
  static bool classof(const Expr *E) { return E->getKind() == Kind::UnaryExpr; }

private:
  UnaryExpr(SMRange Loc, OpKind Kind, Expr *Operand)
      : Expr(Loc, Kind::UnaryExpr), Kind(Kind), Operand(Operand) {}

private:
  /** The text representation of the operator. */
  OpKind Kind;
  /** The operand to which it is applied. */
  Expr *Operand;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_AST_AST_H
