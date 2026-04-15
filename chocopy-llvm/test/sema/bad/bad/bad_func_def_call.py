# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

def foo(x:str, y:bool) -> int:
    return bar()

def bar() -> int:
    return 1

# All of the below are bad calls
foo("Hello")
foo("Hello", False, 3)
foo("Hello", 3)
foo(1, "Hello")
baz()

