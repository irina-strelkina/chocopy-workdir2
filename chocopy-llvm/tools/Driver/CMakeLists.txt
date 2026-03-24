set(LLVM_LINK_COMPONENTS
  Core
  Support)

add_llvm_executable(chocopy-llvm main.cpp)

target_link_libraries(chocopy-llvm PRIVATE
    chocopy-llvm-AST
    chocopy-llvm-analysis
    chocopy-llvm-codegen
    chocopy-llvm-sema
    chocopy-llvm-parser
    chocopy-llvm-lexer
)
