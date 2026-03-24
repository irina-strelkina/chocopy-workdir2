#include "chocopy-llvm/Sema/IdentifierResolver.h"
#include "chocopy-llvm/AST/AST.h"

namespace chocopy {
class IdentifierResolver::IdDeclInfoMap {
public:
  IdDeclInfoMap() = default;
  IdDeclInfoMap(const IdDeclInfoMap &) = delete;
  IdDeclInfoMap(IdDeclInfoMap &&) = delete;

  IdDeclInfoMap &operator=(const IdDeclInfoMap &) = delete;
  IdDeclInfoMap &operator=(IdDeclInfoMap &&) = delete;

  ~IdDeclInfoMap() {
    while (IdDeclInfoPool *P = CurPool) {
      CurPool = CurPool->Next;
      delete P;
    }
  }

  /// Returns the IdDeclInfo associated to the DeclarationName.
  /// It creates a new IdDeclInfo if one was not created before for this id.
  IdDeclInfo &operator[](SymbolInfo *SI);

private:
  static constexpr const unsigned int POOL_SIZE = 512;

  struct IdDeclInfoPool {
    IdDeclInfoPool(IdDeclInfoPool *Next) : Next(Next) {}

    IdDeclInfoPool *Next;
    IdDeclInfo Pool[POOL_SIZE];
  };

private:
  IdDeclInfoPool *CurPool = nullptr;
  unsigned int CurIdx = POOL_SIZE;
};

IdentifierResolver::IdentifierResolver()
    : IdDeclInfos(std::make_unique<IdDeclInfoMap>()) {}

IdentifierResolver::~IdentifierResolver() = default;

void IdentifierResolver::addDecl(Declaration *D) {
  SymbolInfo *SI = D->getSymbolInfo();
  void *Ptr = SI->getFETokenInfo();
  if (!Ptr) {
    SI->setFETokenInfo(D);
    return;
  }

  IdDeclInfo *IDI = nullptr;
  if (isDeclarationPtr(Ptr)) {
    SI->setFETokenInfo(nullptr);
    IDI = &(*IdDeclInfos)[SI];
    Declaration *PrevD = static_cast<Declaration *>(Ptr);
    IDI->addDecl(PrevD);
  } else
    IDI = toIdDeclInfo(Ptr);
  IDI->addDecl(D);
}

void IdentifierResolver::removeDecl(Declaration *D) {
  SymbolInfo *SI = D->getSymbolInfo();
  void *Ptr = SI->getFETokenInfo();

  if (isDeclarationPtr(Ptr)) {
    SI->setFETokenInfo(nullptr);
    return;
  }
  toIdDeclInfo(Ptr)->removeDecl(D);
}

auto IdentifierResolver::IdDeclInfoMap::operator[](SymbolInfo *SI)
    -> IdDeclInfo & {
  void *Ptr = SI->getFETokenInfo();

  if (Ptr)
    return *toIdDeclInfo(Ptr);

  if (CurIdx == POOL_SIZE) {
    CurPool = new IdDeclInfoPool(CurPool);
    CurIdx = 0;
  }
  IdDeclInfo *IDI = &CurPool->Pool[CurIdx];
  SI->setFETokenInfo(
      reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(IDI) | 0x1));
  ++CurIdx;
  return *IDI;
}

void IdentifierResolver::IdDeclInfo::removeDecl(Declaration *D) {
  for (DeclsTy::iterator I = Decls.end(); I != Decls.begin(); --I) {
    if (D == *(I - 1)) {
      Decls.erase(I - 1);
      return;
    }
  }
}

void IdentifierResolver::IdDeclInfo::dump(ASTContext &Ctx) {
  for (Declaration *D : Decls) {
    D->dump(Ctx);
  }
}

auto IdentifierResolver::begin(SymbolInfo *Name) -> iterator {
  void *Ptr = Name->getFETokenInfo();
  if (!Ptr)
    return end();

  if (isDeclarationPtr(Ptr))
    return iterator(static_cast<Declaration *>(Ptr));

  IdDeclInfo *IDI = toIdDeclInfo(Ptr);
  IdDeclInfo::DeclsTy::iterator I = IDI->decls_end();
  if (I != IDI->decls_begin()) {
    return iterator(I - 1);
  }
  return end();
}

void IdentifierResolver::iterator::incrementSlowCase() {
  Declaration *D = **this;
  void *InfoPtr = D->getSymbolInfo()->getFETokenInfo();
  assert(!isDeclarationPtr(InfoPtr) && "Decl with wrong id ?");
  IdDeclInfo *Info = toIdDeclInfo(InfoPtr);

  BaseIter I = getIterator();
  if (I != Info->decls_begin())
    *this = iterator(I - 1);
  else // No more decls.
    *this = iterator();
}
} // namespace chocopy
