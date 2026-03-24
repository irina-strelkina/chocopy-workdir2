#include "chocopy-llvm/Parser/Parser.h"
#include "chocopy-llvm/AST/ASTContext.h"
#include "chocopy-llvm/Basic/Diagnostic.h"
#include "chocopy-llvm/Sema/Scope.h"
#include "chocopy-llvm/Sema/Sema.h"

#include <llvm/ADT/APInt.h>

#include <stack>

namespace chocopy {
Parser::Parser(ASTContext &C, Lexer &Lex, Sema &Acts)
    : Diags(Lex.getDiagnostics()), Context(C), TheLexer(Lex) {}

Program *Parser::parse() {
  Program *P = parseProgramDemo();
  return P;
}

bool Parser::consumeToken(tok::TokenKind ExpectedTok) {
  if (Tok.is(ExpectedTok)) {
    TheLexer.lex(Tok);
    return true;
  }
  return false;
}

bool Parser::consumeToken() {
  TheLexer.lex(Tok);
  return true;
}

bool Parser::expect(tok::TokenKind ExpectedTok) {
  if (Tok.is(ExpectedTok))
    return true;
  Diags.emitError(Tok.getLocation().Start, diag::err_near_token) << Tok;
  Diags.emitError(Tok.getLocation().Start, diag::err_expected)
      << tok::getTokenName(ExpectedTok);
  return false;
}

bool Parser::expectAndConsume(tok::TokenKind ExpectedTok) {
  return expect(ExpectedTok) && consumeToken();
}

void Parser::skipToNextLine() {
  while (!Tok.is(tok::eof) && !consumeToken(tok::NEWLINE))
    consumeToken();
}

void Parser::emitUnexpected() {
  Diags.emitError(Tok.getLocation().Start, diag::err_unexpected) << Tok;
}

const Token &Parser::getLookAheadToken(int N) {
  assert(N);
  return TheLexer.LookAhead(N - 1);
}

Program *Parser::parseProgramDemo() {
  DeclList Declarations;
  StmtList Statements;

  auto IsVarDef = [this](Token &Tok) {
    return Tok.isOneOf(tok::identifier, tok::idstring) &&
           TheLexer.LookAhead(0).is(tok::colon);
  };

  consumeToken();
  while (IsVarDef(Tok)) {
    if (VarDef *V = parseVarDef())
      Declarations.push_back(V);
    else
      skipToNextLine();
  }

  while (Tok.isNot(tok::eof)) {
    if (Stmt *S = parseStmtDemo())
      Statements.push_back(S);
    else
      skipToNextLine();
  }

  return Context.createProgram(Declarations, Statements);
}

Stmt *Parser::parseStmtDemo() {
  if (Stmt *S = parseAssignOrExprDemo())
    if (expectAndConsume(tok::NEWLINE))
      return S;

  return nullptr;
}

Stmt *Parser::parseAssignOrExprDemo() {
  auto IsTarget = [](const Expr *E) { return llvm::isa<DeclRef>(E); };

  SMLoc SLoc = Tok.getLocation().Start;
  ExprList Targets;
  Expr *E = nullptr;
  do {
    if (E)
      Targets.push_back(E);

    E = parseExprDemo();
    if (!E)
      return nullptr;
  } while (IsTarget(E) && consumeToken(tok::equal));

  if (!expect(tok::NEWLINE))
    return nullptr;

  SMLoc ELoc = Tok.getLocation().Start;
  SMRange Loc(SLoc, ELoc);
  if (!Targets.empty())
    return Context.createAssignStmt(Loc, Targets, E);

  return Context.createExprStmt(Loc, E);
}

Expr *Parser::parseExprDemo() { return parseBinaryAddOrSubDemo(); }

Expr *Parser::parseBinaryAddOrSubDemo() {
  Expr *L = parseBinaryMulOrDivOrModDemo();
  if (!L)
    return nullptr;

  auto IsContinue = [](const Token &T) {
    return T.isOneOf(tok::plus, tok::minus);
  };

  while (IsContinue(Tok)) {
    SMRange Loc = Tok.getLocation();
    BinaryExpr::OpKind K =
        Tok.is(tok::plus) ? BinaryExpr::OpKind::Add : BinaryExpr::OpKind::Sub;
    consumeToken();

    Expr *R = parseBinaryMulOrDivOrModDemo();
    if (!R)
      return nullptr;

    L = Context.createBinaryExpr(Loc, L, K, R);
  }
  return L;
}

Expr *Parser::parseBinaryMulOrDivOrModDemo() {
  Expr *L = parseFnCallDemo();
  if (!L)
    return nullptr;

  auto IsContinue = [](const Token &T) {
    return T.isOneOf(tok::star, tok::slashslash, tok::percent);
  };

  while (IsContinue(Tok)) {
    BinaryExpr::OpKind K = BinaryExpr::OpKind::Mul;
    if (Tok.is(tok::slashslash))
      K = BinaryExpr::OpKind::FloorDiv;
    else if (Tok.is(tok::percent))
      K = BinaryExpr::OpKind::Mod;
    consumeToken();

    Expr *R = parseFnCallDemo();
    if (!R)
      return nullptr;
    L = Context.createBinaryExpr(Tok.getLocation(), L, K, R);
  }
  return L;
}

Expr *Parser::parseFnCallDemo() {
  Expr *E = parseAtomicExprDemo();
  if (!E)
    return nullptr;

  auto IsContinue = [](const Token &T) {
    return !T.is(tok::eof) && T.is(tok::l_paren);
  };

  auto ParseArgs = [this](ExprList &Args) {
    do {
      Expr *A = parseExprDemo();
      if (!A)
        return false;
      Args.push_back(A);
    } while (consumeToken(tok::comma));
    return true;
  };

  if (DeclRef *DR = dyn_cast<DeclRef>(E)) {
    // parse func(a)(b)(c)...()
    while (IsContinue(Tok)) {
      ExprList Args;
      consumeToken();
      if (Tok.isNot(tok::r_paren))
        if (!ParseArgs(Args))
          return nullptr;

      if (!expectAndConsume(tok::r_paren))
        return nullptr;

      SMRange Loc(DR->getLocation().Start, Tok.getLocation().Start);
      E = Context.createCallExpr(Loc, DR, Args);
    }
  }
  return E;
}

Expr *Parser::parseAtomicExprDemo() {
  SMRange Loc = Tok.getLocation();

  if (Tok.is(tok::identifier)) {
    Expr *E = Context.createDeclRef(Loc, Tok.getSymbolInfo());
    consumeToken();
    return E;
  }

  if (Tok.is(tok::integer_literal)) {
    llvm::APInt Val(32, Tok.getLiteralData(), 10);
    Expr *E = Context.createIntegerLiteral(Loc, Val.getSExtValue());
    consumeToken();
    return E;
  }

  if (Tok.is(tok::l_paren)) {
    consumeToken();
    Expr *E = parseExprDemo();
    if (!E || !expectAndConsume(tok::r_paren))
      return nullptr;
    return E;
  }

  emitUnexpected();
  return nullptr;
}

/// type = ID | IDSTRING | '[' type ']'
TypeAnnotation *Parser::parseType() {
  SMRange Loc = Tok.getLocation();
  switch (Tok.getKind()) {
  case tok::identifier: {
    StringRef Name = Tok.getSymbolInfo()->getName();
    consumeToken();
    return Context.createClassType(Loc, Name);
  }
  case tok::idstring: {
    StringRef Name = Tok.getLiteralData();
    consumeToken();
    return Context.createClassType(Loc, Name);
  }
  case tok::l_square: {
    consumeToken();
    if (TypeAnnotation *T = parseType()) {
      if (expectAndConsume(tok::r_square)) {
        Loc = SMRange(Loc.Start, Tok.getLocation().End);
        return Context.createListType(Loc, T);
      }
    }
    return nullptr;
  }
  default:
    return nullptr;
  }
}

/// var_def = typed_var '=' literal NEWLINE
VarDef *Parser::parseVarDef() {

  if (!expect(tok::identifier))
    return nullptr;

  SymbolInfo *Name = Tok.getSymbolInfo();
  SMRange NameLoc = Tok.getLocation();
  consumeToken();

  if (!expectAndConsume(tok::colon))
    return nullptr;

  TypeAnnotation *T = parseType();
  if (!expectAndConsume(tok::equal))
    return nullptr;

  if (Literal *L = parseLiteral()) {
    if (expectAndConsume(tok::NEWLINE)) {
      SMLoc ELoc = L->getLocation().End;
      SMRange Loc(NameLoc.Start, ELoc);
      Identifier *V = Context.createIdentifier(NameLoc, Name);
      return Context.createVarDef(Loc, V, T, L);
    }
  }

  return nullptr;
}

Literal *Parser::parseLiteral() {
  SMRange Loc = Tok.getLocation();

  if (consumeToken(tok::kw_None)) {
    return Context.createNoneLiteral(Loc);
  } else if (consumeToken(tok::kw_True)) {
    return Context.createBooleanLiteral(Loc, true);
  } else if (consumeToken(tok::kw_False)) {
    return Context.createBooleanLiteral(Loc, false);
  } else if (Tok.is(tok::integer_literal)) {
    llvm::APInt Value(32, Tok.getLiteralData(), 10);
    consumeToken();
    return Context.createIntegerLiteral(Loc, Value.getSExtValue());
  } else if (Tok.isOneOf(tok::idstring, tok::string_literal)) {
    StringRef Str = Tok.getLiteralData();
    consumeToken();
    return Context.createStringLiteral(Loc, Str);
  }

  Diags.emitError(Tok.getLocation().Start, diag::err_near_token) << Tok;
  return nullptr;
}
} // namespace chocopy
