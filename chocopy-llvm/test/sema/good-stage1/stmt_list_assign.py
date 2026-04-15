# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

x:[int] = None
y:[object] = None

x = [1, 2]
y = [None]
x[0] = 3
x[1] = y[0] = 4
