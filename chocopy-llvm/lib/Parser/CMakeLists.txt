set(LLVM_LINK_COMPONENTS
  Support)

llvm_add_library(chocopy-llvm-parser
    Parser.cpp
  LINK_LIBS
    chocopy-llvm-basic
    chocopy-llvm-lexer
    chocopy-llvm-AST
    chocopy-llvm-sema)
