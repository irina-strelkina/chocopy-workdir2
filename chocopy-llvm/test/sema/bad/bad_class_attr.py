# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

class A(object):
    x:int = 1

    def foo(self:"A") -> int:
        return 0

class B(A):
    x:int = 2  # Bad
    foo:str = "" # Bad

A()


