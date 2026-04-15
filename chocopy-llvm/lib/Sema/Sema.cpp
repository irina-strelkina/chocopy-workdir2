#include "chocopy-llvm/Sema/Sema.h"
#include "chocopy-llvm/AST/ASTContext.h"
#include "chocopy-llvm/AST/DeclVisitor.h"
#include "chocopy-llvm/AST/ExprVisitor.h"
#include "chocopy-llvm/AST/StmtVisitor.h"
#include "chocopy-llvm/AST/Type.h"
#include "chocopy-llvm/Analysis/CFG.h"
#include "chocopy-llvm/Basic/Diagnostic.h"
#include "chocopy-llvm/Lexer/Lexer.h"
#include "chocopy-llvm/Sema/Scope.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/TypeSwitch.h>

namespace chocopy {

//===----------------------------------------------------------------------===//
// Вспомогательные функции: вывод типов для диагностики
//===----------------------------------------------------------------------===//

/// Выводит текстовое представление типа \p T в поток \p Stream.
static raw_ostream &operator<<(raw_ostream &Stream, const Type &T) {
  llvm::TypeSwitch<const Type *>(&T)
      .Case([&Stream](const FuncType *FT) {
        Stream << "(";
        for (ValueType *T : FT->getParametersTypes()) {
          Stream << T;
          if (T != FT->getParametersTypes().back())
            Stream << ", ";
        }
        Stream << ") -> " << FT->getReturnType();
      })
      .Case([&Stream](const ClassValueType *CVT) {
        Stream << CVT->getClassName();
      })
      .Case([&Stream](const ListValueType *LVT) {
        std::string Str;
        llvm::raw_string_ostream(Str) << *LVT->getElementType();
        Stream << llvm::formatv("[{}]", Str);
      });
  return Stream;
}

static InFlightDiagnostic &&operator<<(InFlightDiagnostic &&D, const Type &T) {
  std::string Err;
  llvm::raw_string_ostream(Err) << T;
  D << Err;
  return std::move(D);
}

static InFlightDiagnostic &&operator<<(InFlightDiagnostic &&D, int V) {
  std::string Err;
  llvm::raw_string_ostream(Err) << V;
  D << Err;
  return std::move(D);
}

//===----------------------------------------------------------------------===//
// Analysis — класс-посетитель AST, выполняющий все семантические проверки.
//===----------------------------------------------------------------------===//
class Sema::Analysis : public DeclVisitor<Analysis>,
                       public StmtVisitor<Analysis>,
                       public ExprVisitor<Analysis> {
  using DeclVisitor<Analysis>::visit;
  using StmtVisitor<Analysis>::visit;
  using ExprVisitor<Analysis>::visit;

  //===----------------------------------------------------------------------===//
  // RAII-обертка для управления областями видимости (Scope)
  //===----------------------------------------------------------------------===//

  /// RAII-guard для входа и выхода из области видимости.
  ///
  /// Конструктор: создает дочернюю область видимости и делает ее текущей.
  /// Деструктор: вызывает actOnPopScope и восстанавливает родительскую
  /// область.
  class SemaScope {
  public:
    SemaScope() = delete;
    SemaScope(const SemaScope &) = delete;
    SemaScope(SemaScope &&) = delete;
    SemaScope &operator=(const SemaScope &) = delete;
    SemaScope &operator=(SemaScope &&) = delete;

    /// Входит в новую глобальную область видимости (для программы верхнего уровня).
    SemaScope(Analysis *Self) : Self(Self) {
      std::shared_ptr<Scope> NewScope = std::make_shared<Scope>();
      Self->Actions.setGlobalScope(NewScope);
      Self->Actions.setCurScope(NewScope);
      Self->Actions.CurFuncDef = nullptr;
    }

    /// Входит в новую область видимости класса или функции.
    ///
    /// \p D — определение класса (ClassDef) или функции (FuncDef), в тело
    /// которого выполняется вход.
    SemaScope(Analysis *Self, Declaration *D)
        : Self(Self), ParentFunc(Self->Actions.CurFuncDef) {
      std::shared_ptr<Scope> Parent = Self->Actions.getCurScope();
      Scope::ScopeKind K = Scope::ScopeKind::Class;
      Self->Actions.CurFuncDef = nullptr;
      if (FuncDef *F = dyn_cast<FuncDef>(D)) {
        K = Scope::ScopeKind::Func;
        Self->Actions.CurFuncDef = F;
      }
      std::shared_ptr<Scope> NewScope = std::make_shared<Scope>(Parent, K);
      Self->Actions.setCurScope(NewScope);
    }

    ~SemaScope() {
      std::shared_ptr<Scope> S = Self->Actions.getCurScope();
      Self->Actions.actOnPopScope(S.get());
      Self->Actions.CurFuncDef = ParentFunc;
      if (std::shared_ptr<Scope> Parent = S->getParent()) {
        Self->Actions.setCurScope(Parent);
      } else {
        Self->Actions.setCurScope(nullptr);
        Self->Actions.setGlobalScope(nullptr);
      }
    }

  private:
    Analysis *Self;
    FuncDef *ParentFunc = nullptr;
  };

public:
  Analysis(Sema &Actions) : Actions(Actions) {}

  //===----------------------------------------------------------------------===//
  // Посещение верхнего уровня — Program
  //===----------------------------------------------------------------------===//
  void visit(Program *P) {
    SemaScope ScopeGuard(this);
    Actions.setGlobalScope(Actions.getCurScope());
    Actions.initializeGlobalScope();
    SmallVector<FuncDef *> PostFuncs;
    SmallVector<ClassDef *> PostClasses;
    for (Declaration *D : P->getDeclarations()) {
      if (VarDef *V = dyn_cast<VarDef>(D))
        visit(V->getValue());
      Actions.actOnDeclaration(D);
      if (ClassDef *CD = dyn_cast<ClassDef>(D))
        PostClasses.push_back(CD);
      else if (FuncDef *FD = dyn_cast<FuncDef>(D))
        PostFuncs.push_back(FD);
    }
    for (Stmt *S : P->getStatements())
      visit(S);
    for (ClassDef *CD : PostClasses) {
      visit(CD);
    }
    for (FuncDef *FD : PostFuncs) {
      visit(FD);
    }
  }

  //===----------------------------------------------------------------------===//
  // Посетители объявлений (Declaration visitors)
  //===----------------------------------------------------------------------===//
  void visitClassDef(ClassDef *C) {
    SemaScope ScopeGuard(this, C);
    SmallVector<FuncDef *> PostFuncs;
    for (Declaration *D : C->getDeclarations()) {
      if (VarDef *V = dyn_cast<VarDef>(D))
        visit(V->getValue());
      Actions.actOnDeclaration(D);
      if (FuncDef *FD = dyn_cast<FuncDef>(D))
        PostFuncs.push_back(FD);
    }
    for (FuncDef *F : PostFuncs) {
      visit(F);
    }
    Actions.actOnClassDef(C);
  }

  void visitFuncDef(FuncDef *F) {
    SemaScope ScopeGuard(this, F);
    for (ParamDecl *P : F->getParams())
      Actions.actOnDeclaration(P);
    SmallVector<FuncDef *> PostFunc;
    for (Declaration *D : F->getDeclarations()) {
      if (VarDef *V = dyn_cast<VarDef>(D))
        visit(V->getValue());
      Actions.actOnDeclaration(D);
      if (FuncDef *NF = dyn_cast<FuncDef>(D))
        PostFunc.push_back(NF);
    }
    for (Stmt *S : F->getStatements())
      visit(S);
    Actions.actOnFuncDef(F);
    for (FuncDef *NF : PostFunc)
      visit(NF);
  }

  //===----------------------------------------------------------------------===//
  // Посетители инструкций (Statement visitors)
  //===----------------------------------------------------------------------===//
  void visitAssignStmt(AssignStmt *A) {
    visit(A->getValue());
    for (Expr *T : A->getTargets())
      visit(T);
    Actions.actOnAssignStmt(A);
  }
  void visitExprStmt(ExprStmt *E) { visit(E->getExpr()); }
  void visitReturnStmt(ReturnStmt *R) {
    if (Expr *V = R->getValue())
      visit(V);
    Actions.actOnReturnStmt(R);
  }
  void visitWhileStmt(WhileStmt *W) {
    visit(W->getCondition());
    for (Stmt *S : W->getBody())
      visit(S);
  }
  void visitIfStmt(IfStmt *I) {
    visit(I->getCondition());
    for (Stmt *S : I->getThenBody())
      visit(S);
    for (Stmt *S : I->getElseBody())
      visit(S);
  }
  void visitForStmt(ForStmt *F) {
    visit(F->getTarget());
    visit(F->getIterable());
    for (Stmt *S : F->getBody())
      visit(S);
  }

  //===----------------------------------------------------------------------===//
  // Посетители выражений (Expression visitors)
  //===----------------------------------------------------------------------===//
  void visitBinaryExpr(BinaryExpr *B) {
    visit(B->getLeft());
    visit(B->getRight());
    Actions.actOnBinaryExpr(B);
  }
  void visitCallExpr(CallExpr *C) {
    for (Expr *A : C->getArgs())
      visit(A);
    Actions.actOnCallExpr(C);
  }
  void visitDeclRef(DeclRef *DR) { Actions.actOnDeclRef(DR); }
  void visitIfExpr(IfExpr *I) {
    visit(I->getCondExpr());
    visit(I->getThenExpr());
    visit(I->getElseExpr());
    Actions.actOnIfExpr(I);
  }
  void visitIndexExpr(IndexExpr *I) {
    visit(I->getList());
    visit(I->getIndex());
    Actions.actOnIndexExpr(I);
  }
  void visitListExpr(ListExpr *L) {
    for (Expr *E : L->getElements())
      visit(E);
    Actions.actOnListExpr(L);
  }
  void visitLiteral(Literal *L) { Actions.actOnLiteral(L); }
  void visitMemberExpr(MemberExpr *M) {
    visit(M->getObject());
    Actions.actOnMemberExpr(M);
  }
  void visitMethodCallExpr(MethodCallExpr *MC) {
    visit(MC->getMethod()->getObject());
    for (Expr *A : MC->getArgs())
      visit(A);
    Actions.actOnMethodCallExpr(MC);
  }
  void visitUnaryExpr(UnaryExpr *U) {
    visit(U->getOperand());
    Actions.actOnUnaryExpr(U);
  }

private:
  Sema &Actions;
};

//===----------------------------------------------------------------------===//
// ПРИМЕЧАНИЕ:
// Все методы ниже являются заглушками. Замените тело каждой заглушки реальной
// реализацией в соответствии с комментариями TODO.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Sema: construction
//===----------------------------------------------------------------------===//
Sema::Sema(Lexer &L, ASTContext &C)
    : TheLexer(L), Diags(TheLexer.getDiagnostics()), Ctx(C) {}

//===----------------------------------------------------------------------===//
// Sema: initialization
//===----------------------------------------------------------------------===//
void Sema::initialize() {
  ClassDef *ObjCD = Ctx.getObjectClass();
  ClassDef *IntCD = Ctx.getIntClass();
  ClassDef *StrCD = Ctx.getStrClass();
  ClassDef *BoolCD = Ctx.getBoolClass();
  ClassDef *NoneCD = Ctx.getNoneClass();
  FuncDef *PrintFD = Ctx.getPrintFunc();
  FuncDef *InputFD = Ctx.getInputFunc();
  FuncDef *LenFD = Ctx.getLenFunc();
  IdResolver.addDecl(ObjCD);
  IdResolver.addDecl(IntCD);
  IdResolver.addDecl(StrCD);
  IdResolver.addDecl(BoolCD);
  IdResolver.addDecl(NoneCD);
  IdResolver.addDecl(PrintFD);
  IdResolver.addDecl(InputFD);
  IdResolver.addDecl(LenFD);
}

void Sema::initializeGlobalScope() {
  ClassDef *ObjCD = Ctx.getObjectClass();
  ClassDef *IntCD = Ctx.getIntClass();
  ClassDef *StrCD = Ctx.getStrClass();
  ClassDef *BoolCD = Ctx.getBoolClass();
  ClassDef *NoneCD = Ctx.getNoneClass();
  FuncDef *PrintFD = Ctx.getPrintFunc();
  FuncDef *InputFD = Ctx.getInputFunc();
  FuncDef *LenFD = Ctx.getLenFunc();
  GlobalScope->addDecl(ObjCD);
  GlobalScope->addDecl(IntCD);
  GlobalScope->addDecl(StrCD);
  GlobalScope->addDecl(BoolCD);
  GlobalScope->addDecl(NoneCD);
  GlobalScope->addDecl(PrintFD);
  GlobalScope->addDecl(InputFD);
  GlobalScope->addDecl(LenFD);
}

//===----------------------------------------------------------------------===//
// Sema: entry point
//===----------------------------------------------------------------------===//
void Sema::run() {
  Analysis V(*this);
  V.visit(Ctx.getProgram());
}

//===----------------------------------------------------------------------===//
// Sema: scope management
//===----------------------------------------------------------------------===//

void Sema::addDeclaration(Declaration *D) {
  if (!CurScope->isDeclInScope(D)) {
    CurScope->addDecl(D);
    IdResolver.addDecl(D);
  }
}

void Sema::actOnPopScope(Scope *S) {
  for (Declaration *D : S->getDecls())
    IdResolver.removeDecl(D);
}

//===----------------------------------------------------------------------===//
// Sema: функции проверок
//===----------------------------------------------------------------------===//

bool Sema::checkDuplication(Declaration *D) {
  if (lookupName(CurScope.get(), D->getSymbolInfo())) {
    Diags.emitError(D->getLocation().Start, diag::err_dup_decl) << D->getName();
    return false;
  }
  return true;
}

bool Sema::checkClassShadow(Declaration *D) {
  for (auto It = IdResolver.begin(D->getSymbolInfo()); It != IdResolver.end();
       ++It) {
    if (isa<ClassDef>(*It)) {
      Diags.emitError(D->getLocation().Start, diag::err_bad_shadow)
          << D->getName();
      return false;
    }
  }
  return true;
}

bool Sema::checkNonlocalDecl(NonLocalDecl *NLD) {
  for (auto It = IdResolver.begin(NLD->getSymbolInfo()); It != IdResolver.end();
       ++It) {
    if (VarDef *V = dyn_cast<VarDef>(*It)) {
      Scope *DS = getScopeForDecl(CurScope.get(), V);
      if (DS && DS->isFunc())
        return true;
    }
  }
  Diags.emitError(NLD->getLocation().Start, diag::err_not_nonlocal)
      << NLD->getName();
  return false;
}

bool Sema::checkGlobalDecl(GlobalDecl *GD) {
  if (Declaration *D = lookupName(GlobalScope.get(), GD->getSymbolInfo())) {
    if (isa<VarDef>(D))
      return true;
    Diags.emitError(GD->getLocation().Start, diag::err_not_global)
        << GD->getName();
    return false;
  }
  Diags.emitError(GD->getLocation().Start, diag::err_not_global)
      << GD->getName();
  return false;
}

bool Sema::checkSuperClass(ClassDef *D) {
  SymbolInfo *SI = D->getSuperClass()->getSymbolInfo();
  Declaration *SupDecl = lookupDecl(SI);
  if (!SupDecl) {
    Diags.emitError(D->getLocation().Start, diag::err_supclass_not_def)
        << D->getSuperClass()->getName();
    return false;
  }
  ClassDef *S = dyn_cast<ClassDef>(SupDecl);
  if (!S) {
    Diags.emitError(D->getLocation().Start, diag::err_supclass_isnot_class)
        << D->getSuperClass()->getName();
    return false;
  }
  if (Ctx.isIntClass(S) || Ctx.isStrClass(S) || Ctx.isBoolClass(S)) {
    Diags.emitError(D->getLocation().Start, diag::err_supclass_is_special_class)
        << D->getSuperClass()->getName();
    return false;
  }
  return true;
}

bool Sema::checkFirstMethodParam(ClassDef *CD, FuncDef *FD) {
  if (FD->getParams().empty()) {
    Diags.emitError(FD->getLocation().Start, diag::err_first_method_param)
        << FD->getName();
    return false;
  }
  TypeAnnotation *P0Ty = FD->getParams().front()->getType();
  if (ClassType *CT = dyn_cast<ClassType>(P0Ty)) {
    if (CT->getClassName() == CD->getName())
      return true;
  }
  Diags.emitError(FD->getLocation().Start, diag::err_first_method_param)
      << FD->getName();
  return false;
}

bool Sema::checkMethodOverride(FuncDef *OM, FuncDef *M) {
  if (OM->getParams().size() != M->getParams().size())
    return false;
  for (size_t I = 1, E = OM->getParams().size(); I < E; ++I) {
    if (!isSameType(OM->getParams()[I]->getType(), M->getParams()[I]->getType()))
      return false;
  }
  ValueType *OMRet = convertATypeToVType(OM->getReturnType());
  ValueType *MRet = convertATypeToVType(M->getReturnType());
  return isAssignementCompatibility(OMRet, MRet);
}

bool Sema::checkClassAttrs(ClassDef *D) {
  bool Ok = true;
  for (Declaration *Decl : D->getDeclarations()) {
    if (FuncDef *F = dyn_cast<FuncDef>(Decl)) {
      Ok &= checkFirstMethodParam(D, F);
      if (lookupMember(getSuperClass(D),
                       Ctx.createDeclRef(F->getLocation(), F->getSymbolInfo()))) {
        Diags.emitError(F->getLocation().Start, diag::err_redefine_attr)
            << F->getName();
        Ok = false;
      }
      if (FuncDef *SM = lookupMethod(getSuperClass(D), F->getName())) {
        if (!checkMethodOverride(F, SM)) {
          Diags.emitError(F->getLocation().Start, diag::err_method_override)
              << F->getName();
          Ok = false;
        }
      }
    }
    if (VarDef *V = dyn_cast<VarDef>(Decl)) {
      if (lookupMember(getSuperClass(D),
                       Ctx.createDeclRef(V->getLocation(), V->getSymbolInfo()))) {
        Diags.emitError(V->getLocation().Start, diag::err_redefine_attr)
            << V->getName();
        Ok = false;
      }
      if (lookupMethod(getSuperClass(D), V->getName())) {
        Diags.emitError(V->getLocation().Start, diag::err_redefine_attr)
            << V->getName();
        Ok = false;
      }
    }
  }
  return Ok;
}

bool Sema::checkClassDef(ClassDef *D) {
  return checkSuperClass(D) && checkClassAttrs(D);
}

bool Sema::checkAssignTarget(Expr *E) {
  if (DeclRef *DR = dyn_cast<DeclRef>(E)) {
    if (Declaration *D = lookupName(CurScope.get(), DR->getSymbolInfo()))
      return isa<VarDef>(D) || isa<ParamDecl>(D) || isa<GlobalDecl>(D) ||
             isa<NonLocalDecl>(D);
    Diags.emitError(E->getLocation().Start, diag::err_bad_local_assign)
        << DR->getName();
    return false;
  }
  return true;
}

bool Sema::checkReturnStmt(ReturnStmt *S) {
  if (CurScope->isGlobal()) {
    Diags.emitError(S->getLocation().Start, diag::err_bad_return_top);
    return false;
  }
  return true;
}

bool Sema::checkReturnMissing(FuncDef *F) {
  ValueType *RT = convertATypeToVType(F->getReturnType());
  if (RT->isNone() || RT == Ctx.getObjectTy())
    return true;
  std::unique_ptr<CFG> G = CFG::buildCFG(F);
  for (CFGBlock *Pred : G->getExit().preds()) {
    if (Pred->empty())
      continue;
    CFGElement Back = Pred->back();
    if (!(Back.isStmt() && isa<ReturnStmt>(Back.getStmt()))) {
      Diags.emitError(F->getLocation().Start, diag::err_maybe_falloff_nonvoid)
          << F->getName();
      return false;
    }
  }
  return true;
}

bool Sema::checkTypeAnnotation(TypeAnnotation *TA) {
  if (ClassType *CT = dyn_cast<ClassType>(TA)) {
    if (!resolveClassType(CT)) {
      Diags.emitError(TA->getLocation().Start, diag::err_invalid_type_annotation)
          << CT->getClassName();
      return false;
    }
    return true;
  }
  if (ListType *LT = dyn_cast<ListType>(TA))
    return checkTypeAnnotation(LT->getElementType());
  return true;
}

//===----------------------------------------------------------------------===//
//  Sema: реализация действий (actOn)
//===----------------------------------------------------------------------===//

void Sema::actOnDeclaration(Declaration *D) {
  if (!checkDuplication(D))
    return;
  if ((isa<VarDef>(D) || isa<ParamDecl>(D) || isa<FuncDef>(D)) &&
      !checkClassShadow(D))
    return;
  if (ParamDecl *P = dyn_cast<ParamDecl>(D))
    return actOnParamDecl(P);
  if (VarDef *V = dyn_cast<VarDef>(D))
    return actOnVarDef(V);
  if (NonLocalDecl *N = dyn_cast<NonLocalDecl>(D))
    return actOnNonlocalDecl(N);
  if (GlobalDecl *G = dyn_cast<GlobalDecl>(D))
    return actOnGlobalDecl(G);
  if (FuncDef *F = dyn_cast<FuncDef>(D)) {
    if (!checkTypeAnnotation(F->getReturnType()))
      return;
  }
  addDeclaration(D);
}

void Sema::actOnClassDef(ClassDef *C) {
  checkClassDef(C);
}

void Sema::actOnFuncDef(FuncDef *F) {
  checkReturnMissing(F);
}

void Sema::actOnParamDecl(ParamDecl *P) {
  if (checkTypeAnnotation(P->getType()))
    addDeclaration(P);
}

void Sema::actOnVarDef(VarDef *V) {
  if (!checkTypeAnnotation(V->getType()))
    return;
  ValueType *DeclTy = convertATypeToVType(V->getType());
  ValueType *InitTy = dyn_cast_or_null<ValueType>(V->getValue()->getInferredType());
  if (!InitTy)
    if (Literal *L = dyn_cast<Literal>(V->getValue())) {
      actOnLiteral(L);
      InitTy = dyn_cast_or_null<ValueType>(L->getInferredType());
    }
  if (!InitTy)
    InitTy = Ctx.getObjectTy();
  if (!isAssignementCompatibility(InitTy, DeclTy)) {
    Diags.emitError(V->getLocation().Start, diag::err_tc_assign)
        << *DeclTy << *InitTy;
    return;
  }
  addDeclaration(V);
}

void Sema::actOnNonlocalDecl(NonLocalDecl *D) {
  if (checkNonlocalDecl(D))
    addDeclaration(D);
}

void Sema::actOnGlobalDecl(GlobalDecl *D) {
  if (checkGlobalDecl(D))
    addDeclaration(D);
}

void Sema::actOnAssignStmt(AssignStmt *A) {
  ValueType *RHS = dyn_cast_or_null<ValueType>(A->getValue()->getInferredType());
  if (!RHS)
    RHS = Ctx.getObjectTy();
  for (Expr *Target : A->getTargets()) {
    ValueType *LHS = dyn_cast_or_null<ValueType>(Target->getInferredType());
    if (!LHS)
      LHS = Ctx.getObjectTy();
    if (!isAssignementCompatibility(RHS, LHS))
      Diags.emitError(Target->getLocation().Start, diag::err_tc_assign)
          << *LHS << *RHS;
    checkAssignTarget(Target);
  }
}

void Sema::actOnReturnStmt(ReturnStmt *R) {
  if (!checkReturnStmt(R))
    return;
  ValueType *RetTy = convertATypeToVType(CurFuncDef->getReturnType());
  ValueType *ValTy = Ctx.getNoneTy();
  if (Expr *E = R->getValue())
    ValTy = dyn_cast_or_null<ValueType>(E->getInferredType());
  if (!ValTy)
    ValTy = Ctx.getObjectTy();
  if (!isAssignementCompatibility(ValTy, RetTy))
    Diags.emitError(R->getLocation().Start, diag::err_tc_assign)
        << *RetTy << *ValTy;
}

void Sema::actOnBinaryExpr(BinaryExpr *B) {
  ValueType *LTy = dyn_cast_or_null<ValueType>(B->getLeft()->getInferredType());
  ValueType *RTy = dyn_cast_or_null<ValueType>(B->getRight()->getInferredType());
  if (!LTy)
    LTy = Ctx.getObjectTy();
  if (!RTy)
    RTy = Ctx.getObjectTy();
  auto Err = [&]() {
    Diags.emitError(B->getLocation().Start, diag::err_tc_binary)
        << B->getOpKindStr() << *LTy << *RTy;
    B->setInferredType(Ctx.getObjectTy());
  };
  switch (B->getOpKind()) {
  case BinaryExpr::OpKind::And:
  case BinaryExpr::OpKind::Or:
    if (LTy->isBool() && RTy->isBool())
      B->setInferredType(Ctx.getBoolTy());
    else
      Err();
    break;
  case BinaryExpr::OpKind::Add:
    if (LTy->isInt() && RTy->isInt())
      B->setInferredType(Ctx.getIntTy());
    else if (LTy->isStr() && RTy->isStr())
      B->setInferredType(Ctx.getStrTy());
    else if (auto *LL = dyn_cast<ListValueType>(LTy)) {
      if (auto *RL = dyn_cast<ListValueType>(RTy)) {
        B->setInferredType(Ctx.getListVType(join(LL->getElementType(), RL->getElementType())));
      } else
        Err();
    } else
      Err();
    break;
  case BinaryExpr::OpKind::Sub:
  case BinaryExpr::OpKind::Mul:
  case BinaryExpr::OpKind::FloorDiv:
  case BinaryExpr::OpKind::Mod:
    if (LTy->isInt() && RTy->isInt())
      B->setInferredType(Ctx.getIntTy());
    else
      Err();
    break;
  case BinaryExpr::OpKind::EqCmp:
  case BinaryExpr::OpKind::NEqCmp:
    B->setInferredType(Ctx.getBoolTy());
    break;
  case BinaryExpr::OpKind::LEqCmp:
  case BinaryExpr::OpKind::GEqCmp:
  case BinaryExpr::OpKind::LCmp:
  case BinaryExpr::OpKind::GCmp:
    if ((LTy->isInt() && RTy->isInt()) || (LTy->isStr() && RTy->isStr()) ||
        (LTy->isBool() && RTy->isBool()))
      B->setInferredType(Ctx.getBoolTy());
    else
      Err();
    break;
  case BinaryExpr::OpKind::Is:
    if ((!LTy->isInt() && !LTy->isBool() && !LTy->isStr()) &&
        (!RTy->isInt() && !RTy->isBool() && !RTy->isStr()))
      B->setInferredType(Ctx.getBoolTy());
    else
      Err();
    break;
  }
}

void Sema::actOnIndexExpr(IndexExpr *I) {
  ValueType *BTy = dyn_cast_or_null<ValueType>(I->getList()->getInferredType());
  ValueType *ITy = dyn_cast_or_null<ValueType>(I->getIndex()->getInferredType());
  if (!BTy)
    BTy = Ctx.getObjectTy();
  if (!ITy)
    ITy = Ctx.getObjectTy();
  if (!ITy->isInt())
    Diags.emitError(I->getIndex()->getLocation().Start, diag::err_bad_index)
        << *ITy;
  if (BTy->isStr())
    I->setInferredType(Ctx.getStrTy());
  else if (ListValueType *LT = dyn_cast<ListValueType>(BTy))
    I->setInferredType(LT->getElementType());
  else {
    Diags.emitError(I->getLocation().Start, diag::err_cannot_index) << *BTy;
    I->setInferredType(Ctx.getObjectTy());
  }
}

void Sema::actOnListExpr(ListExpr *L) {
  if (L->getElements().empty()) {
    L->setInferredType(Ctx.getListVType(Ctx.getEmptyTy()));
    return;
  }
  ValueType *ElTy = dyn_cast_or_null<ValueType>(L->getElements().front()->getInferredType());
  if (!ElTy)
    ElTy = Ctx.getObjectTy();
  for (Expr *E : L->getElements().drop_front())
    ElTy = join(ElTy, dyn_cast_or_null<ValueType>(E->getInferredType())
                          ? dyn_cast<ValueType>(E->getInferredType())
                          : Ctx.getObjectTy());
  L->setInferredType(Ctx.getListVType(ElTy));
}

void Sema::actOnLiteral(Literal *L) {
  llvm::TypeSwitch<Literal *>(L)
      .Case([&](BooleanLiteral *) { L->setInferredType(Ctx.getBoolTy()); })
      .Case([&](IntegerLiteral *) { L->setInferredType(Ctx.getIntTy()); })
      .Case([&](NoneLiteral *) { L->setInferredType(Ctx.getNoneTy()); })
      .Case([&](StringLiteral *) { L->setInferredType(Ctx.getStrTy()); });
}

void Sema::actOnCallExpr(CallExpr *CE) {
  DeclRef *DR = dyn_cast<DeclRef>(CE->getFunction());
  if (!DR) {
    Diags.emitError(CE->getLocation().Start, diag::err_not_func) << "<expr>";
    CE->setInferredType(Ctx.getObjectTy());
    return;
  }
  Declaration *D = lookupDecl(DR);
  if (FuncDef *FD = dyn_cast_or_null<FuncDef>(D)) {
    if (FD->getParams().size() != CE->getArgs().size()) {
      Diags.emitError(CE->getLocation().Start, diag::err_args_count)
          << int(FD->getParams().size()) << int(CE->getArgs().size());
    } else {
      for (unsigned I = 0; I < CE->getArgs().size(); ++I) {
        ValueType *Expected = convertATypeToVType(FD->getParams()[I]->getType());
        ValueType *Got = dyn_cast_or_null<ValueType>(CE->getArgs()[I]->getInferredType());
        if (!Got)
          Got = Ctx.getObjectTy();
        if (!isAssignementCompatibility(Got, Expected))
          Diags.emitError(CE->getLocation().Start, diag::err_tc_call)
              << *Expected << *Got << int(I);
      }
    }
    CE->setInferredType(convertATypeToVType(FD->getReturnType()));
    return;
  }
  if (ClassDef *CD = dyn_cast_or_null<ClassDef>(D)) {
    CE->setInferredType(Ctx.getClassVType(CD));
    return;
  }
  Diags.emitError(CE->getLocation().Start, diag::err_not_func) << DR->getName();
  CE->setInferredType(Ctx.getObjectTy());
}

void Sema::actOnDeclRef(DeclRef *DR) {
  Declaration *D = lookupDecl(DR);
  if (!D) {
    DR->setInferredType(Ctx.getObjectTy());
    return;
  }
  DR->setDeclInfo(D);
  if (FuncDef *FD = dyn_cast<FuncDef>(D)) {
    Diags.emitError(DR->getLocation().Start, diag::err_not_variable)
        << FD->getName();
    DR->setInferredType(getFuncType(FD));
    return;
  }
  if (VarDef *V = dyn_cast<VarDef>(D))
    return DR->setInferredType(convertATypeToVType(V->getType()));
  if (ParamDecl *P = dyn_cast<ParamDecl>(D))
    return DR->setInferredType(convertATypeToVType(P->getType()));
  if (ClassDef *CD = dyn_cast<ClassDef>(D))
    return DR->setInferredType(Ctx.getClassVType(CD));
  if (GlobalDecl *GD = dyn_cast<GlobalDecl>(D))
    D = lookupName(GlobalScope.get(), GD->getSymbolInfo());
  else if (NonLocalDecl *NLD = dyn_cast<NonLocalDecl>(D)) {
    for (auto It = IdResolver.begin(NLD->getSymbolInfo()); It != IdResolver.end();
         ++It) {
      if (VarDef *V = dyn_cast<VarDef>(*It))
        if (Scope *S = getScopeForDecl(CurScope.get(), V); S && S->isFunc()) {
          D = V;
          break;
        }
    }
  }
  if (VarDef *V = dyn_cast_or_null<VarDef>(D))
    DR->setInferredType(convertATypeToVType(V->getType()));
}

void Sema::actOnIfExpr(IfExpr *I) {
  ValueType *TT = dyn_cast_or_null<ValueType>(I->getThenExpr()->getInferredType());
  ValueType *ET = dyn_cast_or_null<ValueType>(I->getElseExpr()->getInferredType());
  if (!TT)
    TT = Ctx.getObjectTy();
  if (!ET)
    ET = Ctx.getObjectTy();
  I->setInferredType(join(TT, ET));
}

void Sema::actOnMemberExpr(MemberExpr *M) {
  ValueType *ObjTy = dyn_cast_or_null<ValueType>(M->getObject()->getInferredType());
  if (!ObjTy)
    ObjTy = Ctx.getObjectTy();
  if (ClassValueType *CVT = dyn_cast<ClassValueType>(ObjTy)) {
    if (VarDef *V = lookupMember(CVT, M->getMember())) {
      M->setInferredType(convertATypeToVType(V->getType()));
      return;
    }
    Diags.emitError(M->getLocation().Start, diag::err_no_attribute)
        << M->getMember()->getName() << CVT->getClassName();
    M->setInferredType(Ctx.getObjectTy());
    return;
  }
  Diags.emitError(M->getLocation().Start, diag::err_cannot_access_member)
      << *ObjTy;
  M->setInferredType(Ctx.getObjectTy());
}

void Sema::actOnMethodCallExpr(MethodCallExpr *M) {
  ValueType *ObjTy = dyn_cast_or_null<ValueType>(M->getMethod()->getObject()->getInferredType());
  if (!ObjTy)
    ObjTy = Ctx.getObjectTy();
  ClassValueType *CVT = dyn_cast<ClassValueType>(ObjTy);
  if (!CVT) {
    Diags.emitError(M->getLocation().Start, diag::err_cannot_access_member)
        << *ObjTy;
    M->setInferredType(Ctx.getObjectTy());
    return;
  }
  FuncDef *FD = lookupMethod(CVT, M->getMethod()->getMember());
  if (!FD) {
    Diags.emitError(M->getLocation().Start, diag::err_no_method)
        << M->getMethod()->getMember()->getName() << CVT->getClassName();
    M->setInferredType(Ctx.getObjectTy());
    return;
  }
  unsigned Expected = FD->getParams().size() - 1;
  if (Expected != M->getArgs().size()) {
    Diags.emitError(M->getLocation().Start, diag::err_args_count)
        << int(Expected) << int(M->getArgs().size());
  } else {
    for (unsigned I = 0; I < M->getArgs().size(); ++I) {
      ValueType *ETy = convertATypeToVType(FD->getParams()[I + 1]->getType());
      ValueType *GTy = dyn_cast_or_null<ValueType>(M->getArgs()[I]->getInferredType());
      if (!GTy)
        GTy = Ctx.getObjectTy();
      if (!isAssignementCompatibility(GTy, ETy))
        Diags.emitError(M->getLocation().Start, diag::err_tc_call)
            << *ETy << *GTy << int(I);
    }
  }
  M->setInferredType(convertATypeToVType(FD->getReturnType()));
}

void Sema::actOnUnaryExpr(UnaryExpr *U) {
  ValueType *OpTy = dyn_cast_or_null<ValueType>(U->getOperand()->getInferredType());
  if (!OpTy)
    OpTy = Ctx.getObjectTy();
  switch (U->getOpKind()) {
  case UnaryExpr::OpKind::Minus:
    if (!OpTy->isInt())
      Diags.emitError(U->getLocation().Start, diag::err_tc_unary) << "-" << *OpTy;
    U->setInferredType(Ctx.getIntTy());
    break;
  case UnaryExpr::OpKind::Not:
    if (!OpTy->isBool())
      Diags.emitError(U->getLocation().Start, diag::err_tc_unary) << "not" << *OpTy;
    U->setInferredType(Ctx.getBoolTy());
    break;
  }
}

//===----------------------------------------------------------------------===//
// Sema: вспомогательные функции поиска (lookup helpers)
//===----------------------------------------------------------------------===//

Scope *Sema::getScopeForDecl(Scope *S, Declaration *D) {
  for (Scope *Cur = S; Cur; Cur = Cur->getParent().get())
    if (Cur->isDeclInScope(D))
      return Cur;
  return nullptr;
}

ClassDef *Sema::getSuperClass(ClassDef *CD) {
  if (!CD)
    return nullptr;
  if (!CD->getSuperClass())
    return nullptr;
  return resolveClassType(Ctx.createClassType(CD->getSuperClass()->getLocation(),
                                              CD->getSuperClass()->getName()));
}

bool Sema::isSameType(TypeAnnotation *TyA, TypeAnnotation *TyB) {
  if (TyA->getKind() != TyB->getKind())
    return false;
  if (ClassType *A = dyn_cast<ClassType>(TyA)) {
    ClassType *B = cast<ClassType>(TyB);
    return A->getClassName() == B->getClassName();
  }
  ListType *A = cast<ListType>(TyA);
  ListType *B = cast<ListType>(TyB);
  return isSameType(A->getElementType(), B->getElementType());
}

ValueType *Sema::convertATypeToVType(TypeAnnotation *TA) {
  if (ValueType *VT = TA->getValueType())
    return VT;
  ValueType *VT = nullptr;
  if (ClassType *CT = dyn_cast<ClassType>(TA)) {
    if (ClassDef *CD = resolveClassType(CT))
      VT = Ctx.getClassVType(CD);
  } else if (ListType *LT = dyn_cast<ListType>(TA)) {
    VT = Ctx.getListVType(convertATypeToVType(LT->getElementType()));
  }
  TA->setValueType(VT);
  return VT ? VT : Ctx.getObjectTy();
}

FuncType *Sema::getFuncType(FuncDef *FD) {
  ValueTypeList Params;
  for (ParamDecl *P : FD->getParams())
    Params.push_back(convertATypeToVType(P->getType()));
  return Ctx.getFuncType(Params, convertATypeToVType(FD->getReturnType()));
}

Declaration *Sema::lookupName(Scope *S, SymbolInfo *SI) {
  for (Declaration *D : S->getDecls())
    if (D->getSymbolInfo() == SI)
      return D;
  return nullptr;
}

Declaration *Sema::lookupDecl(DeclRef *DR) {
  SymbolInfo *SI = DR->getSymbolInfo();
  return lookupDecl(SI);
}

Declaration *Sema::lookupDecl(SymbolInfo *SI) {
  auto It = IdResolver.begin(SI);
  if (It == IdResolver.end())
    return nullptr;
  return *It;
}

VarDef *Sema::lookupMember(ClassDef *CD, DeclRef *Member) {
  for (ClassDef *Cur = CD; Cur; Cur = getSuperClass(Cur)) {
    for (Declaration *D : Cur->getDeclarations())
      if (VarDef *V = dyn_cast<VarDef>(D))
        if (V->getName() == Member->getName())
          return V;
  }
  return nullptr;
}

VarDef *Sema::lookupMember(ClassValueType *CVT, DeclRef *Member) {
  return lookupMember(CVT->getClassDef(), Member);
}

FuncDef *Sema::lookupMethod(ClassDef *CD, DeclRef *Member) {
  return lookupMethod(CD, Member->getName());
}

FuncDef *Sema::lookupMethod(ClassDef *CD, StringRef Name) {
  for (ClassDef *Cur = CD; Cur; Cur = getSuperClass(Cur)) {
    for (Declaration *D : Cur->getDeclarations())
      if (FuncDef *F = dyn_cast<FuncDef>(D))
        if (F->getName() == Name)
          return F;
  }
  return nullptr;
}

FuncDef *Sema::lookupMethod(ClassValueType *CVT, DeclRef *Member) {
  return lookupMethod(CVT->getClassDef(), Member);
}

ClassDef *Sema::resolveClassType(ClassType *CT) {
  SymbolInfo *SI = &TheLexer.getSymbolTable().get(CT->getClassName());
  for (auto It = IdResolver.begin(SI); It != IdResolver.end(); ++It)
    if (ClassDef *CD = dyn_cast<ClassDef>(*It))
      return CD;
  return nullptr;
}

void Sema::getClassHierarchy(const ClassValueType *CVT,
                             SmallVector<ClassDef *> &Hierarchy) {
  for (ClassDef *CD = CVT->getClassDef(); CD; CD = getSuperClass(CD))
    Hierarchy.push_back(CD);
}

ValueType *Sema::join(ValueType *LTy, ValueType *RTy) {
  if (LTy == RTy)
    return LTy;
  if (auto *LL = dyn_cast<ListValueType>(LTy)) {
    if (auto *RL = dyn_cast<ListValueType>(RTy))
      return Ctx.getListVType(join(LL->getElementType(), RL->getElementType()));
    return Ctx.getObjectTy();
  }
  if (auto *LC = dyn_cast<ClassValueType>(LTy)) {
    if (auto *RC = dyn_cast<ClassValueType>(RTy)) {
      SmallVector<ClassDef *> LH, RH;
      getClassHierarchy(LC, LH);
      getClassHierarchy(RC, RH);
      llvm::SmallPtrSet<ClassDef *, 16> RSet(RH.begin(), RH.end());
      for (ClassDef *C : LH)
        if (RSet.contains(C))
          return Ctx.getClassVType(C);
    }
  }
  return Ctx.getObjectTy();
}

bool Sema::isAssignementCompatibility(const ValueType *Sub,
                                      const ValueType *Super) {
  if (Sub == Super)
    return true;
  if (Sub->isNone()) {
    if (isa<ClassValueType>(Super) && !Super->isInt() && !Super->isStr() &&
        !Super->isBool())
      return true;
    if (auto *SL = dyn_cast<ListValueType>(Super))
      return !SL->getElementType()->isInt() && !SL->getElementType()->isStr() &&
             !SL->getElementType()->isBool();
  }
  if (auto *SubC = dyn_cast<ClassValueType>(Sub))
    if (auto *SupC = dyn_cast<ClassValueType>(Super)) {
      for (ClassDef *CD = SubC->getClassDef(); CD; CD = getSuperClass(CD))
        if (CD == SupC->getClassDef())
          return true;
    }
  if (auto *SubL = dyn_cast<ListValueType>(Sub))
    if (auto *SupL = dyn_cast<ListValueType>(Super))
      return isAssignementCompatibility(SubL->getElementType(),
                                        SupL->getElementType());
  return false;
}

} // namespace chocopy
