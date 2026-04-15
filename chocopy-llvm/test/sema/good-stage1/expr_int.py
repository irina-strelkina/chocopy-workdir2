# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

6 * 9
