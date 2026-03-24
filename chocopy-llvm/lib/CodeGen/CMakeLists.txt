set(LLVM_LINK_COMPONENTS
  Core
  Support)

llvm_add_library(chocopy-llvm-codegen
  ModuleBuilder.cpp
  CodeGenModule.cpp
  CodeGenFunction.cpp

  LINK_LIBS
    chocopy-llvm-AST
)
