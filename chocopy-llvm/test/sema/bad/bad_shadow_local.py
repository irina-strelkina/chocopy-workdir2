# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

class A(object):
    x:int = 1

def foo(x:int, bool:str) -> object: # Second param uses an invalid name
    y:int = 4       # OK
    A:int = 5       # Invalid name
    object:str = "" # Invalid name

    def str() -> bool: # Invalid name
        return False

    pass

pass
