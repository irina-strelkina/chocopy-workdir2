#include "chocopy-llvm/Lexer/TokenKinds.h"

#include <llvm/Support/ErrorHandling.h>

using namespace chocopy;

static const char *const TokNames[] = {
#define TOK(ID) #ID,
#define KEYWORD(ID, STR) #ID,
#define PUNCTUATOR(ID, STR) #ID,
#include "chocopy-llvm/Lexer/TokenKinds.def"
    nullptr,
};

const char *tok::getTokenName(TokenKind Kind) {
  if (Kind < tok::NUM_TOKENS)
    return TokNames[Kind];
  llvm_unreachable("unknown TokenKind");
  return nullptr;
}

const char *tok::getPunctuatorSpelling(TokenKind Kind) {
  switch (Kind) {
#define PUNCTUATOR(ID, STR)                                                    \
  case ID:                                                                     \
    return STR;
#include "chocopy-llvm/Lexer/TokenKinds.def"
  default:
    break;
  }
  return nullptr;
}