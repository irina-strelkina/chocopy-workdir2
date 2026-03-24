# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

if True:
    False
else:
    True
