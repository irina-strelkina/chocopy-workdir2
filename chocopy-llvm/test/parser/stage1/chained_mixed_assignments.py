# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

x[0] = y = z.f = 1
z.g = y = x[0]
