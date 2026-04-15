# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

x:int = 0

for x in [1, 2, 3]:
    x + 1
