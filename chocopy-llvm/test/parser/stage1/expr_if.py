# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

3 if 1 > 2 else 4
