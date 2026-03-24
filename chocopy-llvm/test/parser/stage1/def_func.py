# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

def foo() -> int:
    return 1

foo()
