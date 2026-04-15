# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

x:int = 0
while x < 100:
    x = x + 1
