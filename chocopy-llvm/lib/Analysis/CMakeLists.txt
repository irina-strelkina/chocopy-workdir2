set(LLVM_LINK_COMPONENTS
  Support)

llvm_add_library(chocopy-llvm-analysis
  CFG.cpp
  CFGPrinter.cpp
  LINK_LIBS
    chocopy-llvm-basic
    chocopy-llvm-AST)
