# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

class A(object):
    x:int = 1 # OK

    def foo(self: "A") -> int: # OK
        return 0

    x:int = 1 # Duplicate

    def foo(self: "A") -> int: # Duplicate
        return 0

A()
