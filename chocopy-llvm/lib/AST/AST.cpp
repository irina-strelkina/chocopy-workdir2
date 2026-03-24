#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/AST/ASTContext.h"
#include "chocopy-llvm/AST/JSONASTDumper.h"

namespace chocopy {
void Program::dump(ASTContext &C) const {
  JSONDumper Dumper(C);
  Dumper.visit(this);
}

StringRef BinaryExpr::getOpKindStr(OpKind K) {
  switch (K) {
  case BinaryExpr::OpKind::And:
    return "and";
  case BinaryExpr::OpKind::Or:
    return "or";
  case BinaryExpr::OpKind::Add:
    return "+";
  case BinaryExpr::OpKind::Sub:
    return "-";
  case BinaryExpr::OpKind::Mul:
    return "*";
  case BinaryExpr::OpKind::FloorDiv:
    return "//";
  case BinaryExpr::OpKind::Mod:
    return "%";
  case BinaryExpr::OpKind::EqCmp:
    return "==";
  case BinaryExpr::OpKind::NEqCmp:
    return "!=";
  case BinaryExpr::OpKind::LEqCmp:
    return "<=";
  case BinaryExpr::OpKind::GEqCmp:
    return ">=";
  case BinaryExpr::OpKind::LCmp:
    return "<";
  case BinaryExpr::OpKind::GCmp:
    return ">";
  case BinaryExpr::OpKind::Is:
    return "is";
  }

  llvm_unreachable("Invalid binary expression OpKind!");
  return "";
}
} // namespace chocopy
