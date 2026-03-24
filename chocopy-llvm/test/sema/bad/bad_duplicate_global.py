# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

x:int = 1

def foo() -> object:
    pass

# `foo` cannot be redefined in the same scope
def foo() -> object:
    pass

# `print` cannot be redefined in the same scope
def print(val:object) -> object:
    pass

# `x` cannot be redefined in the same scope
x:int = 2


foo()
