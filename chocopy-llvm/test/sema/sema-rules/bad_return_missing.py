# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

def foo() -> int:
    if True:
        return 1
    # Bad: all paths MUST return int

foo()
