# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

x = y = z = 1
