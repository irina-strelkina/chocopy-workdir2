# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

def foo(x:str, y:bool) -> int:
    return bar()

def bar() -> int:
    return 1

foo("Hello", False)
