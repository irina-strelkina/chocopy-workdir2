#ifndef CHOCOPY_LLVM_SEMA_IDENTIFIERRESOLVER_H
#define CHOCOPY_LLVM_SEMA_IDENTIFIERRESOLVER_H

#include "chocopy-llvm/Basic/LLVM.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>

#include <memory.h>

namespace chocopy {
class ASTContext;
class Declaration;
class SymbolInfo;

class IdentifierResolver {
  class IdDeclInfo {
  public:
    using DeclsTy = SmallVector<Declaration *, 2>;

    DeclsTy::iterator decls_begin() { return Decls.begin(); }

    DeclsTy::iterator decls_end() { return Decls.end(); }

    void addDecl(Declaration *D) { Decls.push_back(D); }
    void removeDecl(Declaration *D);

    size_t size() { return Decls.size(); }
    void dump(ASTContext &Ctx);

  private:
    DeclsTy Decls;
  };

public:
  class iterator {
  public:
    friend class IdentifierResolver;

    using value_type = Declaration *;
    using reference = Declaration *;
    using pointer = Declaration *;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;

    /// Ptr - There are 2 forms that 'Ptr' represents:
    /// 1) A single Declaration. (Ptr & 0x1 == 0)
    /// 2) A IdDeclInfo::DeclsTy::iterator that traverses only the decls of the
    ///    same declaration context. (Ptr & 0x1 == 0x1)
    uintptr_t Ptr = 0;
    using BaseIter = IdDeclInfo::DeclsTy::iterator;

    /// A single Declaration. (Ptr & 0x1 == 0)
    iterator(Declaration *D) {
      Ptr = reinterpret_cast<uintptr_t>(D);
      assert((Ptr & 0x1) == 0 && "Invalid Ptr!");
    }

    /// A IdDeclInfo::DeclsTy::iterator that walks or not the parent declaration
    /// contexts depending on 'LookInParentCtx'.
    iterator(BaseIter I) { Ptr = reinterpret_cast<uintptr_t>(I) | 0x1; }

    bool isIterator() const { return (Ptr & 0x1); }

    BaseIter getIterator() const {
      assert(isIterator() && "Ptr not an iterator!");
      return reinterpret_cast<BaseIter>(Ptr & ~0x1);
    }

    void incrementSlowCase();

  public:
    iterator() = default;

    Declaration *operator*() const {
      if (isIterator())
        return *getIterator();
      else
        return reinterpret_cast<Declaration *>(Ptr);
    }

    bool operator==(const iterator &RHS) const { return Ptr == RHS.Ptr; }
    bool operator!=(const iterator &RHS) const { return Ptr != RHS.Ptr; }

    // Preincrement.
    iterator &operator++() {
      if (!isIterator()) // common case.
        Ptr = 0;
      else
        incrementSlowCase();
      return *this;
    }
  };

public:
  IdentifierResolver();
  IdentifierResolver(IdentifierResolver &&) = delete;
  IdentifierResolver(const IdentifierResolver &) = delete;
  IdentifierResolver &operator=(const IdentifierResolver &) = delete;
  IdentifierResolver &operator=(IdentifierResolver &&) = delete;
  ~IdentifierResolver();

  void addDecl(Declaration *D);
  void removeDecl(Declaration *D);

  iterator begin(SymbolInfo *Name);
  iterator end() { return iterator(); }

private:
  /// FETokenInfo contains a Declaration pointer if lower bit == 0.
  static inline bool isDeclarationPtr(void *Ptr) {
    return (reinterpret_cast<uintptr_t>(Ptr) & 0x1) == 0;
  }

  static inline IdDeclInfo *toIdDeclInfo(void *Ptr) {
    assert((reinterpret_cast<uintptr_t>(Ptr) & 0x1) == 1 &&
           "Ptr not a IdDeclInfo* !");
    return reinterpret_cast<IdDeclInfo *>(reinterpret_cast<uintptr_t>(Ptr) &
                                          ~0x1);
  }

private:
  class IdDeclInfoMap;
  std::unique_ptr<IdDeclInfoMap> IdDeclInfos;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_SEMA_IDENTIFIERRESOLVER_H
