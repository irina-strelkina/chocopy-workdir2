#ifndef CHOCOPY_LLVM_LEXER_LEXER_H
#define CHOCOPY_LLVM_LEXER_LEXER_H

#include "chocopy-llvm/Basic/SymbolTable.h"
#include "chocopy-llvm/Lexer/Token.h"

#include <llvm/Support/SourceMgr.h>

namespace chocopy {
class DiagnosticsEngine;

class Lexer {
public:
  Lexer(DiagnosticsEngine &Diags, llvm::SourceMgr &SrcMgr);

  DiagnosticsEngine &getDiagnostics() const { return Diags; }

  SymbolTable &getSymbolTable() { return SymbolTable; }

  void reset();

  /// Lex returns true if function returns Tok
  bool lex(Token &Tok);

  const Token &LookAhead(unsigned N) {
    if (CachedTokenPos + N < CachedTokens.size())
      return CachedTokens[CachedTokenPos + N];
    else
      return PeekAhead(N + 1);
  }

  /// @todo Add SourceLocation
  /// SourceLocation getLocation();
private:
  void enterCachingMode();
  void exitCachingMode();

  const Token &PeekAhead(unsigned N);

  bool lexImpl(Token &Tok);
  void cachingLexImpl(Token &Tok);

  bool isEof() const { return BufPtr == BufEnd; }
  const char *readNext() { return BufPtr++; }

  void handleIdentifier(Token &Tok);
  void handleIntegerLiteral(Token &Tok);
  void handleString(Token &Tok);

  void handleToken(Token &Tok, StringRef Str, tok::TokenKind Kind);

  static bool callbackLexer(Lexer &Lexer, Token &Tok) {
    return Lexer.lexImpl(Tok);
  }

  static bool callbackCachingLexer(Lexer &Lex, Token &Tok) {
    Lex.cachingLexImpl(Tok);
    return true;
  }

private:
  using CachedTokensTy = SmallVector<Token, 1>;
  using LexerCallback = bool(Lexer &, Token &);

  DiagnosticsEngine &Diags;
  llvm::SourceMgr &SourceMgr;
  CachedTokensTy CachedTokens;
  CachedTokensTy::size_type CachedTokenPos = 0;
  SmallVector<int> IndentStack = {0};
  LexerCallback *CurLexerCallback = callbackLexer;
  unsigned CurBuffer = 0;
  StringRef CurBuf;
  const char *IndentPtr = nullptr;
  const char *BufPtr = nullptr;
  const char *BufEnd = nullptr;
  bool IsCachingMode = true;
  SymbolTable SymbolTable;
  int DedentCount = 0;
  bool IsLogLineStart = false;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_LEXER_LEXER_H