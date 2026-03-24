#ifndef CHOCOPY_LLVM_SEMA_SCOPE_H
#define CHOCOPY_LLVM_SEMA_SCOPE_H

#include "chocopy-llvm/Basic/LLVM.h"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringRef.h>

#include <memory>

namespace chocopy {
class Declaration;

class Scope {
private:
  using DeclSetTy = llvm::SmallPtrSet<Declaration *, 32>;
  using decl_range = llvm::iterator_range<DeclSetTy::iterator>;

public:
  enum class ScopeKind {
    Global,
    Class,
    Func,
  };

public:
  Scope() : Kind(ScopeKind::Global) {}
  Scope(std::shared_ptr<Scope> P, ScopeKind Kind)
      : Kind(Kind), Parent(std::move(P)) {}

  ScopeKind getKind() const { return Kind; }

  bool isGlobal() const { return Kind == ScopeKind::Global; }

  bool isClass() const { return Kind == ScopeKind::Class; }

  bool isFunc() const { return Kind == ScopeKind::Func; }

  bool isMethod() const { return isFunc() && Parent && Parent->isClass(); }

  std::shared_ptr<Scope> getParent() const { return Parent; }

  void setParent(std::shared_ptr<Scope> P) { Parent = std::move(P); }

  bool isDeclInScope(const Declaration *D) const { return Decls.contains(D); }

  const decl_range getDecls() const {
    return llvm::make_range(Decls.begin(), Decls.end());
  }

  void addDecl(Declaration *D);

private:
  ScopeKind Kind;
  std::shared_ptr<Scope> Parent = nullptr;
  DeclSetTy Decls;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_SEMA_SCOPE_H
