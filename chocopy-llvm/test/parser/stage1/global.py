# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

x:int = 1

x = 2

print(x)
