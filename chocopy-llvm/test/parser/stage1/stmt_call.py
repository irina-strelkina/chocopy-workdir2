# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

print(1)
