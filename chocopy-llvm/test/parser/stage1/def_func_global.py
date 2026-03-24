# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

z:int = 0

def foo(x:int) -> bool:
    global z
    return x > z

foo(1)
