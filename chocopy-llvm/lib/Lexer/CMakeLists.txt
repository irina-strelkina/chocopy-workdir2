set(LLVM_LINK_COMPONENTS
  Support)

llvm_add_library(chocopy-llvm-lexer
    Lexer.cpp
    CachingLexer.cpp
    TokenKinds.cpp
  LINK_LIBS
    chocopy-llvm-basic
)
