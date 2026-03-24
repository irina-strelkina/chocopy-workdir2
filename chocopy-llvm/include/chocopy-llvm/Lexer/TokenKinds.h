#ifndef CHOCOPY_LLVM_LEXER_TOKENKINDS_H
#define CHOCOPY_LLVM_LEXER_TOKENKINDS_H

namespace chocopy {
namespace tok {
enum TokenKind : unsigned short {
#define TOK(X) X,
#include "chocopy-llvm/Lexer/TokenKinds.def"
  NUM_TOKENS
};

const char *getTokenName(TokenKind Kind);
const char *getPunctuatorSpelling(TokenKind Kind);

[[maybe_unused]] static bool isLiteral(TokenKind Kind) {
  return Kind == tok::integer_literal || Kind == tok::idstring ||
         Kind == tok::string_literal;
}

[[maybe_unused]] static bool isBinOp(TokenKind Kind) {
  return Kind == tok::plus || Kind == tok::minus || Kind == tok::star ||
         Kind == tok::slashslash || Kind == tok::percent ||
         Kind == tok::equalequal || Kind == tok::exclaimequal ||
         Kind == tok::lessequal || Kind == tok::greaterequal ||
         Kind == tok::less || Kind == tok::greater || Kind == tok::kw_is;
}

[[maybe_unused]] static bool isKeyword(TokenKind Kind) {
  switch (Kind) {
#define KEYWORD(ID, STR) case kw_##ID:
#include "chocopy-llvm/Lexer/TokenKinds.def"
    return true;
  default:
    return false;
  }
}

[[maybe_unused]] static bool isPunctuator(TokenKind Kind) {
  switch (Kind) {
#define PUNCTUATOR(ID, STR) case ID:
#include "chocopy-llvm/Lexer/TokenKinds.def"
    return true;
  default:
    return false;
  }
}
} // namespace tok
} // namespace chocopy
#endif // CHOCOPY_LLVM_LEXER_TOKENKINDS_H