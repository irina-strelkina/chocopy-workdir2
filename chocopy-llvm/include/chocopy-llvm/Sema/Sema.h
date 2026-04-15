#ifndef CHOCOPY_LLVM_SEMA_SEMA_H
#define CHOCOPY_LLVM_SEMA_SEMA_H

#include "chocopy-llvm/AST/AST.h"
#include "chocopy-llvm/Sema/IdentifierResolver.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/SMLoc.h>

namespace chocopy {
class Scope;
class ASTContext;
class DiagnosticsEngine;
class Lexer;
class ValueType;
class ClassValueType;
class FuncType;

/// Sema - Класс реализует семантический анализатор для языка ChocoPy.
///
/// Выполняет семантический анализ AST после синтаксического разбора.
/// Основные задачи:
///   * Управление областями видимости и разрешение имён
///   * Проверка типов выражений, операторов и объявлений
///   * Обнаружение повторных объявлений и "затенения" имён классов
///   * Проверка наследования классов и переопределения методов
///   * Проверка операторов return (включая CFG-based анализ
///     достижимости для обнаружения paths без return)
///   * Проверка корректности объявлений `global` / `nonlocal`
///
/// ВАМ НЕОБХОДИМО:
///   * Реализовать все методы этого класса, отмеченные комментариями
///   "TODO" в файле lib/Sema/Sema.cpp.
///   * Ниже приведена структура класса с документацией.
///   * Сигнатуры методов менять не рекомендуется.
///
class Sema {
  class Analysis;

public:
  Sema(Lexer &L, ASTContext &C);

  /// Инициализация списка предопределённых идентификаторов.
  ///
  /// Регистрация в IdentifierResolver предопределённых классов
  /// (object, int, str, bool, NoneType) и встроенных функций
  /// (print, input, len).
  void initialize();

  /// Инициализация глобальной области видимости предопределёнными
  /// идентификаторами.
  void initializeGlobalScope();

public:
  DiagnosticsEngine &getDiagnosticEngine() const { return Diags; }

  /// Точка входа в семантический анализ. Запускает обход AST.
  void run();

private:
  std::shared_ptr<Scope> getGlobalScope() const { return GlobalScope; }
  void setGlobalScope(std::shared_ptr<Scope> S) { GlobalScope = std::move(S); }

  std::shared_ptr<Scope> getCurScope() const { return CurScope; }
  void setCurScope(std::shared_ptr<Scope> S) { CurScope = std::move(S); }

  // =================================================================
  //  Вспомогательные методы работы с областями видимости
  // =================================================================

  /// Зарегистрировать объявление в текущей области видимости
  /// и в IdentifierResolver. Вызывать после проверки на дубликаты.
  void addDeclaration(Declaration *D);

  /// Удалить все объявления области \p S из IdentifierResolver
  /// при выходе из области видимости.
  void actOnPopScope(Scope *S);

  // =================================================================
  //  Проверки (check-методы)
  //  Возвращают false при ошибке, при этом выводят диагностическое сообщение.
  // =================================================================

  /// checkDuplication - Проверяет, что объявление \p D не дублирует ранее
  /// объявленное имя в текущей области видимости.
  bool checkDuplication(Declaration *D);

  /// checkClassShadow - Проверяет, что объявление \p D не "затеняет" имя
  /// встроенного класса (object, int, str, bool, NoneType).
  bool checkClassShadow(Declaration *D);

  /// checkNonlocalDecl - Проверяет корректность `nonlocal`-объявления:
  /// именованная переменная должна существовать в области видимости
  /// enclosing-функции (но не в глобальной области).
  bool checkNonlocalDecl(NonLocalDecl *NLD);

  /// checkGlobalDecl - Проверяет корректность `global`-объявления:
  /// именованная переменная — это VarDef, объявленный на верхнем уровне.
  bool checkGlobalDecl(GlobalDecl *GD);

  /// checkSuperClass - Проверяет, что базовый класс \p D существует как
  /// ClassDef и не является специальным классом (int, str, bool).
  bool checkSuperClass(ClassDef *D);

  /// checkClassAttrs - Проверяет все атрибуты и методы класса:
  ///   * Первый параметр каждого метода — self типа класса-владельца.
  ///   * Ни один атрибут/метод не переопределяет имя из базового класса.
  ///   * Каждый переопределённый метод имеет совместимую сигнатуру.
  bool checkClassAttrs(ClassDef *D);

  /// checkMethodOverride - Проверяет, что переопределяющий метод \p OM имеет
  /// совместимую сигнатуру с переопределяемым методом \p M
  /// (совпадающее число параметров, совместимые типы параметров,
  /// совместимый возвращаемый тип).
  bool checkMethodOverride(FuncDef *OM, FuncDef *M);

  /// checkClassDef - Комплексная проверка класса (базовый класс + атрибуты).
  bool checkClassDef(ClassDef *D);

  /// checkFirstMethodParam - Проверяет, что первый параметр метода \p FD,
  /// объявленного внутри класса \p CD, имеет тип \p CD.
  bool checkFirstMethodParam(ClassDef *CD, FuncDef *FD);

  /// checkAssignTarget - Проверяет, что цели присваивания (левая часть) явно
  /// объявлены в текущей области видимости.
  bool checkAssignTarget(Expr *E);

  /// checkReturnStmt - Проверяет, что return не находится на верхнем уровне.
  bool checkReturnStmt(ReturnStmt *S);

  /// checkReturnMissing - Проверяет, что функция с не-void возвращаемым типом
  /// имеет return на КАЖДОМ пути исполнения (использует CFG).
  bool checkReturnMissing(FuncDef *F);

  /// checkTypeAnnotation - Проверяет, что все аннотации типов ссылаются на
  /// определённые классы. Рекурсивно проверяет типы внутри ListType.
  bool checkTypeAnnotation(TypeAnnotation *TA);

  // =================================================================
  //  Действия (actOn-методы)
  //  Выполняют вывод типов и дополнительную валидацию.
  // =================================================================

  /// actOnAssignStmt - Проверяет типы оператора присваивания:
  ///   * Проверяет, что тип правой части совместим по присваиванию
  ///     с типом каждой левой части.
  ///   * Для целей-DeclRef проверить наличие явного локального объявления.
  void actOnAssignStmt(AssignStmt *A);

  /// actOnReturnStmt - Проверяет тип значения return-оператора на совместимость
  /// с объявленным типом возвращаемого значения enclosing-функции.
  void actOnReturnStmt(ReturnStmt *R);

  /// actOnDeclaration - Выполняет диспетчеризацию: вызвать соответствующий
  /// обработчик для конкретного вида объявления.
  void actOnDeclaration(Declaration *D);

  /// actOnClassDef - Выполняет отложенные проверки определения класса после
  /// того, как все его члены зарегистрированы в области видимости.
  void actOnClassDef(ClassDef *C);

  /// actOnFuncDef - Выполняет отложенные проверки определения функции:
  ///   * Проверка наличия return на всех путях.
  void actOnFuncDef(FuncDef *F);

  /// actOnParamDecl - Зарегистрирует параметр функции и проверяет
  /// аннотацию его типа.
  void actOnParamDecl(ParamDecl *P);

  /// actOnVarDef - Регистрирует локальную переменную и проверяет
  /// совместимость типа инициализатора.
  void actOnVarDef(VarDef *V);

  /// actOnNonlocalDecl - Регистрирует nonlocal-объявление (после валидации).
  void actOnNonlocalDecl(NonLocalDecl *D);

  /// actOnGlobalDecl - Регистрирует global-объявление (после валидации).
  void actOnGlobalDecl(GlobalDecl *D);

  /// actOnBinaryExpr - Проверяет типы бинарного выражения:
  ///   * Проверяет допустимость типов операндов для оператора.
  ///   * Устанавливает выведенный тип рузультата выражения (inferred result
  ///   type).
  void actOnBinaryExpr(BinaryExpr *B);

  /// actOnIndexExpr - Проверяет индексного выражения:
  ///   * База должна быть list или str.
  ///   * Индекс должен быть int.
  void actOnIndexExpr(IndexExpr *I);

  /// actOnListExpr - Проверяет списки (list display); выводит тип элемента
  /// как join типов всех элементов.
  void actOnListExpr(ListExpr *L);

  /// actOnLiteral Устанавливает выводимый тип литерала в зависимости от его
  /// вида.
  void actOnLiteral(Literal *L);

  /// actOnCallExpr - Проверяет вызов функции или конструктор класса:
  ///   * Разрешить callee.
  ///   * Проверить количество и типы аргументов.
  void actOnCallExpr(CallExpr *CE);

  /// actOnDeclRef - Разрешает имя (DeclRef) — связывает с конкретным
  /// объявлением и выводит его тип.
  void actOnDeclRef(DeclRef *DR);

  /// actOnIfExpr - Выводит тип тернарного условного выражения как join
  /// типов ветвей then и else.
  void actOnIfExpr(IfExpr *I);

  /// actOnMemberExpr - Проверяет обращения к атрибуту: находит атрибут в классе
  /// объекта.
  void actOnMemberExpr(MemberExpr *M);

  /// actOnMethodCallExpr - Проверяет вызов метода: находит метод и проверяет
  /// типы аргументов.
  void actOnMethodCallExpr(MethodCallExpr *M);

  /// \brief Проверка унарного выражения (`-` или `not`):
  ///   * Проверить допустимость типа операнда для оператора.
  void actOnUnaryExpr(UnaryExpr *U);

  // =================================================================
  //  Вспомогательные lookup-методы.
  // =================================================================

  /// getScopeForDecl - Поднимается по цепочке областей видимости и нахоидт
  /// scope, содержащий объявление \p D.
  Scope *getScopeForDecl(Scope *S, Declaration *D);

  /// getSuperClass - Возвращает ClassDef для базового класса \p CD.
  ClassDef *getSuperClass(ClassDef *CD);

  /// isSameType - Проверяет структурное равенство двух аннотаций типов
  /// (совпадение имён классов для ClassType, рекурсивное сравнение
  /// элементов для ListType).
  bool isSameType(TypeAnnotation *TyA, TypeAnnotation *TyB);

  /// convertATypeToVType - Преобразовывает TypeAnnotation (из исходного текста)
  /// в соответствующий ValueType (внутреннее представление).
  ValueType *convertATypeToVType(TypeAnnotation *TA);

  /// getFuncType - Строит FuncType из параметров и типа возврата FuncDef.
  FuncType *getFuncType(FuncDef *FD);

  /// lookupName - Нахоидмт имя в области видимости \p S.
  Declaration *lookupName(Scope *S, SymbolInfo *SI);

  /// lookupDecl - Разрешает DeclRef — связывает его с конкретным объявлением.
  Declaration *lookupDecl(DeclRef *DR);

  /// lookupDecl - Нахолит объявление по SymbolInfo через IdentifierResolver.
  Declaration *lookupDecl(SymbolInfo *SI);

  /// lookupMember - Находит атрибут (VarDef) по имени в классе \p CD,
  /// поднимаясь по иерархии наследования.
  VarDef *lookupMember(ClassDef *CD, DeclRef *Member);

  /// lookupMember - Перегрузка lookupMember для ClassValueType.
  VarDef *lookupMember(ClassValueType *CVT, DeclRef *Member);

  /// lookupMethod - Находит метод (FuncDef) по DeclRef в классе \p CD,
  /// поднимаясь по иерархии наследования.
  FuncDef *lookupMethod(ClassDef *CD, DeclRef *Member);

  /// lookupMethod - Находит метод по имени в классе \p CD.
  FuncDef *lookupMethod(ClassDef *CD, StringRef Name);

  /// lookupMethod - Перегрузка lookupMethod для ClassValueType.
  FuncDef *lookupMethod(ClassValueType *CVT, DeclRef *Member);

  /// resolveClassType - Разрешает ClassType в его ClassDef-объявление.
  ClassDef *resolveClassType(ClassType *CT);

  /// getClassHierarchy - Строит иерархию наследования для \p CVT
  /// (от наиболее производного до object).
  void getClassHierarchy(const ClassValueType *CVT,
                         SmallVector<ClassDef *> &Hierarchy);

  /// join - Вычисляет наименьший общий предковый тип (join) для двух
  /// ValueTypes в иерархии наследования.
  ValueType *join(ValueType *LTy, ValueType *RTy);

  /// isAssignementCompatibility - Проверяет типы на совместимость по присваиванию.
  bool isAssignementCompatibility(const ValueType *Sub, const ValueType *Sup);

private:
  using Nonlocals = SmallVector<NonLocalDecl *, 4>;
  using Globals = SmallVector<GlobalDecl *, 4>;

private:
  Lexer &TheLexer;
  DiagnosticsEngine &Diags;
  ASTContext &Ctx;
  std::shared_ptr<Scope> GlobalScope;
  std::shared_ptr<Scope> CurScope;
  IdentifierResolver IdResolver;
  FuncDef *CurFuncDef;
};
} // namespace chocopy
#endif // CHOCOPY_LLVM_SEMA_SEMA_H
