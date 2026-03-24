# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

x:int = 0
for x in [1, 2, 3]:
    print(x)
