#ifndef CHOCOPY_LLVM_AST_ASTCONTEXT_H
#define CHOCOPY_LLVM_AST_ASTCONTEXT_H

#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/Type.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/SourceMgr.h>

namespace chocopy {
class Lexer;
class SymbolTable;

class ASTContext {
public:
  ASTContext(llvm::SourceMgr &SrcMgr) : SrcMgr(SrcMgr) {}

  void initialize(Lexer &Lex);

public:
  const llvm::SourceMgr &getSourceMgr() const { return SrcMgr; }

  inline ClassDef *getObjectClass() const { return ObjClass; }
  inline ClassDef *getIntClass() const { return IntClass; }
  inline ClassDef *getStrClass() const { return StrClass; }
  inline ClassDef *getBoolClass() const { return BoolClass; }
  inline ClassDef *getNoneClass() const { return NoneClass; }
  inline ClassDef *getEmptyClass() const { return EmptyClass; }

  inline FuncDef *getPrintFunc() const { return PrintFD; }
  inline FuncDef *getInputFunc() const { return InputFD; }
  inline FuncDef *getLenFunc() const { return LenFD; }

  inline Program *getProgram() const { return TheProgram; }

  inline ClassValueType *getObjectTy() const { return ObjectTy; }
  inline ClassValueType *getIntTy() const { return IntTy; }
  inline ClassValueType *getStrTy() const { return StrTy; }
  inline ClassValueType *getBoolTy() const { return BoolTy; }
  inline ClassValueType *getNoneTy() const { return NoneTy; }
  inline ClassValueType *getEmptyTy() const { return EmptyTy; }

  inline bool isObjectClass(const ClassDef *CD) const { return CD == ObjClass; }
  inline bool isIntClass(const ClassDef *CD) const { return CD == IntClass; }
  inline bool isStrClass(const ClassDef *CD) const { return CD == StrClass; }
  inline bool isBoolClass(const ClassDef *CD) const { return CD == BoolClass; }

public:
  Identifier *createIdentifier(SMRange Loc, SymbolInfo *Name);
  Program *createProgram(const DeclList &Decls, const StmtList &Stmts);
  ClassDef *createClassDef(SMRange Loc, Identifier *Name,
                           Identifier *SuperClass,
                           const DeclList &Declarations);
  FuncDef *createFuncDef(SMRange Loc, Identifier *Name,
                         const ParamDeclList &Params,
                         TypeAnnotation *ReturnType,
                         const DeclList &Declarations,
                         const StmtList &Statements);
  GlobalDecl *createGlobalDecl(SMRange Loc, Identifier *Id);
  NonLocalDecl *createNonLocalDecl(SMRange Loc, Identifier *Name);
  VarDef *createVarDef(SMRange Loc, Identifier *Name, TypeAnnotation *Type,
                       Literal *Value);
  ParamDecl *createParamDecl(SMRange Loc, Identifier *Name,
                             TypeAnnotation *Type);
  ClassType *createClassType(SMRange Loc, StringRef ClassName);
  ListType *createListType(SMRange Loc, TypeAnnotation *ElType);
  AssignStmt *createAssignStmt(SMRange Loc, const ExprList &Targets,
                               Expr *Value);
  ExprStmt *createExprStmt(SMRange Loc, Expr *E);
  ForStmt *createForStmt(SMRange Loc, DeclRef *Target, Expr *Iterable,
                         const StmtList &Body);
  IfStmt *createIfStmt(SMRange Loc, Expr *Condition, const StmtList &ThenBody,
                       const StmtList &ElseBody);
  ReturnStmt *createReturnStmt(SMRange Loc, Expr *Value = nullptr);
  WhileStmt *createWhileStmt(SMRange Loc, Expr *Condition,
                             const StmtList &Body);
  BinaryExpr *createBinaryExpr(SMRange Loc, Expr *Left, BinaryExpr::OpKind Kind,
                               Expr *Right);
  CallExpr *createCallExpr(SMRange Loc, Expr *Function, const ExprList &Args);
  DeclRef *createDeclRef(SMRange Loc, SymbolInfo *Name);
  IfExpr *createIfExpr(SMRange Loc, Expr *Cond, Expr *ThenExpr, Expr *ElseExpr);
  IndexExpr *createIndexExpr(SMRange Loc, Expr *List, Expr *Index);
  ListExpr *createListExpr(SMRange Loc, const ExprList &Elts);
  BooleanLiteral *createBooleanLiteral(SMRange Loc, bool Value);
  IntegerLiteral *createIntegerLiteral(SMRange Loc, int64_t Value);
  NoneLiteral *createNoneLiteral(SMRange Loc);
  StringLiteral *createStringLiteral(SMRange Loc, StringRef Value);
  MemberExpr *createMemberExpr(SMRange Loc, Expr *O, DeclRef *M);
  MethodCallExpr *createMethodCallExpr(SMRange Loc, MemberExpr *Method,
                                       const ExprList &Args);
  UnaryExpr *createUnaryExpr(SMRange Loc, UnaryExpr::OpKind Kind,
                             Expr *Operand);

  // Value types
  ClassValueType *getClassVType(StringRef Name);
  ListValueType *getListVType(ValueType *ElTy);
  FuncType *getFuncType(const ValueTypeList &ParametersTy, ValueType *RetTy);

  ValueType *convertAnnotationToVType(TypeAnnotation *TA);

  bool isAssignementCompatibility(const ValueType *Sub, const ValueType *Sup);

private:
  template <typename NodeTy, typename... ArgsTy>
  NodeTy *create(ArgsTy &&...Args) const {
    void *Mem = allocate(sizeof(NodeTy), alignof(NodeTy));
    return new (Mem) NodeTy(std::forward<ArgsTy>(Args)...);
  }

  void *allocate(size_t Size, unsigned Align = 8) const {
    return BumpAlloc.Allocate(Size, Align);
  }

private:
  void initPredefinedClasses(SymbolTable &ST);
  void initPredefinedFunctions(SymbolTable &ST);
  void initPredefinedTypes(SymbolTable &ST);

private:
  struct FuncTypeKeyInfo {
    struct KeyTy {
      const ValueType *ReturnType;
      ArrayRef<ValueType *> Params;

      KeyTy(const ValueType *R, const ArrayRef<ValueType *> &P)
          : ReturnType(R), Params(P) {}
      KeyTy(const FuncType *FT)
          : ReturnType(FT->getReturnType()), Params(FT->getParametersTypes()) {}

      bool operator==(const KeyTy &That) const {
        if (ReturnType != That.ReturnType)
          return false;
        if (Params != That.Params)
          return false;
        return true;
      }
      bool operator!=(const KeyTy &That) const {
        return !this->operator==(That);
      }
    };

    static inline FuncType *getEmptyKey() {
      return llvm::DenseMapInfo<FuncType *>::getEmptyKey();
    }

    static inline FuncType *getTombstoneKey() {
      return llvm::DenseMapInfo<FuncType *>::getTombstoneKey();
    }

    static unsigned getHashValue(const KeyTy &Key) {
      return hash_combine(
          Key.ReturnType,
          llvm::hash_combine_range(Key.Params.begin(), Key.Params.end()));
    }

    static unsigned getHashValue(const FuncType *FT) {
      return getHashValue(KeyTy(FT));
    }

    static bool isEqual(const KeyTy &LHS, const FuncType *RHS) {
      if (RHS == getEmptyKey() || RHS == getTombstoneKey())
        return false;
      return LHS == KeyTy(RHS);
    }

    static bool isEqual(const FuncType *LHS, const FuncType *RHS) {
      return LHS == RHS;
    }
  };

private:
  mutable llvm::BumpPtrAllocator BumpAlloc;
  llvm::SourceMgr &SrcMgr;
  Program *TheProgram = nullptr;
  ClassDef *ObjClass = nullptr;
  ClassDef *IntClass = nullptr;
  ClassDef *StrClass = nullptr;
  ClassDef *BoolClass = nullptr;
  ClassDef *NoneClass = nullptr;
  ClassDef *EmptyClass = nullptr;

  FuncDef *PrintFD = nullptr;
  FuncDef *InputFD = nullptr;
  FuncDef *LenFD = nullptr;

  ClassValueType *ObjectTy = nullptr;
  ClassValueType *IntTy = nullptr;
  ClassValueType *StrTy = nullptr;
  ClassValueType *BoolTy = nullptr;
  ClassValueType *NoneTy = nullptr;
  ClassValueType *EmptyTy = nullptr;

  // Map from Element type to List type
  llvm::DenseMap<ValueType *, ListValueType *> ListVTypes;
  llvm::StringMap<ClassValueType *> ClassVTypes;
  llvm::DenseSet<FuncType *, FuncTypeKeyInfo> FuncTypes;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_AST_ASTCONTEXT_H
