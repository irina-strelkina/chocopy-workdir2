# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

[1, 2] + [3, 4]
"Hello " + "World"
[1, 2] + [True]
