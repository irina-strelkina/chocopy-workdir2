#include "chocopy-llvm/Lexer/Lexer.h"
#include "chocopy-llvm/Basic/ASCIICharInfo.h"
#include "chocopy-llvm/Basic/Diagnostic.h"

#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringSwitch.h>

namespace chocopy {
static llvm::StringMap<tok::TokenKind> KwTable = {
#define KEYWORD(KW, STR) std::make_pair(StringRef(STR), tok::kw_##KW),
#include "chocopy-llvm/Lexer/TokenKinds.def"
};

bool isKeyword(StringRef Identifier) { return KwTable.contains(Identifier); }

tok::TokenKind getKeword(StringRef Identifier,
                         tok::TokenKind DefaultTokenCode = tok::unknown) {
  auto Result = KwTable.find(Identifier);
  if (Result != KwTable.end())
    return Result->second;
  return DefaultTokenCode;
}

static void initializeSymbolTable(SymbolTable &ST) {
  for (auto &Item : KwTable)
    ST.get(Item.getKey(), Item.getValue());
}

Lexer::Lexer(DiagnosticsEngine &Diags, llvm::SourceMgr &SrcMgr)
    : Diags(Diags), SourceMgr(SrcMgr), CurBuffer(SourceMgr.getMainFileID()),
      CurBuf(SourceMgr.getMemoryBuffer(CurBuffer)->getBuffer()),
      BufPtr(CurBuf.begin()), BufEnd(CurBuf.end()) {
  initializeSymbolTable(SymbolTable);
}

void Lexer::reset() {
  IndentStack = {0};
  DedentCount = 0;
  IsLogLineStart = false;
  BufPtr = CurBuf.begin();
  CurLexerCallback = callbackLexer;
  CachedTokenPos = 0;
  CachedTokens.clear();
  SymbolTable = chocopy::SymbolTable();
  initializeSymbolTable(SymbolTable);
}

bool Lexer::lex(Token &Tok) {
  while (!CurLexerCallback(*this, Tok))
    ;
  return true;
}

bool Lexer::lexImpl(Token &Tok) {
  Tok.startToken();

  if (DedentCount) {
    SMLoc L = SMLoc::getFromPointer(IndentPtr);
    Tok.setKind(tok::DEDENT);
    Tok.setDedentData(IndentPtr);
    Tok.setLocation(SMRange(L, L));
    DedentCount--;
    return true;
  }

  const char *Ptr = BufPtr;
  while (!IsLogLineStart && !isEof()) {
    int Indention = 0;
    while (!isEof()) {
      if (*BufPtr == ' ')
        Indention += 1;
      else if (*BufPtr == '\t')
        Indention += 8 - Indention % 8;
      else
        break;
      readNext();
    }

    IndentPtr = BufPtr;

    if (*BufPtr == '#') {
      do {
        readNext();
      } while (!isEof() && !isVerticalWhitespace(*BufPtr));
    }

    if (isVerticalWhitespace(*BufPtr)) {
      readNext();
      continue;
    }

    if (isEof())
      break;

    IsLogLineStart = true;
    int IndentDiff = IndentStack.back() - Indention;
    if (IndentDiff > 0) {
      while (IndentStack.back() > Indention) {
        DedentCount++;
        IndentStack.pop_back();
      }

      if (IndentStack.back() != Indention) {
        SMLoc S = SMLoc::getFromPointer(Ptr);
        SMLoc E = SMLoc::getFromPointer(IndentPtr);
        Diags.emitError(S, diag::err_badent);
        Tok.setKind(tok::BADENT);
        Tok.setLocation(SMRange(S, E));
        return true;
      }

      return false;
    }

    if (IndentDiff < 0) {
      SMLoc S = SMLoc::getFromPointer(Ptr);
      SMLoc E = SMLoc::getFromPointer(IndentPtr);
      SMRange Loc(S, E);
      Tok.setKind(tok::INDENT);
      Tok.setIndentData(BufPtr);
      Tok.setLocation(Loc);
      IndentStack.push_back(Indention);
      return true;
    }
  }

  IndentPtr = BufPtr;
  while (!isEof() && !isVerticalWhitespace(*BufPtr)) {
    if (*BufPtr == '#') {
      do {
        readNext();
      } while (!isEof() && !isVerticalWhitespace(*BufPtr));
      continue;
    }

    if (isHorizontalWhitespace(*BufPtr)) {
      do {
        readNext();
      } while (isHorizontalWhitespace(*BufPtr));
      continue;
    }

    if (isChocopyIdentifierHead(*BufPtr)) {
      handleIdentifier(Tok);
      return true;
    }

    if (isDigit(*BufPtr)) {
      handleIntegerLiteral(Tok);
      return true;
    }

    if (*BufPtr == '\"') {
      handleString(Tok);
      return true;
    }

#define PUNCTUATOR(KIND, STR)                                                  \
  if (StringRef(BufPtr).starts_with(STR)) {                                    \
    handleToken(Tok, STR, tok::KIND);                                          \
    return true;                                                               \
  }
#include "chocopy-llvm/Lexer/TokenKinds.def"

    Tok.setKind(tok::unknown);
    Tok.setUnknownData(readNext());
    Tok.setLength(1);
    SMLoc S = SMLoc::getFromPointer(BufPtr - 1);
    SMLoc E = SMLoc::getFromPointer(BufPtr);
    Tok.setLocation(SMRange(S, E));
    Diags.emitError(S, diag::err_unknow_token) << Tok;
    return true;
  }

  SMLoc L = SMLoc::getFromPointer(BufPtr);
  Tok.setLocation(SMRange(L, L));
  Tok.setLength(1);

  if (IsLogLineStart) {
    IsLogLineStart = false;
    Tok.setKind(tok::NEWLINE);
    Tok.setNewLineData(BufPtr);
    if (isVerticalWhitespace(*BufPtr))
      readNext();
    return true;
  }

  assert(isEof());

  if (IndentStack.empty()) {
    Tok.setKind(tok::eof);
    Tok.setEofData(BufPtr);
    return true;
  }

  DedentCount = IndentStack.size() - 1;
  IndentStack.clear();
  return false;
}

void Lexer::handleIdentifier(Token &Tok) {
  const char *Ptr = BufPtr;
  while (isChocopyIdentifierBody(*BufPtr))
    readNext();

  unsigned Length = BufPtr - Ptr;
  SMLoc B = SMLoc::getFromPointer(Ptr);
  SMLoc E = SMLoc::getFromPointer(BufPtr);
  SymbolInfo &SI = SymbolTable.get(StringRef(Ptr, Length));
  Tok.setKind(SI.getKind());
  Tok.setLocation(SMRange(B, E));
  Tok.setSymbolInfo(&SI);
  Tok.setLength(Length);
}

void Lexer::handleIntegerLiteral(Token &Tok) {
  const char *Ptr = BufPtr;
  while (isDigit(*BufPtr))
    readNext();

  unsigned Length = BufPtr - Ptr;
  SMLoc B = SMLoc::getFromPointer(Ptr);
  SMLoc E = SMLoc::getFromPointer(BufPtr);
  Tok.setKind(tok::integer_literal);
  Tok.setLocation(SMRange(B, E));
  Tok.setLiteralData(Ptr);
  Tok.setLength(Length);
}

void Lexer::handleString(Token &Tok) {
  bool IsId = true;
  const char *Start = readNext();
  while (!isEof() && *Start != *BufPtr && !isVerticalWhitespace(*BufPtr)) {
    if (*BufPtr == '\\') {
      IsId = false;
      if (++BufPtr != BufEnd)
        readNext();
      continue;
    }

    if (!isChocopyIdentifierBody(*readNext()))
      IsId = false;
  }

  if (IsId && isChocopyIdentifierHead(*++Start))
    Tok.setKind(tok::idstring);
  else
    Tok.setKind(tok::string_literal);

  unsigned Length = BufPtr - Start;
  SMLoc B = SMLoc::getFromPointer(Start);
  SMLoc E = SMLoc::getFromPointer(BufPtr);
  Tok.setLocation(SMRange(B, E));
  Tok.setLiteralData(Start);
  Tok.setLength(Length);

  if (!isEof() && *BufPtr == '\"')
    readNext();
}

void Lexer::handleToken(Token &Tok, StringRef Str, tok::TokenKind Kind) {
  SymbolInfo &SI = SymbolTable.get(Str, Kind);
  Tok.setKind(SI.getKind());
  SMLoc B = SMLoc::getFromPointer(BufPtr);
  SMLoc E = SMLoc::getFromPointer(BufPtr + SI.getName().size());

  Tok.setLocation(SMRange(B, E));
  Tok.setSymbolInfo(&SI);
  Tok.setLength(SI.getName().size());
  BufPtr += Str.size();
}
} // namespace chocopy