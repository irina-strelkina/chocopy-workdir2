#ifndef CHOCOPY_LLVM_BASIC_SYMBOLTABLE_H
#define CHOCOPY_LLVM_BASIC_SYMBOLTABLE_H

#include "chocopy-llvm/Basic/LLVM.h"
#include "chocopy-llvm/Lexer/TokenKinds.h"

#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>

namespace chocopy {
class Declaration;

class SymbolInfo {
  friend class SymbolTable;

public:
  tok::TokenKind getKind() const { return TokenID; }

  StringRef getName() const { return Entry->getKey(); }

  void *getFETokenInfo() const { return FETokenInfo; }
  void setFETokenInfo(void *T) { FETokenInfo = T; }

private:
  tok::TokenKind TokenID = tok::TokenKind::identifier;
  llvm::StringMapEntry<SymbolInfo *> *Entry = nullptr;
  void *FETokenInfo = nullptr;
};

class SymbolTable {
public:
  SymbolInfo &get(StringRef Name) {
    HashTableTy::iterator Item = HashTable.try_emplace(Name, nullptr).first;
    llvm::StringMapEntry<SymbolInfo *> &Entry = *Item;
    if (Entry.second)
      return *Entry.second;
    llvm::BumpPtrAllocator &Allocator = getAllocator();
    void *Mem = Allocator.Allocate<SymbolInfo>();
    SymbolInfo *SInfo = new (Mem) SymbolInfo();
    SInfo->Entry = &Entry;
    Entry.second = SInfo;
    return *SInfo;
  }

  SymbolInfo &get(StringRef Name, tok::TokenKind Kind) {
    SymbolInfo &SI = get(Name);
    SI.TokenID = Kind;
    return SI;
  }

private:
  llvm::BumpPtrAllocator &getAllocator() { return HashTable.getAllocator(); }

private:
  using HashTableTy = llvm::StringMap<SymbolInfo *, llvm::BumpPtrAllocator>;

  HashTableTy HashTable;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_BASIC_SYMBOLTABLE_H
