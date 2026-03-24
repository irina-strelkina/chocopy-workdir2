set(LLVM_LINK_COMPONENTS
  Support)

llvm_add_library(chocopy-llvm-sema
    IdentifierResolver.cpp
    Sema.cpp
    Scope.cpp
  LINK_LIBS
    chocopy-llvm-basic
    chocopy-llvm-AST
    chocopy-llvm-analysis)
