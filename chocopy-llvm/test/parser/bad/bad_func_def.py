# RUN: %chocopy-llvm %s 2>&1 | FileCheck %s.err

def foo(a, b) -> 1:
    x:int = a
    return 1

print(1)
print(3**6)
