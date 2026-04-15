# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

3 if 1 > 2 else 4
