#ifndef CHOCOPY_LLVM_ANALYSIS_CFG_H
#define CHOCOPY_LLVM_ANALYSIS_CFG_H

#include "chocopy-llvm/Basic/LLVM.h"

#include <llvm/ADT/GraphTraits.h>
#include <llvm/ADT/PointerIntPair.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/iterator.h>
#include <llvm/Support/Allocator.h>

#include <memory>

namespace chocopy {

class FuncDef;
class Expr;
class Stmt;
class Program;
class CFG;
class CFGBlock;

class CFGElement {
public:
  enum class Kind : uint8_t { Stmt, Expr };

public:
  CFGElement(CFGBlock *Parent, Stmt *S) : Parent(Parent), Ptr(S) {
    Ptr.setInt(int(Kind::Stmt));
  }

  CFGElement(CFGBlock *Parent, Expr *E) : Parent(Parent), Ptr(E) {
    Ptr.setInt(int(Kind::Expr));
  }

public:
  CFGBlock *getParent() const { return Parent; }
  Kind getKind() const { return Kind(Ptr.getInt()); }
  Stmt *getStmt() const { return static_cast<Stmt *>(Ptr.getPointer()); }
  Expr *getExpr() const { return static_cast<Expr *>(Ptr.getPointer()); }

  bool isExpr() const { return getKind() == Kind::Expr; }
  bool isStmt() const { return getKind() == Kind::Stmt; }

private:
  CFGBlock *Parent = nullptr;
  llvm::PointerIntPair<void *, 2> Ptr;
};

class CFGTerminator {
public:
  CFGTerminator() = default;
  CFGTerminator(Expr *E) : Ptr(E, 0) {}
  CFGTerminator(Stmt *S) : Ptr(S, 1) {}

  bool isValid() const { return Ptr.getOpaqueValue() != nullptr; }
  bool isExpr() const { return Ptr.getInt() == 0; }
  bool isStmt() const { return Ptr.getInt() == 1; }

  Expr *getExpr() const {
    if (isExpr())
      return static_cast<Expr *>(Ptr.getPointer());
    return nullptr;
  }

  Stmt *getStmt() const {
    if (isStmt())
      return static_cast<Stmt *>(Ptr.getPointer());
    return nullptr;
  }

private:
  llvm::PointerIntPair<void *, 1> Ptr;
};

class CFGBlock {
private:
  using ElementList = SmallVector<CFGElement>;

  template <bool IsConst> class ElementRefImpl {
    template <bool IsOtherConst> friend class ElementRefImpl;

    using CFGBlockPtr =
        std::conditional_t<IsConst, const CFGBlock *, CFGBlock *>;

    using CFGElementPtr =
        std::conditional_t<IsConst, const CFGElement *, CFGElement *>;

  protected:
    CFGBlockPtr Parent;
    size_t Index;

  public:
    ElementRefImpl(CFGBlockPtr Parent, size_t Index)
        : Parent(Parent), Index(Index) {}

    template <bool IsOtherConst>
    ElementRefImpl(ElementRefImpl<IsOtherConst> Other)
        : ElementRefImpl(Other.Parent, Other.Index) {}

    size_t getIndexInBlock() const { return Index; }

    CFGBlockPtr getParent() { return Parent; }
    CFGBlockPtr getParent() const { return Parent; }

    bool operator<(ElementRefImpl Other) const {
      return std::make_pair(Parent, Index) <
             std::make_pair(Other.Parent, Other.Index);
    }

    bool operator==(ElementRefImpl Other) const {
      return Parent == Other.Parent && Index == Other.Index;
    }

    bool operator!=(ElementRefImpl Other) const { return !(*this == Other); }
    CFGElement operator*() const { return (*Parent)[Index]; }
    CFGElementPtr operator->() const { return &*(Parent->begin() + Index); }
  };

  template <bool IsReverse, bool IsConst> class ElementRefIterator {
    template <bool IsOtherReverse, bool IsOtherConst>
    friend class ElementRefIterator;

    using CFGBlockRef =
        std::conditional_t<IsConst, const CFGBlock *, CFGBlock *>;

    using UnderlayingIteratorTy = std::conditional_t<
        IsConst,
        std::conditional_t<IsReverse, ElementList::const_reverse_iterator,
                           ElementList::const_iterator>,
        std::conditional_t<IsReverse, ElementList::reverse_iterator,
                           ElementList::iterator>>;

    using IteratorTraits = typename std::iterator_traits<UnderlayingIteratorTy>;
    using ElementRef = typename CFGBlock::ElementRefImpl<IsConst>;

  public:
    using difference_type = typename IteratorTraits::difference_type;
    using value_type = ElementRef;
    using pointer = ElementRef *;
    using iterator_category = typename IteratorTraits::iterator_category;

  private:
    CFGBlockRef Parent;
    UnderlayingIteratorTy Pos;

  public:
    ElementRefIterator(CFGBlockRef Parent, UnderlayingIteratorTy Pos)
        : Parent(Parent), Pos(Pos) {}

    template <bool IsOtherConst>
    ElementRefIterator(ElementRefIterator<false, IsOtherConst> E)
        : ElementRefIterator(E.Parent, E.Pos.base()) {}

    template <bool IsOtherConst>
    ElementRefIterator(ElementRefIterator<true, IsOtherConst> E)
        : ElementRefIterator(E.Parent, std::make_reverse_iterator(E.Pos)) {}

    bool operator<(ElementRefIterator Other) const {
      assert(Parent == Other.Parent);
      return Pos < Other.Pos;
    }

    bool operator==(ElementRefIterator Other) const {
      return Parent == Other.Parent && Pos == Other.Pos;
    }

    bool operator!=(ElementRefIterator Other) const {
      return !(*this == Other);
    }

  private:
    template <bool IsOtherConst>
    static size_t
    getIndexInBlock(CFGBlock::ElementRefIterator<true, IsOtherConst> E) {
      return E.Parent->size() - (E.Pos - E.Parent->rbegin()) - 1;
    }

    template <bool IsOtherConst>
    static size_t
    getIndexInBlock(CFGBlock::ElementRefIterator<false, IsOtherConst> E) {
      return E.Pos - E.Parent->begin();
    }

  public:
    value_type operator*() { return {Parent, getIndexInBlock(*this)}; }

    difference_type operator-(ElementRefIterator Other) const {
      return Pos - Other.Pos;
    }

    ElementRefIterator operator++() {
      ++this->Pos;
      return *this;
    }
    ElementRefIterator operator++(int) {
      ElementRefIterator Ret = *this;
      ++*this;
      return Ret;
    }
    ElementRefIterator operator+(size_t count) {
      this->Pos += count;
      return *this;
    }
    ElementRefIterator operator-(size_t count) {
      this->Pos -= count;
      return *this;
    }
  };

public:
  CFGBlock(unsigned BlockId) : BlockId(BlockId) {}

public:
  // Statement iterators
  using iterator = ElementList::reverse_iterator;
  using const_iterator = ElementList::const_reverse_iterator;
  using reverse_iterator = ElementList::iterator;
  using const_reverse_iterator = ElementList::const_iterator;

  size_t getIndexInCFG() const;

  CFGElement front() const { return Elements.back(); }
  CFGElement back() const { return Elements.front(); }

  iterator begin() { return Elements.rbegin(); }
  iterator end() { return Elements.rend(); }
  const_iterator begin() const { return Elements.rbegin(); }
  const_iterator end() const { return Elements.rend(); }

  reverse_iterator rbegin() { return Elements.begin(); }
  reverse_iterator rend() { return Elements.end(); }
  const_reverse_iterator rbegin() const { return Elements.begin(); }
  const_reverse_iterator rend() const { return Elements.end(); }

  using CFGElementRef = ElementRefImpl<false>;
  using ConstCFGElementRef = ElementRefImpl<true>;

  using ref_iterator = ElementRefIterator<false, false>;
  using ref_iterator_range = llvm::iterator_range<ref_iterator>;
  using const_ref_iterator = ElementRefIterator<false, true>;
  using const_ref_iterator_range = llvm::iterator_range<const_ref_iterator>;

  using reverse_ref_iterator = ElementRefIterator<true, false>;
  using reverse_ref_iterator_range = llvm::iterator_range<reverse_ref_iterator>;

  using const_reverse_ref_iterator = ElementRefIterator<true, true>;
  using const_reverse_ref_iterator_range =
      llvm::iterator_range<const_reverse_ref_iterator>;

  ref_iterator ref_begin() { return {this, rbegin()}; }
  ref_iterator ref_end() { return {this, rend()}; }
  const_ref_iterator ref_begin() const { return {this, rbegin()}; }
  const_ref_iterator ref_end() const { return {this, rend()}; }

  reverse_ref_iterator rref_begin() { return {this, begin()}; }
  reverse_ref_iterator rref_end() { return {this, end()}; }
  const_reverse_ref_iterator rref_begin() const { return {this, begin()}; }
  const_reverse_ref_iterator rref_end() const { return {this, end()}; }

  ref_iterator_range refs() { return {ref_begin(), ref_end()}; }
  const_ref_iterator_range refs() const { return {ref_begin(), ref_end()}; }
  reverse_ref_iterator_range rrefs() { return {rref_begin(), rref_end()}; }
  const_reverse_ref_iterator_range rrefs() const {
    return {rref_begin(), rref_end()};
  }

  unsigned size() const { return Elements.size(); }
  bool empty() const { return Elements.empty(); }

  CFGElement operator[](size_t i) const { return Elements[i]; }

  // CFG iterators
  using AdjacentBlocks = SmallVector<CFGBlock *>;
  using pred_iterator = AdjacentBlocks::iterator;
  using const_pred_iterator = AdjacentBlocks::const_iterator;
  using pred_reverse_iterator = AdjacentBlocks::reverse_iterator;
  using const_pred_reverse_iterator = AdjacentBlocks::const_reverse_iterator;
  using pred_range = llvm::iterator_range<pred_iterator>;
  using pred_const_range = llvm::iterator_range<const_pred_iterator>;

  using succ_iterator = AdjacentBlocks::iterator;
  using const_succ_iterator = AdjacentBlocks::const_iterator;
  using succ_reverse_iterator = AdjacentBlocks::reverse_iterator;
  using const_succ_reverse_iterator = AdjacentBlocks::const_reverse_iterator;
  using succ_range = llvm::iterator_range<succ_iterator>;
  using succ_const_range = llvm::iterator_range<const_succ_iterator>;

  pred_iterator pred_begin() { return Preds.begin(); }
  pred_iterator pred_end() { return Preds.end(); }
  const_pred_iterator pred_begin() const { return Preds.begin(); }
  const_pred_iterator pred_end() const { return Preds.end(); }

  pred_reverse_iterator pred_rbegin() { return Preds.rbegin(); }
  pred_reverse_iterator pred_rend() { return Preds.rend(); }
  const_pred_reverse_iterator pred_rbegin() const { return Preds.rbegin(); }
  const_pred_reverse_iterator pred_rend() const { return Preds.rend(); }

  pred_range preds() { return pred_range(pred_begin(), pred_end()); }

  pred_const_range preds() const {
    return pred_const_range(pred_begin(), pred_end());
  }

  succ_iterator succ_begin() { return Succs.begin(); }
  succ_iterator succ_end() { return Succs.end(); }
  const_succ_iterator succ_begin() const { return Succs.begin(); }
  const_succ_iterator succ_end() const { return Succs.end(); }

  succ_reverse_iterator succ_rbegin() { return Succs.rbegin(); }
  succ_reverse_iterator succ_rend() { return Succs.rend(); }
  const_succ_reverse_iterator succ_rbegin() const { return Succs.rbegin(); }
  const_succ_reverse_iterator succ_rend() const { return Succs.rend(); }

  succ_range succs() { return succ_range(succ_begin(), succ_end()); }

  succ_const_range succs() const {
    return succ_const_range(succ_begin(), succ_end());
  }

  unsigned succ_size() const { return Succs.size(); }
  bool succ_empty() const { return Succs.empty(); }

  unsigned pred_size() const { return Preds.size(); }
  bool pred_empty() const { return Preds.empty(); }

  void addSuccessor(CFGBlock *B) {
    Succs.push_back(B);
    B->Preds.push_back(this);
  }

  void appendStmt(Stmt *S) { Elements.push_back(CFGElement(this, S)); }

  void appendExpr(Expr *E) { Elements.push_back(CFGElement(this, E)); }

  void setTerminator(CFGTerminator T) { Terminator = T; }
  CFGTerminator getTerminator() const { return Terminator; }

  CFG *getParent() const { return Parent; }

  unsigned getBlockId() const { return BlockId; }

private:
  CFG *Parent;
  ElementList Elements;
  CFGTerminator Terminator;
  unsigned BlockId;
  AdjacentBlocks Preds;
  AdjacentBlocks Succs;
};

class CFG {
public:
  static std::unique_ptr<CFG> buildCFG(FuncDef *FD);

public:
  CFGBlock *createBlock();

  using CFGBlockListTy = SmallVector<CFGBlock *>;
  using iterator = CFGBlockListTy::iterator;
  using const_iterator = CFGBlockListTy::const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  CFGBlock &front() { return *Blocks.front(); }
  CFGBlock &back() { return *Blocks.back(); }

  iterator begin() { return Blocks.begin(); }
  iterator end() { return Blocks.end(); }
  const_iterator begin() const { return Blocks.begin(); }
  const_iterator end() const { return Blocks.end(); }

  iterator nodes_begin() { return iterator(Blocks.begin()); }
  iterator nodes_end() { return iterator(Blocks.end()); }

  llvm::iterator_range<iterator> nodes() { return {begin(), end()}; }
  llvm::iterator_range<const_iterator> const_nodes() const {
    return {begin(), end()};
  }

  const_iterator nodes_begin() const { return const_iterator(Blocks.begin()); }
  const_iterator nodes_end() const { return const_iterator(Blocks.end()); }

  reverse_iterator rbegin() { return Blocks.rbegin(); }
  reverse_iterator rend() { return Blocks.rend(); }
  const_reverse_iterator rbegin() const { return Blocks.rbegin(); }
  const_reverse_iterator rend() const { return Blocks.rend(); }

  llvm::iterator_range<reverse_iterator> reverse_nodes() {
    return {rbegin(), rend()};
  }
  llvm::iterator_range<const_reverse_iterator> const_reverse_nodes() const {
    return {rbegin(), rend()};
  }

  CFGBlock &getEntry() { return *Entry; }
  const CFGBlock &getEntry() const { return *Entry; }
  CFGBlock &getExit() { return *Exit; }
  const CFGBlock &getExit() const { return *Exit; }

  void setEntry(CFGBlock *B) { Entry = B; }

  void dump();

public:
  unsigned size() const { return Blocks.size(); }

private:
  unsigned NumBlockIds = 0;
  CFGBlockListTy Blocks;
  CFGBlock *Entry;
  CFGBlock *Exit;
  llvm::BumpPtrAllocator Allocator;
};

void dumpCFG(Program *);

} // namespace chocopy

namespace llvm {
// Traits for: CFGBlock

template <> struct GraphTraits<::chocopy::CFGBlock *> {
  using NodeRef = ::chocopy::CFGBlock *;
  using ChildIteratorType = ::chocopy::CFGBlock::succ_iterator;

  static NodeRef getEntryNode(::chocopy::CFGBlock *BB) { return BB; }
  static ChildIteratorType child_begin(NodeRef N) { return N->succ_begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->succ_end(); }
};

template <> struct GraphTraits<const ::chocopy::CFGBlock *> {
  using NodeRef = const ::chocopy::CFGBlock *;
  using ChildIteratorType = ::chocopy::CFGBlock::const_succ_iterator;

  static NodeRef getEntryNode(const ::chocopy::CFGBlock *BB) { return BB; }
  static ChildIteratorType child_begin(NodeRef N) { return N->succ_begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->succ_end(); }
};

template <> struct GraphTraits<Inverse<::chocopy::CFGBlock *>> {
  using NodeRef = ::chocopy::CFGBlock *;
  using ChildIteratorType = ::chocopy::CFGBlock::const_pred_iterator;

  static NodeRef getEntryNode(Inverse<::chocopy::CFGBlock *> G) {
    return G.Graph;
  }

  static ChildIteratorType child_begin(NodeRef N) { return N->pred_begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->pred_end(); }
};

template <> struct GraphTraits<Inverse<const ::chocopy::CFGBlock *>> {
  using NodeRef = const ::chocopy::CFGBlock *;
  using ChildIteratorType = ::chocopy::CFGBlock::const_pred_iterator;

  static NodeRef getEntryNode(Inverse<const ::chocopy::CFGBlock *> G) {
    return G.Graph;
  }

  static ChildIteratorType child_begin(NodeRef N) { return N->pred_begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->pred_end(); }
};

// Traits for: CFG

template <>
struct GraphTraits<::chocopy::CFG *>
    : public GraphTraits<::chocopy::CFGBlock *> {
  using nodes_iterator = ::chocopy::CFG::iterator;

  static NodeRef getEntryNode(::chocopy::CFG *F) { return &F->getEntry(); }
  static nodes_iterator nodes_begin(::chocopy::CFG *F) {
    return F->nodes_begin();
  }
  static nodes_iterator nodes_end(::chocopy::CFG *F) { return F->nodes_end(); }
  static unsigned size(::chocopy::CFG *F) { return F->size(); }
};

template <>
struct GraphTraits<const ::chocopy::CFG *>
    : public GraphTraits<const ::chocopy::CFGBlock *> {
  using nodes_iterator = ::chocopy::CFG::const_iterator;

  static NodeRef getEntryNode(const ::chocopy::CFG *F) {
    return &F->getEntry();
  }

  static nodes_iterator nodes_begin(const ::chocopy::CFG *F) {
    return F->nodes_begin();
  }

  static nodes_iterator nodes_end(const ::chocopy::CFG *F) {
    return F->nodes_end();
  }

  static unsigned size(const ::chocopy::CFG *F) { return F->size(); }
};

template <>
struct GraphTraits<Inverse<::chocopy::CFG *>>
    : public GraphTraits<Inverse<::chocopy::CFGBlock *>> {
  using nodes_iterator = ::chocopy::CFG::iterator;

  static NodeRef getEntryNode(::chocopy::CFG *F) { return &F->getExit(); }
  static nodes_iterator nodes_begin(::chocopy::CFG *F) {
    return F->nodes_begin();
  }
  static nodes_iterator nodes_end(::chocopy::CFG *F) { return F->nodes_end(); }
};

template <>
struct GraphTraits<Inverse<const ::chocopy::CFG *>>
    : public GraphTraits<Inverse<const ::chocopy::CFGBlock *>> {
  using nodes_iterator = ::chocopy::CFG::const_iterator;

  static NodeRef getEntryNode(const ::chocopy::CFG *F) { return &F->getExit(); }

  static nodes_iterator nodes_begin(const ::chocopy::CFG *F) {
    return F->nodes_begin();
  }

  static nodes_iterator nodes_end(const ::chocopy::CFG *F) {
    return F->nodes_end();
  }
};

} // namespace llvm
#endif // CHOCOPY_LLVM_ANALYSIS_CFG_H
