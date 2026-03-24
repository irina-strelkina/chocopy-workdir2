#include "chocopy-llvm/Lexer/Lexer.h"

namespace chocopy {
void Lexer::enterCachingMode() {
  IsCachingMode = true;
  CurLexerCallback = callbackCachingLexer;
}

void Lexer::exitCachingMode() {
  IsCachingMode = false;
  CurLexerCallback = callbackLexer;
}

const Token &Lexer::PeekAhead(unsigned N) {
  assert(CachedTokenPos + N > CachedTokens.size() && "Confused caching.");
  exitCachingMode();
  for (size_t C = CachedTokenPos + N - CachedTokens.size(); C > 0; --C) {
    CachedTokens.push_back(Token());
    lex(CachedTokens.back());
  }
  enterCachingMode();
  return CachedTokens.back();
}

void Lexer::cachingLexImpl(Token &Tok) {
  if (!IsCachingMode)
    return;

  if (CachedTokenPos < CachedTokens.size()) {
    Tok = CachedTokens[CachedTokenPos++];
    return;
  }

  exitCachingMode();
  lex(Tok);

  CachedTokenPos = 0;
  CachedTokens.clear();
}
} // namespace chocopy