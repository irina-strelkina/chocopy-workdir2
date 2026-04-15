# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

1 + 2 * 3 > 13 // 3 % 2 or 1 != 1 and False == False

