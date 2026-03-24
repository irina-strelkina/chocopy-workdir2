set(LLVM_LINK_COMPONENTS
  Support)

llvm_add_library(chocopy-llvm-AST
  AST.cpp
  ASTContext.cpp
  Declaration.cpp
  TextDiagnosticPrinter.cpp
  Type.cpp
  Stmt.cpp
  JSONASTDumper.cpp
)
