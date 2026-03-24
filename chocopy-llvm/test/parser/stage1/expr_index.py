# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

a + b[i][j]
