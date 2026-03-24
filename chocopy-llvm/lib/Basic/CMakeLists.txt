set(LLVM_LINK_COMPONENTS
  Support)

llvm_add_library(chocopy-llvm-basic
  Diagnostic.cpp
  ASCIICharInfo.cpp)
