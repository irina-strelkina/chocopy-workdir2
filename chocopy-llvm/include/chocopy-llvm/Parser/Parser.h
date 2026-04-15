#ifndef CHOCOPY_LLVM_PARSER_PARSER_H
#define CHOCOPY_LLVM_PARSER_PARSER_H

#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/Lexer/Lexer.h"

namespace chocopy {
class Sema;

class Parser {
public:
  Parser(ASTContext &C, Lexer &Lex, Sema &Acts);

  Program *parse();

private:
  bool consumeToken(tok::TokenKind ExpectedTok);
  bool consumeToken();

  bool expect(tok::TokenKind ExpectedTok);
  bool expectAndConsume(tok::TokenKind ExpectedTok);

  void skipToNextLine();
  void skipToNextDedent();

  void emitUnexpected();

  const Token &getLookAheadToken(int N);

  Program *parseProgram();

  // Declarations
  bool isVarDefStart() const;
  bool isDeclStart() const;
  Declaration *parseDeclaration(bool InClassScope = false,
                                bool InFunctionScope = false);
  ClassDef *parseClassDef();
  FuncDef *parseFuncDef();
  GlobalDecl *parseGlobalDecl();
  NonLocalDecl *parseNonLocalDecl();
  VarDef *parseVarDef();
  ParamDecl *parseTypedVar(bool IsParam = false);

  // Statements
  bool parseStmt(StmtList &Statements);
  Stmt *parseSimpleStmt();
  IfStmt *parseIfStmt();
  WhileStmt *parseWhileStmt();
  ForStmt *parseForStmt();
  bool parseSuite(StmtList &Statements);

  // Expressions
  Expr *parseExpr();
  Expr *parseOrExpr();
  Expr *parseAndExpr();
  Expr *parseNotExpr();
  Expr *parseCmpExpr();
  Expr *parseAddExpr();
  Expr *parseMulExpr();
  Expr *parseUnaryExpr();
  Expr *parsePostfixExpr();
  Expr *parsePrimaryExpr();

  // Types / literals
  TypeAnnotation *parseType();
  Literal *parseLiteral();

  bool isAssignTarget(const Expr *E) const;

private:
  DiagnosticsEngine &Diags;
  ASTContext &Context;
  Lexer &TheLexer;
  Token Tok;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_PARSER_PARSER_H
