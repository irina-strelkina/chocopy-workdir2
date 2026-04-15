# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

def foo(x:str, y:bool) -> int:
    return None

def bar() -> bool:
    return 1

def baz() -> str:
    return

foo("Hello", False)
