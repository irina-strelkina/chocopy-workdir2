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
      Actions.actOnDeclaration(D);
      if (ClassDef *CD = dyn_cast<ClassDef>(D))
        PostClasses.push_back(CD);
      else if (FuncDef *FD = dyn_cast<FuncDef>(D))
        PostFuncs.push_back(FD);
    }
    // TODO: Обработать завершающие инструкции (statements)
    // верхнего уровня и отложенные проверки.
    (void)P; (void)PostFuncs; (void)PostClasses;
  }

  //===----------------------------------------------------------------------===//
  // Посетители объявлений (Declaration visitors)
  //===----------------------------------------------------------------------===//
  void visitClassDef(ClassDef *C) {
    SemaScope ScopeGuard(this, C);
    SmallVector<FuncDef *> PostFuncs;
    for (Declaration *D : C->getDeclarations()) {
      Actions.actOnDeclaration(D);
      if (FuncDef *FD = dyn_cast<FuncDef>(D))
        PostFuncs.push_back(FD);
    }
    for (FuncDef *F : PostFuncs)
      visit(F);
    // TODO: Actions.actOnClassDef(C);
    (void)C;
  }

  void visitFuncDef(FuncDef *F) {
    SemaScope ScopeGuard(this, F);
    for (ParamDecl *P : F->getParams())
      Actions.actOnDeclaration(P);
    SmallVector<FuncDef *> PostFunc;
    for (Declaration *D : F->getDeclarations()) {
      Actions.actOnDeclaration(D);
      if (FuncDef *NF = dyn_cast<FuncDef>(D))
        PostFunc.push_back(NF);
    }
    // TODO: visit all statements in the function body
    // TODO: Actions.actOnFuncDef(F);
    (void)F;
    for (FuncDef *NF : PostFunc)
      visit(NF);
  }

  //===----------------------------------------------------------------------===//
  // Посетители инструкций (Statement visitors)
  //===----------------------------------------------------------------------===//
  void visitAssignStmt(AssignStmt *A) { (void)A; }
  void visitExprStmt(ExprStmt *E) { (void)E; }
  void visitReturnStmt(ReturnStmt *R) { (void)R; }
  void visitWhileStmt(WhileStmt *W) { (void)W; }
  void visitIfStmt(IfStmt *I) { (void)I; }
  void visitForStmt(ForStmt *F) { (void)F; }

  //===----------------------------------------------------------------------===//
  // Посетители выражений (Expression visitors)
  //===----------------------------------------------------------------------===//
  void visitBinaryExpr(BinaryExpr *B) { (void)B; }
  void visitCallExpr(CallExpr *C) { (void)C; }
  void visitDeclRef(DeclRef *DR) { (void)DR; }
  void visitIfExpr(IfExpr *I) { (void)I; }
  void visitIndexExpr(IndexExpr *I) { (void)I; }
  void visitListExpr(ListExpr *L) { (void)L; }
  void visitLiteral(Literal *L) { (void)L; }
  void visitMemberExpr(MemberExpr *M) { (void)M; }
  void visitMethodCallExpr(MethodCallExpr *MC) { (void)MC; }
  void visitUnaryExpr(UnaryExpr *U) { (void)U; }

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
  // TODO: register all predefined classes and functions with IdResolver
  (void)ObjCD; (void)IntCD; (void)StrCD; (void)BoolCD; (void)NoneCD;
  (void)PrintFD; (void)InputFD; (void)LenFD;
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
  // TODO: add all predefined classes and functions to GlobalScope
  (void)ObjCD; (void)IntCD; (void)StrCD; (void)BoolCD; (void)NoneCD;
  (void)PrintFD; (void)InputFD; (void)LenFD;
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
  // TODO: if D is not already tracked in CurScope, add it to CurScope
  //       and to IdResolver.
  (void)D;
}

void Sema::actOnPopScope(Scope *S) {
  // TODO: remove every declaration belonging to scope S from IdResolver
  (void)S;
}

//===----------------------------------------------------------------------===//
// Sema: функции проверок
//===----------------------------------------------------------------------===//

bool Sema::checkDuplication(Declaration *D) {
  // TODO: look up D->getSymbolInfo() in CurScope
  //       emit err_dup_decl then return false if found
  (void)D;
  return true;
}

bool Sema::checkClassShadow(Declaration *D) {
  // TODO: search IdResolver for a ClassDef with the same name
  //       emit err_bad_shadow then return false if found
  (void)D;
  return true;
}

bool Sema::checkNonlocalDecl(NonLocalDecl *NLD) {
  // TODO: walk IdResolver to find a VarDef in an enclosing function scope
  //       (but NOT the global scope)
  //       emit err_not_nonlocal if not found
  (void)NLD;
  return true;
}

bool Sema::checkGlobalDecl(GlobalDecl *GD) {
  // TODO: verify a top-level VarDef with the same name exists
  //       emit err_not_global if not found
  (void)GD;
  return true;
}

bool Sema::checkSuperClass(ClassDef *D) {
  // TODO: resolve the super-class name to a ClassDef
  //       emit err_supclass_not_def, err_supclass_isnot_class,
  //       or err_supclass_is_special_class as needed
  (void)D;
  return true;
}

bool Sema::checkFirstMethodParam(ClassDef *CD, FuncDef *FD) {
  // TODO: first param of FD must be typed as CD
  //       emit err_first_method_param on failure
  (void)CD; (void)FD;
  return true;
}

bool Sema::checkClassAttrs(ClassDef *D) {
  // TODO: check first method param, no attr shadowing (err_redefine_attr),
  //       and method override compatibility (err_method_override)
  (void)D;
  return true;
}

bool Sema::checkMethodOverride(FuncDef *OM, FuncDef *M) {
  // TODO: same param count, same non-self param types, compatible return type
  (void)OM; (void)M;
  return true;
}

bool Sema::checkClassDef(ClassDef *D) {
  // TODO: call checkSuperClass(D) and checkClassAttrs(D)
  (void)D;
  return true;
}

bool Sema::checkAssignTarget(Expr *E) {
  // TODO: if E is a DeclRef, check the declaration is in current scope
  //       emit err_bad_local_assign if not
  (void)E;
  return true;
}

bool Sema::checkReturnStmt(ReturnStmt *S) {
  // TODO: return must not be at top level
  //       emit err_bad_return_top if CurScope is global
  (void)S;
  return true;
}

bool Sema::checkReturnMissing(FuncDef *F) {
  // TODO: build CFG, check that non-void functions have return on all paths
  //       emit err_maybe_falloff_nonvoid if a path falls through
  (void)F;
  return true;
}

bool Sema::checkTypeAnnotation(TypeAnnotation *TA) {
  // TODO: ClassType must resolve to a defined class (err_invalid_type_annotation)
  //       ListType: recursively check element type
  (void)TA;
  return true;
}

//===----------------------------------------------------------------------===//
//  Sema: реализация действий (actOn)
//===----------------------------------------------------------------------===//

void Sema::actOnDeclaration(Declaration *D) {
  // TODO: dispatch based on declaration kind:
  //   checkDuplication, checkClassShadow, then addDeclaration
  //   For NonLocalDecl: checkNonlocalDecl
  //   For GlobalDecl: checkGlobalDecl
  (void)D;
}

void Sema::actOnClassDef(ClassDef *C) {
  // TODO: call checkClassDef(C)
  (void)C;
}

void Sema::actOnFuncDef(FuncDef *F) {
  // TODO: call checkReturnMissing(F)
  (void)F;
}

void Sema::actOnParamDecl(ParamDecl *P) {
  // TODO: check type annotation, then addDeclaration
  (void)P;
}

void Sema::actOnVarDef(VarDef *V) {
  // TODO: check type annotation, verify initializer type compatibility
  //       (err_tc_assign), then addDeclaration
  (void)V;
}

void Sema::actOnNonlocalDecl(NonLocalDecl *D) {
  // TODO: after validation, add to current scope
  (void)D;
}

void Sema::actOnGlobalDecl(GlobalDecl *D) {
  // TODO: after validation, add to current scope
  (void)D;
}

void Sema::actOnAssignStmt(AssignStmt *A) {
  // TODO: for each target check assignment compatibility
  //       checkAssignTarget for DeclRef targets
  //       emit err_tc_assign on type mismatch
  (void)A;
}

void Sema::actOnReturnStmt(ReturnStmt *R) {
  // TODO: checkReturnStmt, then type-check return value against
  //       CurFuncDef's return type
  (void)R;
}

void Sema::actOnBinaryExpr(BinaryExpr *B) {
  // TODO: check operand types for each operator kind
  //       set inferred type, emit err_tc_binary on error
  //   ValueType *LTy = cast<ValueType>(B->getLeft()->getInferredType());
  //   ValueType *RTy = cast<ValueType>(B->getRight()->getInferredType());
  //   switch (B->getOpKind()) {
  //     case BinaryExpr::OpKind::Add:
  //     case BinaryExpr::OpKind::Sub:
  //     // ...
  //   }
  (void)B;
}

void Sema::actOnIndexExpr(IndexExpr *I) {
  // TODO: base must be list or str, index must be int
  //       emit err_cannot_index or err_bad_index
  (void)I;
}

void Sema::actOnListExpr(ListExpr *L) {
  // TODO: empty list -> empty class type
  //       otherwise join all element types
  (void)L;
}

void Sema::actOnLiteral(Literal *L) {
  // TODO: TypeSwitch: BooleanLiteral->BoolTy, IntegerLiteral->IntTy,
  //       NoneLiteral->NoneTy, StringLiteral->StrTy
  (void)L;
}

void Sema::actOnCallExpr(CallExpr *CE) {
  // TODO: resolve callee, check arg count and types
  //       Handle both FuncDef call and ClassDef constructor
  //       emit err_not_func, err_args_count, err_tc_call
  (void)CE;
}

void Sema::actOnDeclRef(DeclRef *DR) {
  // TODO: lookup in IdResolver, convert to ValueType
  //       emit err_not_variable if resolves to FuncDef
  (void)DR;
}

void Sema::actOnIfExpr(IfExpr *I) {
  // TODO: infer result type as join of then/else branch types
  (void)I;
}

void Sema::actOnMemberExpr(MemberExpr *M) {
  // TODO: object must be ClassValueType, lookupMember
  //       emit err_cannot_access_member or err_no_attribute
  (void)M;
}

void Sema::actOnMethodCallExpr(MethodCallExpr *M) {
  // TODO: lookupMethod, check arg count/types
  //       emit err_no_method, err_args_count, err_tc_call
  (void)M;
}

void Sema::actOnUnaryExpr(UnaryExpr *U) {
  // TODO: '-' requires int, 'not' requires bool
  //       emit err_tc_unary
  (void)U;
}

//===----------------------------------------------------------------------===//
// Sema: вспомогательные функции поиска (lookup helpers)
//===----------------------------------------------------------------------===//

Scope *Sema::getScopeForDecl(Scope *S, Declaration *D) {
  // TODO: walk up from S through parent chain until scope containing D found
  (void)S; (void)D;
  return nullptr;
}

ClassDef *Sema::getSuperClass(ClassDef *CD) {
  // TODO: resolve the super-class identifier to a ClassDef
  (void)CD;
  return nullptr;
}

bool Sema::isSameType(TypeAnnotation *TyA, TypeAnnotation *TyB) {
  // TODO: structural equality: ClassType by name, ListType recursively
  (void)TyA; (void)TyB;
  return false;
}

ValueType *Sema::convertATypeToVType(TypeAnnotation *TA) {
  // TODO: ClassType -> ClassValueType, ListType -> ListValueType recursively
  (void)TA;
  return nullptr;
}

FuncType *Sema::getFuncType(FuncDef *FD) {
  // TODO: build a FuncType from FD's params and return type annotation
  (void)FD;
  return nullptr;
}

Declaration *Sema::lookupName(Scope *S, SymbolInfo *SI) {
  // TODO: find declaration with given SymbolInfo in scope S
  (void)S; (void)SI;
  return nullptr;
}

Declaration *Sema::lookupDecl(DeclRef *DR) {
  SymbolInfo *SI = DR->getSymbolInfo();
  return lookupDecl(SI);
}

Declaration *Sema::lookupDecl(SymbolInfo *SI) {
  return *IdResolver.begin(SI);
}

VarDef *Sema::lookupMember(ClassDef *CD, DeclRef *Member) {
  // TODO: walk class hierarchy looking for VarDef matching Member name
  (void)CD; (void)Member;
  return nullptr;
}

VarDef *Sema::lookupMember(ClassValueType *CVT, DeclRef *Member) {
  return lookupMember(CVT->getClassDef(), Member);
}

FuncDef *Sema::lookupMethod(ClassDef *CD, DeclRef *Member) {
  return lookupMethod(CD, Member->getName());
}

FuncDef *Sema::lookupMethod(ClassDef *CD, StringRef Name) {
  // TODO: walk class hierarchy looking for FuncDef matching Name
  (void)CD; (void)Name;
  return nullptr;
}

FuncDef *Sema::lookupMethod(ClassValueType *CVT, DeclRef *Member) {
  return lookupMethod(CVT->getClassDef(), Member);
}

ClassDef *Sema::resolveClassType(ClassType *CT) {
  // TODO: look up class name in IdentifierResolver
  (void)CT;
  return nullptr;
}

void Sema::getClassHierarchy(const ClassValueType *CVT,
                             SmallVector<ClassDef *> &Hierarchy) {
  // TODO: build inheritance chain from CVT to object
  (void)CVT; (void)Hierarchy;
}

ValueType *Sema::join(ValueType *LTy, ValueType *RTy) {
  // TODO: compute least upper bound type in class hierarchy
  (void)LTy; (void)RTy;
  return nullptr;
}

bool Sema::isAssignementCompatibility(const ValueType *Sub,
                                      const ValueType *Super) {
  // TODO:
  (void)Sub; (void)Super;
  return false;
}

} // namespace chocopy
