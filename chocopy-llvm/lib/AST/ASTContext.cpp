#include "chocopy-llvm/AST/ASTContext.h"
#include "chocopy-llvm/Lexer/Lexer.h"

#include <llvm/ADT/TypeSwitch.h>

namespace chocopy {

static constexpr const char *OBJECT = "object";
static constexpr const char *INT = "int";
static constexpr const char *STR = "str";
static constexpr const char *BOOL = "bool";
static constexpr const char *NONE = "<None>";
static constexpr const char *EMPTY = "<Empty>";
static constexpr const char *INIT = "__init__";
static constexpr const char *THIS = "this";
static constexpr const char *PRINT = "print";
static constexpr const char *INPUT = "input";
static constexpr const char *LEN = "len";
static constexpr const char *PTHIS = "<this>";

void ASTContext::initialize(Lexer &Lex) {
  SymbolTable &ST = Lex.getSymbolTable();
  initPredefinedClasses(ST);
  initPredefinedFunctions(ST);
  initPredefinedTypes(ST);
}

Identifier *ASTContext::createIdentifier(SMRange Loc, SymbolInfo *Name) {
  return create<Identifier>(Loc, Name);
}

Program *ASTContext::createProgram(const DeclList &Decls,
                                   const StmtList &Stmts) {
  assert(!TheProgram);
  TheProgram = create<Program>(Decls, Stmts);
  return TheProgram;
}

ClassDef *ASTContext::createClassDef(SMRange Loc, Identifier *Name,
                                     Identifier *SuperClass,
                                     const DeclList &Declarations) {
  return create<ClassDef>(Loc, Name, SuperClass, Declarations);
}

FuncDef *ASTContext::createFuncDef(SMRange Loc, Identifier *Name,
                                   const ParamDeclList &Params,
                                   TypeAnnotation *ReturnType,
                                   const DeclList &Declarations,
                                   const StmtList &Statements) {
  return create<FuncDef>(Loc, Name, Params, ReturnType, Declarations,
                         Statements);
}
GlobalDecl *ASTContext::createGlobalDecl(SMRange Loc, Identifier *Name) {
  return create<GlobalDecl>(Loc, Name);
}

NonLocalDecl *ASTContext::createNonLocalDecl(SMRange Loc, Identifier *Name) {
  return create<NonLocalDecl>(Loc, Name);
}

VarDef *ASTContext::createVarDef(SMRange Loc, Identifier *Name,
                                 TypeAnnotation *Type, Literal *Value) {
  return create<VarDef>(Loc, Name, Type, Value);
}

ParamDecl *ASTContext::createParamDecl(SMRange Loc, Identifier *Name,
                                       TypeAnnotation *Type) {
  return create<ParamDecl>(Loc, Name, Type);
}

ClassType *ASTContext::createClassType(SMRange Loc, StringRef ClassName) {
  size_t TySize = sizeof(ClassType);
  void *Mem = allocate(TySize + ClassName.size() + 1);
  ClassType *CT = new (Mem) ClassType(Loc);
  CT->NameLength = ClassName.size();
  char *NamePtr = reinterpret_cast<char *>(CT + 1);
  ::memcpy(NamePtr, ClassName.data(), ClassName.size());
  NamePtr[ClassName.size()] = 0; // Null terminator
  return CT;
}

ListType *ASTContext::createListType(SMRange Loc, TypeAnnotation *ElType) {
  return create<ListType>(Loc, ElType);
}

AssignStmt *ASTContext::createAssignStmt(SMRange Loc, const ExprList &Targets,
                                         Expr *Value) {
  return create<AssignStmt>(Loc, Targets, Value);
}

ExprStmt *ASTContext::createExprStmt(SMRange Loc, Expr *E) {
  return create<ExprStmt>(Loc, E);
}

ForStmt *ASTContext::createForStmt(SMRange Loc, DeclRef *Target, Expr *Iterable,
                                   const StmtList &Body) {
  return create<ForStmt>(Loc, Target, Iterable, Body);
}

IfStmt *ASTContext::createIfStmt(SMRange Loc, Expr *Condition,
                                 const StmtList &ThenBody,
                                 const StmtList &ElseBody) {
  return create<IfStmt>(Loc, Condition, ThenBody, ElseBody);
}

ReturnStmt *ASTContext::createReturnStmt(SMRange Loc, Expr *Value) {
  return create<ReturnStmt>(Loc, Value);
}

WhileStmt *ASTContext::createWhileStmt(SMRange Loc, Expr *Condition,
                                       const StmtList &Body) {
  return create<WhileStmt>(Loc, Condition, Body);
}

BinaryExpr *ASTContext::createBinaryExpr(SMRange Loc, Expr *Left,
                                         BinaryExpr::OpKind Kind, Expr *Right) {
  return create<BinaryExpr>(Loc, Left, Kind, Right);
}

CallExpr *ASTContext::createCallExpr(SMRange Loc, Expr *Function,
                                     const ExprList &Args) {
  return create<CallExpr>(Loc, Function, Args);
}

DeclRef *ASTContext::createDeclRef(SMRange Loc, SymbolInfo *Name) {
  return create<DeclRef>(Loc, Name);
}

IfExpr *ASTContext::createIfExpr(SMRange Loc, Expr *Cond, Expr *ThenExpr,
                                 Expr *ElseExpr) {
  return create<IfExpr>(Loc, Cond, ThenExpr, ElseExpr);
}

IndexExpr *ASTContext::createIndexExpr(SMRange Loc, Expr *List, Expr *Index) {
  return create<IndexExpr>(Loc, List, Index);
}

ListExpr *ASTContext::createListExpr(SMRange Loc, const ExprList &Elts) {
  return create<ListExpr>(Loc, Elts);
}

BooleanLiteral *ASTContext::createBooleanLiteral(SMRange Loc, bool Value) {
  return create<BooleanLiteral>(Loc, Value);
}

IntegerLiteral *ASTContext::createIntegerLiteral(SMRange Loc, int64_t Value) {
  return create<IntegerLiteral>(Loc, Value);
}

NoneLiteral *ASTContext::createNoneLiteral(SMRange Loc) {
  return create<NoneLiteral>(Loc);
}

StringLiteral *ASTContext::createStringLiteral(SMRange Loc, StringRef Value) {
  return create<StringLiteral>(Loc, Value);
}

MemberExpr *ASTContext::createMemberExpr(SMRange Loc, Expr *O, DeclRef *M) {
  return create<MemberExpr>(Loc, O, M);
}

MethodCallExpr *ASTContext::createMethodCallExpr(SMRange Loc,
                                                 MemberExpr *Method,
                                                 const ExprList &Args) {
  return create<MethodCallExpr>(Loc, Method, Args);
}

UnaryExpr *ASTContext::createUnaryExpr(SMRange Loc, UnaryExpr::OpKind Kind,
                                       Expr *Operand) {
  return create<UnaryExpr>(Loc, Kind, Operand);
}

ClassValueType *ASTContext::getClassVType(StringRef Name) {
  if (ClassValueType *C = ClassVTypes.lookup(Name))
    return C;

  return ClassVTypes.insert({Name, create<ClassValueType>(*this, Name)})
      .first->getValue();
}

ListValueType *ASTContext::getListVType(ValueType *ElTy) {
  if (ListValueType *L = ListVTypes.lookup(ElTy))
    return L;
  return ListVTypes.insert({ElTy, create<ListValueType>(*this, ElTy)})
      .first->getSecond();
}

FuncType *ASTContext::getFuncType(const ValueTypeList &ParametersTy,
                                  ValueType *RetTy) {
  const FuncTypeKeyInfo::KeyTy Key{RetTy, ParametersTy};
  auto It = FuncTypes.insert_as(nullptr, Key);
  if (It.second)
    *It.first = create<FuncType>(ParametersTy, RetTy);

  return *It.first;
}

ValueType *ASTContext::convertAnnotationToVType(TypeAnnotation *TA) {
  return llvm::TypeSwitch<TypeAnnotation *, ValueType *>(TA)
      .Case([this](ClassType *CT) { return getClassVType(CT->getClassName()); })
      .Case([this](ListType *L) {
        return getListVType(convertAnnotationToVType(L->getElementType()));
      });
}

bool ASTContext::isAssignementCompatibility(const ValueType *Sub,
                                            const ValueType *Super) {
  if (Sub == Super)
    return true;

  if (Sub == NoneTy)
    return Super != IntTy && Super != StrTy && Super != BoolTy;

  if (isa<const ListValueType>(Super)) {
    if (Sub == EmptyTy)
      return true;

    if (auto *L = dyn_cast<ListValueType>(Sub))
      return L->getElementType() == NoneTy;

    return false;
  }

  return false;
}

void ASTContext::initPredefinedClasses(SymbolTable &ST) {
  SMRange Loc;

  Identifier *ObjId = createIdentifier(Loc, &ST.get(OBJECT));
  Identifier *IntId = createIdentifier(Loc, &ST.get(INT));
  Identifier *StrId = createIdentifier(Loc, &ST.get(STR));
  Identifier *BoolId = createIdentifier(Loc, &ST.get(BOOL));
  Identifier *NoneId = createIdentifier(Loc, &ST.get(NONE));
  Identifier *EmptyId = createIdentifier(Loc, &ST.get(EMPTY));
  Identifier *InitId = createIdentifier(Loc, &ST.get(INIT));
  Identifier *ThisId = createIdentifier(Loc, &ST.get(THIS));

  ClassType *ObjTy = createClassType(Loc, OBJECT);
  ParamDecl *ThisPD = createParamDecl(Loc, ThisId, ObjTy);

  TypeAnnotation *NoneRet = createClassType(Loc, NoneId->getName());
  FuncDef *InitFD = createFuncDef(Loc, InitId, ParamDeclList{ThisPD}, NoneRet,
                                  DeclList{}, StmtList{});

  ObjClass = createClassDef(Loc, ObjId, nullptr, DeclList{InitFD});
  IntClass = createClassDef(Loc, IntId, ObjId, DeclList{InitFD});
  StrClass = createClassDef(Loc, StrId, ObjId, DeclList{InitFD});
  BoolClass = createClassDef(Loc, BoolId, ObjId, DeclList{InitFD});
  NoneClass = createClassDef(Loc, NoneId, ObjId, DeclList{InitFD});
  EmptyClass = createClassDef(Loc, EmptyId, ObjId, DeclList{InitFD});
}

void ASTContext::initPredefinedFunctions(SymbolTable &ST) {
  SMRange Loc;
  Identifier *PringId = createIdentifier(Loc, &ST.get(PRINT));
  Identifier *InputId = createIdentifier(Loc, &ST.get(INPUT));
  Identifier *LenId = createIdentifier(Loc, &ST.get(LEN));
  Identifier *ParamId = createIdentifier(Loc, &ST.get(PTHIS));

  ClassType *ObjTy = createClassType(Loc, OBJECT);
  ClassType *StrTy = createClassType(Loc, STR);
  ClassType *IntTy = createClassType(Loc, INT);

  ParamDecl *ObjParam = createParamDecl(Loc, ParamId, ObjTy);

  DeclList Declarations;
  StmtList Statements;

  ParamDeclList Params{ObjParam};
  PrintFD =
      createFuncDef(Loc, PringId, Params, nullptr, Declarations, Statements);

  Params.clear();
  InputFD =
      createFuncDef(Loc, InputId, Params, StrTy, Declarations, Statements);

  Params.push_back(ObjParam);
  LenFD = createFuncDef(Loc, LenId, Params, IntTy, Declarations, Statements);
}

void ASTContext::initPredefinedTypes(SymbolTable &ST) {
  ObjectTy = create<ClassValueType>(*this, OBJECT);
  IntTy = create<ClassValueType>(*this, INT);
  StrTy = create<ClassValueType>(*this, STR);
  BoolTy = create<ClassValueType>(*this, BOOL);
  NoneTy = create<ClassValueType>(*this, NONE);
  EmptyTy = create<ClassValueType>(*this, EMPTY);

  ClassVTypes.insert({OBJECT, ObjectTy});
  ClassVTypes.insert({INT, IntTy});
  ClassVTypes.insert({STR, StrTy});
  ClassVTypes.insert({BOOL, BoolTy});
  ClassVTypes.insert({NONE, NoneTy});
  ClassVTypes.insert({EMPTY, EmptyTy});
}
} // namespace chocopy
