#include "chocopy-llvm/Parser/Parser.h"
#include "chocopy-llvm/AST/ASTContext.h"
#include "chocopy-llvm/Basic/Diagnostic.h"
#include "chocopy-llvm/Sema/Sema.h"

#include <llvm/ADT/APInt.h>

namespace chocopy {
Parser::Parser(ASTContext &C, Lexer &Lex, Sema &Acts)
    : Diags(Lex.getDiagnostics()), Context(C), TheLexer(Lex) {
  (void)Acts;
}

Program *Parser::parse() { return parseProgram(); }

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

void Parser::skipToNextDedent() {
  while (Tok.isNot(tok::eof) && Tok.isNot(tok::DEDENT))
    consumeToken();
  consumeToken(tok::DEDENT);
}

void Parser::emitUnexpected() {
  Diags.emitError(Tok.getLocation().Start, diag::err_unexpected) << Tok;
}

const Token &Parser::getLookAheadToken(int N) {
  assert(N);
  return TheLexer.LookAhead(N - 1);
}

bool Parser::isVarDefStart() const {
  return Tok.isOneOf(tok::identifier, tok::idstring) &&
         TheLexer.LookAhead(0).is(tok::colon);
}

bool Parser::isDeclStart() const {
  return Tok.isOneOf(tok::kw_def, tok::kw_class, tok::kw_global,
                     tok::kw_nonlocal) ||
         isVarDefStart();
}

Program *Parser::parseProgram() {
  DeclList Declarations;
  StmtList Statements;

  consumeToken();

  while (isDeclStart()) {
    if (Declaration *D = parseDeclaration())
      Declarations.push_back(D);
    else
      skipToNextLine();
  }

  while (Tok.isNot(tok::eof)) {
    if (!parseStmt(Statements)) {
      skipToNextLine();
    }
  }

  return Context.createProgram(Declarations, Statements);
}

Declaration *Parser::parseDeclaration(bool InClassScope, bool InFunctionScope) {
  if (Tok.is(tok::kw_class)) {
    if (InClassScope || InFunctionScope) {
      emitUnexpected();
      return nullptr;
    }
    return parseClassDef();
  }

  if (Tok.is(tok::kw_def))
    return parseFuncDef();

  if (Tok.is(tok::kw_global)) {
    if (!InFunctionScope) {
      emitUnexpected();
      return nullptr;
    }
    return parseGlobalDecl();
  }

  if (Tok.is(tok::kw_nonlocal)) {
    if (!InFunctionScope) {
      emitUnexpected();
      return nullptr;
    }
    return parseNonLocalDecl();
  }

  if (isVarDefStart())
    return parseVarDef();

  emitUnexpected();
  return nullptr;
}

ClassDef *Parser::parseClassDef() {
  SMLoc Start = Tok.getLocation().Start;
  consumeToken(); // class

  if (!expect(tok::identifier))
    return nullptr;
  SMRange NameLoc = Tok.getLocation();
  SymbolInfo *Name = Tok.getSymbolInfo();
  consumeToken();

  if (!expectAndConsume(tok::l_paren))
    return nullptr;

  if (!expect(tok::identifier))
    return nullptr;
  SMRange SuperLoc = Tok.getLocation();
  SymbolInfo *Super = Tok.getSymbolInfo();
  consumeToken();

  if (!expectAndConsume(tok::r_paren) || !expectAndConsume(tok::colon) ||
      !expectAndConsume(tok::NEWLINE) || !expectAndConsume(tok::INDENT))
    return nullptr;

  DeclList Declarations;
  while (Tok.isNot(tok::DEDENT) && Tok.isNot(tok::eof)) {
    if (Declaration *D = parseDeclaration(/*InClassScope=*/true,
                                          /*InFunctionScope=*/false)) {
      Declarations.push_back(D);
      continue;
    }

    // Abort body parse on malformed declaration prefix.
    if (!isDeclStart()) {
      emitUnexpected();
      skipToNextDedent();
      break;
    }
    skipToNextLine();
  }

  if (!expectAndConsume(tok::DEDENT))
    return nullptr;

  SMLoc End = NameLoc.End;
  if (!Declarations.empty())
    End = Declarations.back()->getLocation().End;

  SMRange Loc(Start, End);
  Identifier *ClassId = Context.createIdentifier(NameLoc, Name);
  Identifier *SuperId = Context.createIdentifier(SuperLoc, Super);
  return Context.createClassDef(Loc, ClassId, SuperId, Declarations);
}

ParamDecl *Parser::parseTypedVar(bool IsParam) {
  if (!expect(tok::identifier))
    return nullptr;

  SMRange NameLoc = Tok.getLocation();
  SymbolInfo *Name = Tok.getSymbolInfo();
  consumeToken();

  if (!expectAndConsume(tok::colon))
    return nullptr;

  TypeAnnotation *T = parseType();
  if (!T)
    return nullptr;

  Identifier *Id = Context.createIdentifier(NameLoc, Name);
  if (IsParam)
    return Context.createParamDecl(SMRange(NameLoc.Start, Tok.getLocation().Start),
                                   Id, T);

  return nullptr;
}

FuncDef *Parser::parseFuncDef() {
  SMLoc Start = Tok.getLocation().Start;
  consumeToken(); // def

  if (!expect(tok::identifier))
    return nullptr;
  SMRange NameLoc = Tok.getLocation();
  SymbolInfo *Name = Tok.getSymbolInfo();
  consumeToken();

  if (!expectAndConsume(tok::l_paren))
    return nullptr;

  ParamDeclList Params;
  if (Tok.isNot(tok::r_paren)) {
    do {
      ParamDecl *P = parseTypedVar(/*IsParam=*/true);
      if (!P)
        return nullptr;
      Params.push_back(P);
    } while (consumeToken(tok::comma));
  }

  if (!expectAndConsume(tok::r_paren))
    return nullptr;

  TypeAnnotation *RetType = nullptr;
  if (consumeToken(tok::arrow)) {
    RetType = parseType();
    if (!RetType)
      return nullptr;
  } else {
    SMRange NoneLoc(Tok.getLocation().Start, Tok.getLocation().Start);
    RetType = Context.createClassType(NoneLoc, "<None>");
  }

  if (!expectAndConsume(tok::colon) || !expectAndConsume(tok::NEWLINE) ||
      !expectAndConsume(tok::INDENT))
    return nullptr;

  DeclList Declarations;
  while (isDeclStart()) {
    if (Declaration *D = parseDeclaration(/*InClassScope=*/false,
                                          /*InFunctionScope=*/true)) {
      Declarations.push_back(D);
      continue;
    }
    skipToNextLine();
  }

  StmtList Statements;
  while (Tok.isNot(tok::DEDENT) && Tok.isNot(tok::eof)) {
    if (!parseStmt(Statements)) {
      skipToNextLine();
    }
  }

  if (!expectAndConsume(tok::DEDENT))
    return nullptr;

  SMLoc End = NameLoc.End;
  if (!Declarations.empty())
    End = Declarations.back()->getLocation().End;
  if (!Statements.empty())
    End = Statements.back()->getLocation().End;

  SMRange Loc(Start, End);
  Identifier *FuncId = Context.createIdentifier(NameLoc, Name);
  return Context.createFuncDef(Loc, FuncId, Params, RetType, Declarations,
                               Statements);
}

GlobalDecl *Parser::parseGlobalDecl() {
  SMLoc Start = Tok.getLocation().Start;
  consumeToken();

  if (!expect(tok::identifier))
    return nullptr;

  SMRange NameLoc = Tok.getLocation();
  SymbolInfo *Name = Tok.getSymbolInfo();
  consumeToken();

  if (!expect(tok::NEWLINE))
    return nullptr;
  SMLoc End = NameLoc.End;
  consumeToken();

  Identifier *Id = Context.createIdentifier(NameLoc, Name);
  return Context.createGlobalDecl(SMRange(Start, End), Id);
}

NonLocalDecl *Parser::parseNonLocalDecl() {
  SMLoc Start = Tok.getLocation().Start;
  consumeToken();

  if (!expect(tok::identifier))
    return nullptr;

  SMRange NameLoc = Tok.getLocation();
  SymbolInfo *Name = Tok.getSymbolInfo();
  consumeToken();

  if (!expect(tok::NEWLINE))
    return nullptr;
  SMLoc End = NameLoc.End;
  consumeToken();

  Identifier *Id = Context.createIdentifier(NameLoc, Name);
  return Context.createNonLocalDecl(SMRange(Start, End), Id);
}

VarDef *Parser::parseVarDef() {
  if (!expect(tok::identifier))
    return nullptr;

  SymbolInfo *Name = Tok.getSymbolInfo();
  SMRange NameLoc = Tok.getLocation();
  consumeToken();

  if (!expectAndConsume(tok::colon))
    return nullptr;

  TypeAnnotation *T = parseType();
  if (!T || !expectAndConsume(tok::equal))
    return nullptr;

  if (Literal *L = parseLiteral()) {
    if (expectAndConsume(tok::NEWLINE)) {
      SMRange Loc(NameLoc.Start, L->getLocation().End);
      Identifier *V = Context.createIdentifier(NameLoc, Name);
      return Context.createVarDef(Loc, V, T, L);
    }
  }

  return nullptr;
}

bool Parser::parseStmt(StmtList &Statements) {
  if (Tok.is(tok::kw_if)) {
    if (IfStmt *S = parseIfStmt()) {
      Statements.push_back(S);
      return true;
    }
    return false;
  }

  if (Tok.is(tok::kw_while)) {
    if (WhileStmt *S = parseWhileStmt()) {
      Statements.push_back(S);
      return true;
    }
    return false;
  }

  if (Tok.is(tok::kw_for)) {
    if (ForStmt *S = parseForStmt()) {
      Statements.push_back(S);
      return true;
    }
    return false;
  }

  if (Tok.is(tok::kw_pass)) {
    consumeToken();
    return expectAndConsume(tok::NEWLINE);
  }

  if (Stmt *S = parseSimpleStmt()) {
    Statements.push_back(S);
    return true;
  }

  return false;
}

Stmt *Parser::parseSimpleStmt() {
  SMLoc Start = Tok.getLocation().Start;

  if (Tok.is(tok::kw_return)) {
    consumeToken();

    Expr *Value = nullptr;
    if (Tok.isNot(tok::NEWLINE)) {
      Value = parseExpr();
      if (!Value)
        return nullptr;
    }

    if (!expect(tok::NEWLINE))
      return nullptr;
    SMLoc End = Tok.getLocation().Start;
    consumeToken();

    if (Value)
      End = Value->getLocation().End;
    return Context.createReturnStmt(SMRange(Start, End), Value);
  }

  ExprList Targets;
  Expr *E = parseExpr();
  if (!E)
    return nullptr;

  while (consumeToken(tok::equal)) {
    if (!isAssignTarget(E)) {
      emitUnexpected();
      return nullptr;
    }
    Targets.push_back(E);

    E = parseExpr();
    if (!E)
      return nullptr;
  }

  if (!expect(tok::NEWLINE))
    return nullptr;
  SMLoc End = Tok.getLocation().Start;
  consumeToken();

  if (E->getLocation().End.getPointer() > End.getPointer())
    End = E->getLocation().End;

  SMRange Loc(Start, End);
  if (!Targets.empty())
    return Context.createAssignStmt(Loc, Targets, E);
  return Context.createExprStmt(Loc, E);
}

IfStmt *Parser::parseIfStmt() {
  SMLoc Start = Tok.getLocation().Start;
  consumeToken();

  Expr *Cond = parseExpr();
  if (!Cond || !expectAndConsume(tok::colon) || !expectAndConsume(tok::NEWLINE))
    return nullptr;

  StmtList ThenBody;
  if (!parseSuite(ThenBody))
    return nullptr;

  StmtList ElseBody;
  if (Tok.is(tok::kw_elif)) {
    if (IfStmt *Elif = parseIfStmt())
      ElseBody.push_back(Elif);
    else
      return nullptr;
  } else if (Tok.is(tok::kw_else)) {
    consumeToken();
    if (!expectAndConsume(tok::colon) || !expectAndConsume(tok::NEWLINE) ||
        !parseSuite(ElseBody))
      return nullptr;
  }

  return Context.createIfStmt(SMRange(Start, Tok.getLocation().Start), Cond,
                              ThenBody, ElseBody);
}

WhileStmt *Parser::parseWhileStmt() {
  SMLoc Start = Tok.getLocation().Start;
  consumeToken();

  Expr *Cond = parseExpr();
  if (!Cond || !expectAndConsume(tok::colon) || !expectAndConsume(tok::NEWLINE))
    return nullptr;

  StmtList Body;
  if (!parseSuite(Body))
    return nullptr;

  return Context.createWhileStmt(SMRange(Start, Tok.getLocation().Start), Cond,
                                 Body);
}

ForStmt *Parser::parseForStmt() {
  SMLoc Start = Tok.getLocation().Start;
  consumeToken();

  if (!expect(tok::identifier))
    return nullptr;

  SMRange NameLoc = Tok.getLocation();
  SymbolInfo *Name = Tok.getSymbolInfo();
  consumeToken();

  if (!expectAndConsume(tok::kw_in))
    return nullptr;

  Expr *Iterable = parseExpr();
  if (!Iterable || !expectAndConsume(tok::colon) ||
      !expectAndConsume(tok::NEWLINE))
    return nullptr;

  StmtList Body;
  if (!parseSuite(Body))
    return nullptr;

  DeclRef *Target = Context.createDeclRef(NameLoc, Name);
  return Context.createForStmt(SMRange(Start, Tok.getLocation().Start), Target,
                               Iterable, Body);
}

bool Parser::parseSuite(StmtList &Statements) {
  if (!expectAndConsume(tok::INDENT))
    return false;

  while (Tok.isNot(tok::DEDENT) && Tok.isNot(tok::eof)) {
    if (!parseStmt(Statements))
      return false;
  }

  return expectAndConsume(tok::DEDENT);
}

Expr *Parser::parseExpr() {
  Expr *LHS = parseOrExpr();
  if (!LHS)
    return nullptr;

  if (consumeToken(tok::kw_if)) {
    Expr *Cond = parseOrExpr();
    if (!Cond || !expectAndConsume(tok::kw_else))
      return nullptr;

    Expr *ElseExpr = parseExpr();
    if (!ElseExpr)
      return nullptr;

    return Context.createIfExpr(SMRange(LHS->getLocation().Start,
                                        ElseExpr->getLocation().End),
                                Cond, LHS, ElseExpr);
  }

  return LHS;
}

Expr *Parser::parseOrExpr() {
  Expr *L = parseAndExpr();
  if (!L)
    return nullptr;

  while (Tok.is(tok::kw_or)) {
    consumeToken();
    Expr *R = parseAndExpr();
    if (!R)
      return nullptr;
    L = Context.createBinaryExpr(SMRange(L->getLocation().Start,
                                         R->getLocation().End),
                                 L, BinaryExpr::OpKind::Or, R);
  }

  return L;
}

Expr *Parser::parseAndExpr() {
  Expr *L = parseNotExpr();
  if (!L)
    return nullptr;

  while (Tok.is(tok::kw_and)) {
    consumeToken();
    Expr *R = parseNotExpr();
    if (!R)
      return nullptr;
    L = Context.createBinaryExpr(SMRange(L->getLocation().Start,
                                         R->getLocation().End),
                                 L, BinaryExpr::OpKind::And, R);
  }

  return L;
}

Expr *Parser::parseNotExpr() {
  if (Tok.is(tok::kw_not)) {
    SMRange OpLoc = Tok.getLocation();
    consumeToken();
    Expr *Operand = parseNotExpr();
    if (!Operand)
      return nullptr;
    return Context.createUnaryExpr(SMRange(OpLoc.Start, Operand->getLocation().End),
                                   UnaryExpr::OpKind::Not, Operand);
  }

  return parseCmpExpr();
}

Expr *Parser::parseCmpExpr() {
  Expr *L = parseAddExpr();
  if (!L)
    return nullptr;

  BinaryExpr::OpKind K;
  if (Tok.is(tok::equalequal))
    K = BinaryExpr::OpKind::EqCmp;
  else if (Tok.is(tok::exclaimequal))
    K = BinaryExpr::OpKind::NEqCmp;
  else if (Tok.is(tok::lessequal))
    K = BinaryExpr::OpKind::LEqCmp;
  else if (Tok.is(tok::greaterequal))
    K = BinaryExpr::OpKind::GEqCmp;
  else if (Tok.is(tok::less))
    K = BinaryExpr::OpKind::LCmp;
  else if (Tok.is(tok::greater))
    K = BinaryExpr::OpKind::GCmp;
  else if (Tok.is(tok::kw_is))
    K = BinaryExpr::OpKind::Is;
  else
    return L;

  consumeToken();
  Expr *R = parseAddExpr();
  if (!R)
    return nullptr;

  return Context.createBinaryExpr(SMRange(L->getLocation().Start,
                                          R->getLocation().End),
                                  L, K, R);
}

Expr *Parser::parseAddExpr() {
  Expr *L = parseMulExpr();
  if (!L)
    return nullptr;

  while (Tok.isOneOf(tok::plus, tok::minus)) {
    BinaryExpr::OpKind K =
        Tok.is(tok::plus) ? BinaryExpr::OpKind::Add : BinaryExpr::OpKind::Sub;
    consumeToken();

    Expr *R = parseMulExpr();
    if (!R)
      return nullptr;

    L = Context.createBinaryExpr(SMRange(L->getLocation().Start,
                                         R->getLocation().End),
                                 L, K, R);
  }

  return L;
}

Expr *Parser::parseMulExpr() {
  Expr *L = parseUnaryExpr();
  if (!L)
    return nullptr;

  while (Tok.isOneOf(tok::star, tok::slashslash, tok::percent)) {
    BinaryExpr::OpKind K = BinaryExpr::OpKind::Mul;
    if (Tok.is(tok::slashslash))
      K = BinaryExpr::OpKind::FloorDiv;
    else if (Tok.is(tok::percent))
      K = BinaryExpr::OpKind::Mod;

    consumeToken();
    Expr *R = parseUnaryExpr();
    if (!R)
      return nullptr;

    L = Context.createBinaryExpr(SMRange(L->getLocation().Start,
                                         R->getLocation().End),
                                 L, K, R);
  }

  return L;
}

Expr *Parser::parseUnaryExpr() {
  if (Tok.is(tok::minus)) {
    SMRange OpLoc = Tok.getLocation();
    consumeToken();

    Expr *Operand = parseUnaryExpr();
    if (!Operand)
      return nullptr;

    return Context.createUnaryExpr(SMRange(OpLoc.Start, Operand->getLocation().End),
                                   UnaryExpr::OpKind::Minus, Operand);
  }

  return parsePostfixExpr();
}

Expr *Parser::parsePostfixExpr() {
  Expr *E = parsePrimaryExpr();
  if (!E)
    return nullptr;

  while (true) {
    if (Tok.is(tok::l_paren)) {
      SMLoc CallStart = E->getLocation().Start;
      consumeToken();

      ExprList Args;
      if (Tok.isNot(tok::r_paren)) {
        do {
          Expr *A = parseExpr();
          if (!A)
            return nullptr;
          Args.push_back(A);
        } while (consumeToken(tok::comma));
      }

      if (!expect(tok::r_paren))
        return nullptr;
      SMLoc End = Tok.getLocation().End;
      consumeToken();
      End = Tok.getLocation().Start;
      SMRange Loc(CallStart, End);
      if (MemberExpr *ME = dyn_cast<MemberExpr>(E))
        E = Context.createMethodCallExpr(Loc, ME, Args);
      else
        E = Context.createCallExpr(Loc, E, Args);
      continue;
    }

    if (Tok.is(tok::l_square)) {
      SMLoc Start = E->getLocation().Start;
      consumeToken();

      Expr *Index = parseExpr();
      if (!Index || !expect(tok::r_square))
        return nullptr;
      SMLoc End = Tok.getLocation().End;
      consumeToken();
      if (Tok.is(tok::NEWLINE))
        End = Tok.getLocation().Start;
      E = Context.createIndexExpr(SMRange(Start, End), E, Index);
      continue;
    }

    if (Tok.is(tok::period)) {
      SMLoc Start = E->getLocation().Start;
      consumeToken();

      if (!expect(tok::identifier))
        return nullptr;
      SMRange NameLoc = Tok.getLocation();
      SymbolInfo *Name = Tok.getSymbolInfo();
      consumeToken();

      DeclRef *Member = Context.createDeclRef(NameLoc, Name);
      E = Context.createMemberExpr(SMRange(Start, NameLoc.End), E, Member);
      continue;
    }

    break;
  }

  return E;
}

Expr *Parser::parsePrimaryExpr() {
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
    Expr *E = parseExpr();
    if (!E || !expectAndConsume(tok::r_paren))
      return nullptr;
    return E;
  }

  if (Tok.is(tok::l_square)) {
    SMLoc Start = Tok.getLocation().Start;
    consumeToken();

    ExprList Elts;
    if (Tok.isNot(tok::r_square)) {
      do {
        Expr *Elt = parseExpr();
        if (!Elt)
          return nullptr;
        Elts.push_back(Elt);
      } while (consumeToken(tok::comma));
    }

    if (!expect(tok::r_square))
      return nullptr;
    SMLoc End = Tok.getLocation().End;
    consumeToken();
    if (Tok.is(tok::NEWLINE))
      End = Tok.getLocation().Start;
    return Context.createListExpr(SMRange(Start, End), Elts);
  }

  if (Tok.isOneOf(tok::kw_None, tok::kw_True, tok::kw_False, tok::idstring,
                  tok::string_literal))
    return parseLiteral();

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
      if (expect(tok::r_square)) {
        SMLoc End = Tok.getLocation().End;
        consumeToken();
        Loc = SMRange(Loc.Start, End);
        return Context.createListType(Loc, T);
      }
    }
    return nullptr;
  }
  default:
    emitUnexpected();
    return nullptr;
  }
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

bool Parser::isAssignTarget(const Expr *E) const {
  return llvm::isa<DeclRef>(E) || llvm::isa<MemberExpr>(E) ||
         llvm::isa<IndexExpr>(E);
}
} // namespace chocopy
