# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

x:int = 1

x - 1
