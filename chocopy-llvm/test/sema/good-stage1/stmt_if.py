# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

if False:
    pass
elif True:
    if 1 == 1:
        pass
else:
    pass
