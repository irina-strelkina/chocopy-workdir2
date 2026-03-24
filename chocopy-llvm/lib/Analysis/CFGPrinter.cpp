#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/ExprVisitor.h"
#include "chocopy-llvm/AST/RecursiveASTVisitor.h"
#include "chocopy-llvm/AST/StmtVisitor.h"
#include "chocopy-llvm/Analysis/CFG.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>

namespace chocopy {
namespace {
class FuncVisitor : public RecursiveASTVisitor<FuncVisitor> {
  using Base = RecursiveASTVisitor<FuncVisitor>;

public:
  bool visitFuncDef(FuncDef *F) {
    std::unique_ptr<CFG> C = CFG::buildCFG(F);
    llvm::errs() << "def " << F->getName() << ":\n";
    C->dump();
    return true;
  }
};

class PrintHelper : public ConstStmtVisitor<PrintHelper>,
                    public ConstExprVisitor<PrintHelper> {
  using StmtVisitor = ConstStmtVisitor<PrintHelper>;
  using ExprVisitor = ConstExprVisitor<PrintHelper>;
  using StmtMapTy = llvm::DenseMap<const Stmt *, std::pair<unsigned, unsigned>>;
  using ExprMapTy = llvm::DenseMap<const Expr *, std::pair<unsigned, unsigned>>;

public:
  PrintHelper(const CFG *Cfg, raw_ostream &OS) : OS(OS) {
    for (CFGBlock *B : *Cfg) {
      unsigned j = 0;
      for (CFGElement El : *B) {
        if (El.isStmt()) {
          const Stmt *S = El.getStmt();
          StmtMap[S] = std::make_pair(B->getBlockId(), j);
        } else if (El.isExpr()) {
          const Expr *E = El.getExpr();
          ExprMap[E] = std::make_pair(B->getBlockId(), j);
        }
        j++;
      }
    }
  }

  void visit(const Expr *E) { ExprVisitor::visit(E); }

  void visitBinaryExpr(const BinaryExpr *B) {
    if (!handleExpr(B->getLeft()))
      visit(B->getLeft());

    OS << " " << B->getOpKindStr() << " ";

    if (!handleExpr(B->getRight()))
      visit(B->getRight());
  }

  void visitCallExpr(const CallExpr *C) {
    if (!handleExpr(C->getFunction()))
      visit(C->getFunction());

    OS << "(";
    for (const Expr *E : C->getArgs()) {
      if (!handleExpr(E))
        visit(E);

      if (E != C->getArgs().back())
        OS << ", ";
    }
    OS << ")";
  }

  void visitDeclRef(const DeclRef *D) {
    if (!handleExpr(D))
      OS << D->getName();
  }

  void visitIfExpr(const IfExpr *E) {
    OS << " ... if ";
    if (!handleExpr(E))
      visit(E->getCondExpr());

    OS << " else ...";
  }

  void visitIndexExpr(const IndexExpr *E) {
    if (!handleExpr(E->getList()))
      handleExpr(E->getList());

    OS << "[ ";
    if (!handleExpr(E->getIndex()))
      visit(E->getIndex());
    OS << " ]";
  }

  void visitListExpr(const ListExpr *L) {
    OS << "[";
    for (const Expr *E : L->getElements()) {
      if (!handleExpr(E))
        visit(E);

      if (E != L->getElements().back())
        OS << ", ";
    }
    OS << "]";
  }

  void visitBooleanLiteral(const BooleanLiteral *B) {
    if (!handleExpr(B))
      OS << (B->getValue() ? "True" : "False");
  }

  void visitIntegerLiteral(const IntegerLiteral *I) {
    if (!handleExpr(I))
      OS << I->getValue();
  }

  void visitNoneLiteral(const NoneLiteral *N) {
    if (!handleExpr(N))
      OS << "None";
  }

  void visitStringLiteral(const StringLiteral *S) {
    if (!handleExpr(S))
      OS << S->getValue();
  }

  void visitMemberExpr(const MemberExpr *E) {
    visit(E->getObject());
    OS << ".";
    visit(E->getMember());
  }

  void visitMethodCallExpr(const MethodCallExpr *M) {
    if (!handleExpr(M->getMethod()))
      visit(M->getMethod());

    OS << "( ";
    for (const Expr *E : M->getArgs()) {
      if (E != M->getArgs().back())
        OS << ", ";

      if (handleExpr(E))
        visit(E);
    }
    OS << " )";
  }

  void visitUnaryExpr(const UnaryExpr *E) {
    if (E->getOpKind() == UnaryExpr::OpKind::Minus)
      OS << "- ";
    else
      OS << "not ";

    if (!handleExpr(E->getOperand()))
      visit(E->getOperand());
  }

  void visitAssignStmt(const AssignStmt *A) {
    for (Expr *E : A->getTargets()) {
      if (!handleExpr(E))
        visit(E);
      OS << " ";
    }

    OS << "= ";

    if (!handleExpr(A->getValue()))
      visit(A->getValue());
  }

  void visit(const Stmt *S) { StmtVisitor::visit(S); }
  // void visitExprStmt(const ExprStmt *E);
  // void visitForStmt(const ForStmt *F);
  void visitIfStmt(const IfStmt *I) {
    OS << "if ";
    if (!handleExpr(I->getCondition()))
      visit(I->getCondition());
  }

  void visitReturnStmt(const ReturnStmt *R) {
    OS << "return ";
    if (const Expr *E = R->getValue())
      if (!handleExpr(E))
        visit(E);
  }

  void visitWhileStmt(const WhileStmt *W) {
    OS << "while (";
    if (!handleExpr(W->getCondition()))
      visit(W->getCondition());
    OS << ")";
  }

public:
  void setBBId(unsigned Id) { CurBB = Id; }

  void setExprId(unsigned Id) { CurExpr = Id; }

private:
  bool handleExpr(const Expr *E) {
    ExprMapTy::iterator It = ExprMap.find(E);
    if (It == ExprMap.end())
      return false;

    if (CurBB >= 0 && It->second.first == CurBB && It->second.second == CurExpr)
      return false;

    OS << "[BB" << It->second.first << "." << It->second.second << "]";
    return true;
  }

private:
  raw_ostream &OS;
  StmtMapTy StmtMap;
  ExprMapTy ExprMap;
  unsigned CurBB;
  unsigned CurExpr;
};
} // namespace

static void printTerminator(raw_ostream &OS, CFGTerminator &T, CFGBlock *B,
                            CFG *Cfg, PrintHelper &Helper) {
  Helper.setBBId(-1);
  OS.changeColor(raw_ostream::GREEN);
  OS << "  " << llvm::format("%3c", 'T') << ": ";
  if (T.isExpr())
    Helper.visit(T.getExpr());
  else
    Helper.visit(T.getStmt());
  OS << "\n";
  OS.resetColor();
}

static void printElement(raw_ostream &OS, CFGElement &El, CFGBlock *B, CFG *Cfg,
                         PrintHelper &Helper) {
  switch (El.getKind()) {
  case CFGElement::Kind::Expr: {
    Helper.visit(El.getExpr());
    break;
  }
  case CFGElement::Kind::Stmt: {
    Helper.visit(El.getStmt());
    break;
  }
  }
  OS << "\n";
}

static void printBlock(raw_ostream &OS, CFGBlock *B, CFG *Cfg,
                       PrintHelper &Helper) {
  OS.changeColor(raw_ostream::YELLOW, true);

  OS << "[ BB" << B->getBlockId();
  if (B == &Cfg->getEntry())
    OS << " (ENTRY)";
  else if (B == &Cfg->getExit())
    OS << " (EXIT)";
  OS << "]\n";

  OS.resetColor();

  unsigned i = 0;
  for (CFGElement El : *B) {
    OS << "  " << llvm::format("%3d", i) << ": ";
    Helper.setExprId(i++);
    printElement(OS, El, B, Cfg, Helper);
  }

  if (CFGTerminator T = B->getTerminator(); T.isValid()) {
    printTerminator(OS, T, B, Cfg, Helper);
  }

  OS.changeColor(raw_ostream::BLUE);
  OS << "    Preds ";

  OS.resetColor();
  OS << " (" << B->pred_size() << "): ";

  OS.changeColor(raw_ostream::BLUE);
  for (CFGBlock *P : B->preds())
    OS << " BB" << P->getBlockId();

  OS.changeColor(raw_ostream::MAGENTA);
  OS << "\n    Succs ";

  OS.resetColor();
  OS << " (" << B->succ_size() << "): ";

  OS.changeColor(raw_ostream::MAGENTA);
  for (CFGBlock *S : B->succs())
    OS << " BB" << S->getBlockId();
  OS << "\n";
  OS.resetColor();
}

void CFG::dump() {
  PrintHelper Helper(this, llvm::errs());
  Helper.setBBId(Entry->getBlockId());
  printBlock(llvm::errs(), Entry, this, Helper);
  for (CFGBlock *B : *this) {
    if (B == &getEntry() || B == &getExit())
      continue;
    Helper.setBBId(B->getBlockId());
    printBlock(llvm::errs(), B, this, Helper);
  }
  Helper.setBBId(Exit->getBlockId());
  printBlock(llvm::errs(), Exit, this, Helper);
}

void dumpCFG(Program *P) {
  FuncVisitor V;
  V.traverseProgram(P);
}
} // namespace chocopy