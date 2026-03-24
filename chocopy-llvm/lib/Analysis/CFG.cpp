#include "chocopy-llvm/Analysis/CFG.h"
#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/ExprVisitor.h"
#include "chocopy-llvm/AST/StmtVisitor.h"

namespace chocopy {
namespace {
class CFGBuilder;

class CFGBuilder : public StmtVisitor<CFGBuilder, CFGBlock *>,
                   public ExprVisitor<CFGBuilder, CFGBlock *> {
  using StmtVisitorBase = StmtVisitor<CFGBuilder, CFGBlock *>;
  using ExprVisitorBase = ExprVisitor<CFGBuilder, CFGBlock *>;

public:
  std::unique_ptr<CFG> buildCFG(FuncDef *F);

  void autoCreateBlock() {
    if (!Block)
      Block = createBlock();
  }

  CFGBlock *createBlock(bool AddSucc = true) {
    CFGBlock *B = Cfg->createBlock();
    if (AddSucc && Succ)
      B->addSuccessor(Succ);
    return B;
  }

  CFGBlock *visit(Stmt *S) { return StmtVisitorBase::visit(S); }

  CFGBlock *visitAssignStmt(AssignStmt *S);
  CFGBlock *visitExprStmt(ExprStmt *S);
  CFGBlock *visitForStmt(ForStmt *S);
  CFGBlock *visitIfStmt(IfStmt *S);
  CFGBlock *visitReturnStmt(ReturnStmt *S);
  CFGBlock *visitWhileStmt(WhileStmt *S);

  CFGBlock *visit(Expr *E) { return ExprVisitorBase::visit(E); }

  CFGBlock *visitBinaryExpr(BinaryExpr *E);
  CFGBlock *visitCallExpr(CallExpr *E);
  CFGBlock *visitDeclRef(DeclRef *E);
  CFGBlock *visitIfExpr(IfExpr *E);
  CFGBlock *visitIndexExpr(IndexExpr *E);
  CFGBlock *visitListExpr(ListExpr *E);

  CFGBlock *visitLiteral(Literal *E);
  CFGBlock *visitMemberExpr(MemberExpr *E);
  CFGBlock *visitMethodCallExpr(MethodCallExpr *E);
  CFGBlock *visitUnaryExpr(UnaryExpr *E);

  CFGBlock *visitLogicalExpr(BinaryExpr *B);
  CFGBlock *visitLogicalExpr(BinaryExpr *B, BinaryExpr *Term,
                             CFGBlock *TrueBlock, CFGBlock *FalseBlock);

private:
  std::unique_ptr<CFG> Cfg = std::make_unique<CFG>();
  CFGBlock *Block = nullptr;
  CFGBlock *Succ = nullptr;
};
} // namespace

std::unique_ptr<CFG> CFG::buildCFG(FuncDef *F) {
  CFGBuilder Builder;
  return Builder.buildCFG(F);
}

CFGBlock *CFG::createBlock() {
  bool isFirst = begin() == end();
  CFGBlock *B = new (Allocator) CFGBlock(NumBlockIds++);
  Blocks.push_back(B);

  if (isFirst)
    Entry = Exit = &back();

  return &back();
}

std::unique_ptr<CFG> CFGBuilder::buildCFG(FuncDef *F) {
  Succ = createBlock();
  CFGBlock *B = Succ;
  for (Stmt *S : llvm::reverse(F->getStatements())) {
    B = Succ = visit(S);
    if (isa<WhileStmt>(S))
      Block = nullptr;
  }

  Succ = B;
  Cfg->setEntry(createBlock());
  return std::move(Cfg);
}

CFGBlock *CFGBuilder::visitAssignStmt(AssignStmt *S) {
  autoCreateBlock();
  CFGBlock *B = Block;
  B->appendStmt(S);
  for (Expr *T : S->getTargets())
    visit(T);
  return visit(S->getValue());
}

CFGBlock *CFGBuilder::visitExprStmt(ExprStmt *S) {
  autoCreateBlock();
  return visit(S->getExpr());
}

CFGBlock *CFGBuilder::visitForStmt(ForStmt *F) {
  CFGBlock *Exit = Block ? Block : &Cfg->getExit();

  Block = createBlock(false);
  CFGBlock *Latch = Block;
  for (Stmt *S : llvm::reverse(F->getBody()))
    visit(S);
  CFGBlock *Body = Block;

  Block = createBlock(false);
  CFGBlock *Header = Block;

  Header->addSuccessor(Body);
  Header->addSuccessor(Exit);
  Latch->addSuccessor(Header);

  Block->setTerminator(F);
  visit(F->getIterable());

  Succ = Block;
  Block = createBlock();
  return visit(F->getTarget());
}

CFGBlock *CFGBuilder::visitIfStmt(IfStmt *I) {
  Succ = Block ? Block : &Cfg->getExit();

  CFGBlock *ElseBlock = Succ;
  if (!I->getElseBody().empty()) {
    Block = createBlock();
    for (Stmt *S : llvm::reverse(I->getElseBody()))
      Block = visit(S);
    Succ = ElseBlock;
    ElseBlock = Block;
  }

  Block = createBlock();
  if (!I->getThenBody().empty()) {
    for (Stmt *S : llvm::reverse(I->getThenBody()))
      Block = visit(S);
  }
  CFGBlock *ThenBlock = Block;

  Block = createBlock(false);
  Block->addSuccessor(ThenBlock);
  Block->addSuccessor(ElseBlock);
  Block->setTerminator(I);
  return visit(I->getCondition());
}

CFGBlock *CFGBuilder::visitReturnStmt(ReturnStmt *S) {
  Block = createBlock(false);
  Block->appendStmt(S);
  Block->addSuccessor(&Cfg->getExit());
  if (Expr *E = S->getValue())
    visit(E);
  return Block;
}

CFGBlock *CFGBuilder::visitWhileStmt(WhileStmt *S) {
  CFGBlock *Exit = Block ? Block : &Cfg->getExit();

  Block = createBlock(false);
  CFGBlock *Latch = Block;
  for (Stmt *S : llvm::reverse(S->getBody()))
    Block = visit(S);

  CFGBlock *Body = Block;
  CFGBlock *Header = createBlock(false);
  Header->setTerminator(S);

  Latch->addSuccessor(Header);
  Header->addSuccessor(Body);
  Header->addSuccessor(Exit);

  Block = Header;
  visit(S->getCondition());
  Succ = Block;
  return Succ;
}

static bool isLogicalOp(BinaryExpr::OpKind K) {
  return K == BinaryExpr::OpKind::And || K == BinaryExpr::OpKind::Or;
}

CFGBlock *CFGBuilder::visitLogicalExpr(BinaryExpr *B) {
  autoCreateBlock();
  CFGBlock *ConfluenceB = Block;
  ConfluenceB->appendExpr(B);
  return visitLogicalExpr(B, nullptr, ConfluenceB, ConfluenceB);
}

CFGBlock *CFGBuilder::visitLogicalExpr(BinaryExpr *B, BinaryExpr *Term,
                                       CFGBlock *TrueBlock,
                                       CFGBlock *FalseBlock) {
  CFGBlock *RHSBlock = nullptr;
  Expr *R = B->getRight();
  do {
    if (BinaryExpr *BR = dyn_cast<BinaryExpr>(R)) {
      if (isLogicalOp(BR->getOpKind())) {
        RHSBlock = visitLogicalExpr(BR, Term, TrueBlock, FalseBlock);
        break;
      }
    }

    RHSBlock = createBlock(false);

    if (!Term) {
      assert(TrueBlock == FalseBlock);
      RHSBlock->addSuccessor(TrueBlock);
    } else {
      RHSBlock->setTerminator(Term);
      RHSBlock->addSuccessor(TrueBlock);
      RHSBlock->addSuccessor(FalseBlock);
    }

    Block = RHSBlock;
    visit(R);
  } while (false);

  Expr *L = B->getLeft();
  if (BinaryExpr *BL = dyn_cast<BinaryExpr>(L)) {
    if (isLogicalOp(BL->getOpKind())) {
      if (BL->getOpKind() == BinaryExpr::OpKind::And)
        TrueBlock = RHSBlock;
      else
        FalseBlock = RHSBlock;
      return visitLogicalExpr(BL, B, TrueBlock, FalseBlock);
    }
  }

  CFGBlock *LHSBlock = createBlock(false);
  LHSBlock->setTerminator(B);
  Block = LHSBlock;
  visit(L);

  if (B->getOpKind() == BinaryExpr::OpKind::And) {
    LHSBlock->addSuccessor(RHSBlock);
    LHSBlock->addSuccessor(FalseBlock);
  } else {
    LHSBlock->addSuccessor(TrueBlock);
    LHSBlock->addSuccessor(RHSBlock);
  }

  return Block;
}

CFGBlock *CFGBuilder::visitBinaryExpr(BinaryExpr *B) {
  autoCreateBlock();

  if (isLogicalOp(B->getOpKind())) {
    return visitLogicalExpr(B);
  }

  Block->appendExpr(B);
  visit(B->getRight());
  visit(B->getLeft());
  return Block;
}

CFGBlock *CFGBuilder::visitCallExpr(CallExpr *C) {
  autoCreateBlock();
  Block->appendExpr(C);
  visit(C->getFunction());
  for (Expr *E : C->getArgs())
    visit(E);
  return Block;
}

CFGBlock *CFGBuilder::visitDeclRef(DeclRef *E) {
  autoCreateBlock();
  Block->appendExpr(E);
  return Block;
}

CFGBlock *CFGBuilder::visitIfExpr(IfExpr *E) {
  autoCreateBlock();
  Block->appendExpr(E);

  CFGBlock *B = Succ = Block;
  Block = createBlock();
  visit(E->getElseExpr());
  CFGBlock *Else = Block;
  Block = Succ = B;
  Block = createBlock();
  visit(E->getThenExpr());

  Succ = Block;
  Block = createBlock();
  Block->addSuccessor(Else);
  Block->setTerminator(E);
  visit(E->getCondExpr());
  return Block;
}

CFGBlock *CFGBuilder::visitIndexExpr(IndexExpr *I) {
  autoCreateBlock();
  Block->appendExpr(I);
  visit(I->getIndex());
  visit(I->getList());
  return Block;
}

CFGBlock *CFGBuilder::visitListExpr(ListExpr *L) {
  autoCreateBlock();
  Block->appendExpr(L);
  for (Expr *E : llvm::reverse(L->getElements()))
    visit(E);
  return Block;
}

CFGBlock *CFGBuilder::visitLiteral(Literal *L) {
  autoCreateBlock();
  Block->appendExpr(L);
  return Block;
}

CFGBlock *CFGBuilder::visitMemberExpr(MemberExpr *E) {
  autoCreateBlock();
  Block->appendExpr(E);
  visit(E->getObject());
  return Block;
}

CFGBlock *CFGBuilder::visitMethodCallExpr(MethodCallExpr *M) {
  autoCreateBlock();
  Block->appendExpr(M);
  visit(M->getMethod());
  for (Expr *E : M->getArgs())
    visit(E);
  return Block;
}

CFGBlock *CFGBuilder::visitUnaryExpr(UnaryExpr *U) {
  autoCreateBlock();
  Block->appendExpr(U);
  visit(U->getOperand());
  return Block;
}
} // namespace chocopy