# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

1 + 2 + 3
