#include "chocopy-llvm/Sema/Sema.h"
#include "chocopy-llvm/AST/ASTContext.h"
#include "chocopy-llvm/AST/RecursiveASTVisitor.h"
#include "chocopy-llvm/AST/Type.h"
#include "chocopy-llvm/Analysis/CFG.h"
#include "chocopy-llvm/Basic/Diagnostic.h"
#include "chocopy-llvm/Lexer/Lexer.h"
#include "chocopy-llvm/Sema/Scope.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/TypeSwitch.h>

namespace chocopy {
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
        llvm::raw_string_ostream(Str) << LVT->getElementType();
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

class Sema::Analysis : public RecursiveASTVisitor<Analysis> {
  using Base = RecursiveASTVisitor<Analysis>;

  class SemaScope {
  public:
    SemaScope() = delete;
    SemaScope(const SemaScope &) = delete;
    SemaScope(SemaScope &&) = delete;
    SemaScope &operator=(const SemaScope &) = delete;
    SemaScope &operator=(SemaScope &&) = delete;

    SemaScope(Analysis *Self, Scope::ScopeKind Kind = Scope::ScopeKind::Global)
        : Self(Self) {
      std::shared_ptr<Scope> NewScope =
          std::make_shared<Scope>(Self->Actions.getCurScope(), Kind);
      Self->Actions.setCurScope(NewScope);
    }

    ~SemaScope() {
      std::shared_ptr<Scope> S = Self->Actions.getCurScope();
      Self->Actions.actOnPopScope(S.get());
      Self->Actions.setCurScope(S->getParent());
    }

  private:
    Analysis *Self;
  };

public:
  Analysis(Sema &Actions)
      : Actions(Actions), Diags(Actions.getDiagnosticEngine()) {}

  bool traverseProgram(Program *P) {
    SemaScope ScopeGuard(this);
    Actions.setGlobalScope(Actions.getCurScope());
    Actions.initializeGlobalScope();
    for (Declaration *D : P->getDeclarations())
      handleDeclaration(D);
    return Base::traverseProgram(P);
  }

  bool traverseClassDef(ClassDef *C) {
    /// @todo: Here should be your code
    llvm::report_fatal_error("Classes are not supported! Add support...");
    return Base::traverseClassDef(C);
  }

  bool traverseFuncDef(FuncDef *F) {
    /// @todo: Here should be your code
    llvm::report_fatal_error("Functions are not supported! Add support...");
    return Base::traverseFuncDef(F);
  }

  bool traverseVarDef(VarDef *V) {
    Base::traverseLiteral(V->getValue());
    Actions.actOnVarDef(V);
    return true;
  }

  bool traverseAssignStmt(AssignStmt *A) {
    Base::traverseExpr(A->getValue());
    for (Expr *T : A->getTargets()) {
      Base::traverseExpr(T);
      Actions.checkAssignTarget(T);
    }
    return true;
  }

  bool traverseReturnStmt(ReturnStmt *R) {
    /// @todo: Here should be your code
    llvm::report_fatal_error("Return is not supported! Add support...");
    return true;
  }

  /// Expressions:
  bool traverseBinaryExpr(BinaryExpr *B) {
    Base::traverseExpr(B->getLeft());
    Base::traverseExpr(B->getRight());
    Actions.actOnBinaryExpr(B);
    return true;
  }

  bool traverseCallExpr(CallExpr *C) {
    for (Expr *P : C->getArgs())
      if (DeclRef *R = dyn_cast<DeclRef>(P))
        Actions.actOnDeclRef(R);

    return true;
  }

  bool traverseDeclRef(DeclRef *DR) {
    Actions.actOnDeclRef(DR);
    return true;
  }

  bool traverseBooleanLiteral(BooleanLiteral *B) {
    B->setInferredType(Actions.Ctx.getBoolTy());
    return true;
  }

  bool traverseIntegerLiteral(IntegerLiteral *I) {
    I->setInferredType(Actions.Ctx.getIntTy());
    return true;
  }

  bool traverseNoneLiteral(NoneLiteral *N) {
    N->setInferredType(Actions.Ctx.getNoneTy());
    return true;
  }

  bool traverseStringLiteral(StringLiteral *S) {
    S->setInferredType(Actions.Ctx.getStrTy());
    return true;
  }

  bool traverseMemberExpr(MemberExpr *M) {
    /// @todo: Here should be your code
    llvm::report_fatal_error(
        "Member expressions are not supported! Add support...");
    return true;
  }
  /// Types
  bool visitClassType(ClassType *C) {
    /// @todo: Here should be your code
    llvm::report_fatal_error("Classes are not supported! Add support...");
    Actions.checkTypeAnnotation(C);
    return true;
  }

private:
  void handleDeclaration(Declaration *D) {
    StringRef Name = D->getName();
    Scope *S = Actions.getCurScope().get();
    if (Actions.lookupName(S, D->getSymbolInfo())) {
      Diags.emitError(D->getLocation().Start, diag::err_dup_decl) << Name;
      return;
    }

    /// @todo: Here should be your code
    if (!isa<VarDef>(D))
      llvm::report_fatal_error(
          "Unsupported kind of declaration! Add support...");

    Actions.handleDeclaration(D);
  }

private:
  Sema &Actions;
  DiagnosticsEngine &Diags;
};

Sema::Sema(Lexer &L, ASTContext &C)
    : TheLexer(L), Diags(TheLexer.getDiagnostics()), Ctx(C) {}

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

void Sema::handleDeclaration(Declaration *D) {
  /// @todo: Here should be your code

  if (CurScope->isDeclInScope(D))
    return;
  CurScope->addDecl(D);
  IdResolver.addDecl(D);
}

void Sema::run() {
  Analysis V(*this);
  V.traverseAST(Ctx);
}

void Sema::actOnPopScope(Scope *S) {
  auto Decls = S->getDecls();
  for (Declaration *D : Decls)
    IdResolver.removeDecl(D);
}

bool Sema::checkNonlocalDecl(NonLocalDecl *NLD) {
  /// @todo: Here should be your code
  return true;
}

bool Sema::checkGlobalDecl(GlobalDecl *GD) {
  /// @todo: Here should be your code
  return true;
}

bool Sema::checkSuperClass(ClassDef *D) {
  /// @todo: Here should be your code
  return true;
}

bool Sema::checkClassAttrs(ClassDef *D) {
  /// @todo: Here should be your code
  return true;
}

bool Sema::checkMethodOverride(FuncDef *OM, FuncDef *M) {
  /// @todo: Here should be your code
  return true;
}

bool Sema::checkClassDef(ClassDef *D) {
  /// @todo: Here should be your code
  return true;
}

bool Sema::checkFirstMethodParam(ClassDef *CD, FuncDef *FD) {
  /// @todo: Here should be your code
  return true;
}

bool Sema::checkAssignTarget(Expr *E) {
  DeclRef *DR = dyn_cast<DeclRef>(E);

  /// @todo: Here should be your code
  if (!DR)
    llvm::report_fatal_error("Unsupported assignement target! Add support...");

  IdentifierResolver::iterator It = IdResolver.begin(DR->getSymbolInfo());

  if (It == IdResolver.end() || !CurScope->isDeclInScope(*It)) {
    Diags.emitError(DR->getLocation().Start, diag::err_bad_local_assign)
        << DR->getName();
    return false;
  }
  return true;
}

bool Sema::checkReturnStmt(ReturnStmt *S) {
  /// @todo: Here should be your code
  return true;
}

bool Sema::checkReturnMissing(FuncDef *F) {
  /// @todo: Here should be your code
  return true;
}

bool Sema::checkTypeAnnotation(ClassType *C) {
  /// @todo: Here should be your code
  Diags.emitError(C->getLocation().Start, diag::err_invalid_type_annotation)
      << C->getClassName();
  return false;
}

void Sema::actOnVarDef(VarDef *V) {
  auto &RTy = *cast<ValueType>(V->getValue()->getInferredType());
  auto &LTy = *cast<ValueType>(Ctx.convertAnnotationToVType(V->getType()));
  if (!(RTy <= LTy))
    Diags.emitError(V->getLocation().Start, diag::err_tc_assign) << LTy << RTy;
}

void Sema::actOnBinaryExpr(BinaryExpr *B) {
  ValueType &LTy = *cast<ValueType>(B->getLeft()->getInferredType());
  ValueType &RTy = *cast<ValueType>(B->getRight()->getInferredType());

  bool Err = false;

  switch (B->getOpKind()) {
  case BinaryExpr::OpKind::Add:
    if (LTy.isInt() || RTy.isInt()) {
      Err = &LTy != &RTy;
      B->setInferredType(Ctx.getIntTy());
    } else if (LTy.isStr() || RTy.isStr()) {
      Err = &LTy != &RTy;
      B->setInferredType(Ctx.getStrTy());
    } else {
      auto *LListTy = dyn_cast<ListValueType>(&LTy);
      auto *RListTy = dyn_cast<ListValueType>(&RTy);
      if (LListTy && RListTy)
        Err = LListTy->getElementType() != RListTy->getElementType();
      else
        Err = true;
      B->setInferredType(Err ? static_cast<ValueType *>(Ctx.getObjectTy())
                             : static_cast<ValueType *>(LListTy));
    }
    break;
  case BinaryExpr::OpKind::Sub:
  case BinaryExpr::OpKind::Mul:
  case BinaryExpr::OpKind::Mod:
  case BinaryExpr::OpKind::FloorDiv:
    Err = !LTy.isInt() || !RTy.isInt();
    B->setInferredType(Ctx.getIntTy());
    break;

  case BinaryExpr::OpKind::And:
  case BinaryExpr::OpKind::Or:
    /// @todo: Here should be your code
    break;

  case BinaryExpr::OpKind::EqCmp:
  case BinaryExpr::OpKind::NEqCmp:
    /// @todo: Here should be your code
    break;

  case BinaryExpr::OpKind::LEqCmp:
  case BinaryExpr::OpKind::GEqCmp:
  case BinaryExpr::OpKind::LCmp:
  case BinaryExpr::OpKind::GCmp:
    /// @todo: Here should be your code
    break;

  case BinaryExpr::OpKind::Is:
    /// @todo: Here should be your code
    break;
  }

  if (Err) {
    Diags.emitError(B->getLocation().Start, diag::err_tc_binary)
        << B->getOpKindStr() << LTy << RTy;
  }
}

void Sema::actOnDeclRef(DeclRef *DR) {
  Declaration *D = lookupDecl(DR);

  if (!D || isa<FuncDef>(D)) {
    SMLoc Loc = DR->getLocation().Start;
    Diags.emitError(Loc, diag::err_not_variable) << DR->getName();
    DR->setInferredType(Ctx.getObjectTy());
    return;
  }

  auto convertToValueTy = [DR, this](auto *D) {
    ValueType *VT = Ctx.convertAnnotationToVType(D->getType());
    DR->setDeclInfo(D);
    DR->setInferredType(VT);
  };

  llvm::TypeSwitch<Declaration *>(D)
      .Case<ParamDecl>(convertToValueTy)
      .Case<VarDef>(convertToValueTy)
      .Default([](auto) {
        llvm::report_fatal_error("Unsupported declaration! Add support...");
      });
}

Scope *Sema::getScopeForDecl(Scope *S, Declaration *D) {
  do {
    if (S->isDeclInScope(D))
      return S;
  } while ((S = S->getParent().get()));
  return nullptr;
}

ClassDef *Sema::getSuperClass(ClassDef *CD) {
  /// @todo: Here should be your code
  llvm::report_fatal_error("Unsupported feature! Add support...");
  return nullptr;
}

bool Sema::isSameType(TypeAnnotation *TyA, TypeAnnotation *TyB) {
  /// @todo: Here should be your code
  llvm::report_fatal_error("Unsupported feature! Add support...");
  return false;
}

Declaration *Sema::lookupName(Scope *S, SymbolInfo *SI) {
  auto Decls = S->getDecls();
  auto It = llvm::find_if(
      Decls, [SI](Declaration *D) { return D->getSymbolInfo() == SI; });
  if (It != Decls.end())
    return *It;
  return nullptr;
}

Declaration *Sema::lookupDecl(DeclRef *DR) {
  SymbolInfo *SI = DR->getSymbolInfo();
  IdentifierResolver::iterator I = IdResolver.begin(SI);
  IdentifierResolver::iterator E = IdResolver.end();

  Declaration *D = nullptr;

  for (; !D && I != E; ++I) {
    if (GlobalDecl *GD = dyn_cast<GlobalDecl>(*I)) {
      D = lookupName(GlobalScope.get(), GD->getSymbolInfo());
      return cast<VarDef>(D);
    }

    if (isa<NonLocalDecl>(*I))
      continue;
    D = *I;
  }

  return D;
}
} // namespace chocopy
