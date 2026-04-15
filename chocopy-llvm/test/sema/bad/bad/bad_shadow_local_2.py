# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

def foo(x:int) -> object:
    A:int = 5       # Invalid name
    pass

class A(object):
    x:int = 1

pass
