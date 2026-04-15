# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

def outer() -> int:
    def inner() -> int:
        nonlocal x
        x = 1
        return x
    x:int = 0
    inner()
    return x

print(outer())
