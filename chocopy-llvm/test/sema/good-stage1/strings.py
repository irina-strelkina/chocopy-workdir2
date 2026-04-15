# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

x:str = "Hello"
y:str = "World"
z:str = ""

z = x + y
z = x[0]
x = y = z

