# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

x:A = None

def foo(x:B) -> C:
    y:D = None
    return

pass
