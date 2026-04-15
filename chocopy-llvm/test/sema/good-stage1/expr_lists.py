# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

x:[int] = None
y:[object] = None
z:[bool] = None
o:object = None

x = [1, 2, 3]
x = []
y = [1, True]
z = [False, True]
x = None
o = x
o = x = [1]
