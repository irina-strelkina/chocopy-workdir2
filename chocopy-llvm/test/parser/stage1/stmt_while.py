# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

while True:
    pass
