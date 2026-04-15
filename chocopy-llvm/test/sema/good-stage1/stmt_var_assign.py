# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

x:int = 1
y:bool = True
z:str = ""
o:object = None

x = 2
y = False
z = "Hello"
o = z
