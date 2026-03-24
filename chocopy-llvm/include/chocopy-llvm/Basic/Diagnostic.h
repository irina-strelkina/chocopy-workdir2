#ifndef CHOCOPY_LLVM_BASIC_DIAGNOSTIC_H
#define CHOCOPY_LLVM_BASIC_DIAGNOSTIC_H

#include "chocopy-llvm/Basic/LLVM.h"

#include <llvm/ADT/Twine.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SourceMgr.h>

namespace chocopy {
using llvm::SMLoc;
using llvm::SMRange;
using llvm::SourceMgr;

class DiagnosticsEngine;
class InFlightDiagnostic;

namespace diag {
enum {
#define DIAG(ID, LEVEL, MSG) ID,
#include "chocopy-llvm/Basic/DiagnosticKinds.def"
};
} // namespace diag

class Diagnostic {
public:
  Diagnostic(SourceMgr::DiagKind Kind, SMLoc Loc, const llvm::Twine &Msg)
      : Location(Loc), Kind(Kind) {
        llvm::SmallVector<char> Str;
        Message = Msg.toNullTerminatedStringRef(Str);
      }

  const std::string &getMessage() const { return Message; }
  SMLoc getLocation() const { return Location; }
  SourceMgr::DiagKind getKind() const { return Kind; }

private:
  std::string Message;
  SMLoc Location;
  SourceMgr::DiagKind Kind;
};

class DiagnosticConsumer {
public:
  virtual ~DiagnosticConsumer() = default;

  virtual void handleDiagnostic(const Diagnostic &Diag) = 0;
};

class DiagnosticsEngine {
public:
  DiagnosticsEngine(DiagnosticConsumer *Client) : Client(Client) {}

  unsigned getNumErrors() const { return NumErrors; }

  InFlightDiagnostic emitError(SMLoc Loc, unsigned DiagId);
  InFlightDiagnostic emitWarning(SMLoc Loc, unsigned DiagId);
  void report(SourceMgr::DiagKind Kind, SMLoc Loc, StringRef Msg);

private:
  DiagnosticConsumer *Client;
  unsigned NumWarnings = 0;
  unsigned NumErrors = 0;
};

class InFlightDiagnostic {
public:
  InFlightDiagnostic(DiagnosticsEngine *DiagEngine, SourceMgr::DiagKind Kind,
                     SMLoc Loc, unsigned DiagId)
      : DiagEngine(DiagEngine), Kind(Kind), Location(Loc), DiagId(DiagId) {}

  ~InFlightDiagnostic() { emit(); }

  InFlightDiagnostic &&operator<<(StringRef Arg) {
    NumArgs++;
    Args.emplace_back(Arg);
    return std::move(*this);
  }

private:
  void emit();

private:
  static constexpr const int MaxNumArgs = 10;

private:
  template <std::size_t... Idx>
  std::string formatMessage(std::index_sequence<Idx...>, StringRef Format) {
    return std::string(llvm::formatv(Format.data(), Args[Idx]...));
  }

private:
  DiagnosticsEngine *DiagEngine;
  SmallVector<std::string, MaxNumArgs> Args;
  SourceMgr::DiagKind Kind;
  SMLoc Location;
  unsigned DiagId;
  unsigned NumArgs = 0;
};

} // namespace chocopy

#endif // CHOCOPY_LLVM_BASIC_DIAGNOSTIC_H
