# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

def foo(x:int, y:int) -> bool:
    return x > y

foo(1,2)
