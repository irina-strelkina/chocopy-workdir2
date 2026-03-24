#ifndef CHOCOPY_LLVM_LEXER_TOKEN_H
#define CHOCOPY_LLVM_LEXER_TOKEN_H

#include "chocopy-llvm/Basic/Diagnostic.h"
#include "chocopy-llvm/Basic/LLVM.h"
#include "chocopy-llvm/Basic/SymbolTable.h"
#include "chocopy-llvm/Lexer/TokenKinds.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/raw_ostream.h>

namespace chocopy {
using llvm::SMRange;

class Token {
  tok::TokenKind Kind = tok::TokenKind::unknown;
  SMRange Loc;
  void *Ptr = nullptr;
  unsigned Length = 0;

public:
  void startToken() {
    Kind = tok::unknown;
    Loc = SMRange();
    Ptr = nullptr;
    Length = 0;
  }

  void setLocation(SMRange Location) { Loc = Location; }

  SMRange getLocation() const { return Loc; }

  void setSymbolInfo(SymbolInfo *SI) { Ptr = SI; }

  SymbolInfo *getSymbolInfo() const {
    if (is(tok::identifier) || tok::isKeyword(Kind))
      return static_cast<SymbolInfo *>(Ptr);
    return nullptr;
  }

  tok::TokenKind getKind() const { return Kind; }

  void setKind(tok::TokenKind K) { Kind = K; }

  bool is(tok::TokenKind K) const { return Kind == K; }

  bool isNot(tok::TokenKind K) const { return Kind != K; }

  bool isLiteral() const { return tok::isLiteral(Kind); }

  bool isBinOp() const { return tok::isBinOp(Kind); }

  bool isOneOf(tok::TokenKind K1, tok::TokenKind K2) const {
    return is(K1) || is(K2);
  }

  template <typename... Ts> bool isOneOf(tok::TokenKind K1, Ts... Ks) const {
    return is(K1) || isOneOf(Ks...);
  }

  unsigned getLength() const { return Length; }

  void setLength(unsigned Len) { Length = Len; }

  const char *getName() const { return tok::getTokenName(Kind); }

  StringRef getLiteralData() const {
    assert(isLiteral() && "Cannot get literal data of non-literal");
    return StringRef(static_cast<char *>(Ptr), Length);
  }

  void setLiteralData(const char *DataPtr) {
    assert(isLiteral() && "Cannot set literal data of non-literal");
    Ptr = const_cast<char *>(DataPtr);
  }

  StringRef getUnknownData() const {
    assert(is(tok::unknown) && "Cannot get unknown data of non-unknow");
    return StringRef(static_cast<char *>(Ptr), Length);
  }

  void setUnknownData(const char *DataPtr) {
    assert(is(tok::unknown) && "Cannot set unknown data of non-unknown");
    Ptr = const_cast<char *>(DataPtr);
  }

  void setEofData(const char *DataPtr) {
    assert(is(tok::eof));
    Ptr = const_cast<char *>(DataPtr);
  }

  const char *getEofData() {
    assert(is(tok::eof));
    return static_cast<char *>(Ptr);
  }

  void setIndentData(const char *DataPtr) {
    assert(is(tok::INDENT));
    Ptr = const_cast<char *>(DataPtr);
  }

  const char *getIndentData() {
    assert(is(tok::INDENT));
    return static_cast<char *>(Ptr);
  }

  void setDedentData(const char *DataPtr) {
    assert(is(tok::DEDENT));
    Ptr = const_cast<char *>(DataPtr);
  }

  const char *getDedentData() {
    assert(is(tok::DEDENT));
    return static_cast<char *>(Ptr);
  }

  void setNewLineData(const char *DataPtr) {
    assert(is(tok::NEWLINE));
    Ptr = const_cast<char *>(DataPtr);
  }

  const char *getNewLineData() {
    assert(is(tok::NEWLINE));
    return static_cast<char *>(Ptr);
  }

  operator std::string() const {
    std::string Str;
    llvm::raw_string_ostream(Str) << *this;
    return Str;
  }

  friend raw_ostream &operator<<(raw_ostream &Stream, const Token &Tok) {
    if (Tok.is(tok::unknown))
      Stream << llvm::formatv("[{}]: {}", Tok.getName(), Tok.getUnknownData());
    else if (Tok.isOneOf(tok::INDENT, tok::DEDENT, tok::NEWLINE, tok::eof))
      Stream << llvm::formatv("[{}]", Tok.getName());
    else if (tok::isPunctuator(Tok.getKind()))
      Stream << llvm::formatv("[{}]: {}", Tok.getName(),
                              tok::getPunctuatorSpelling(Tok.getKind()));
    else if (Tok.isLiteral())
      Stream << llvm::formatv("[{}]: {}", Tok.getName(), Tok.getLiteralData());
    else
      Stream << llvm::formatv("[{}]: {}", Tok.getName(),
                              Tok.getSymbolInfo()->getName());
    return Stream;
  }

  friend InFlightDiagnostic &operator<<(InFlightDiagnostic &&D,
                                        const Token &Tok) {
    D << std::string(Tok);
    return D;
  }
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_LEXER_TOKEN_H
