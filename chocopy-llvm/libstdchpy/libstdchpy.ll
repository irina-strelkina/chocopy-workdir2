; C library declarations
declare void @abort() noreturn
declare i32 @fprintf(ptr noundef, ptr noundef, ...)
declare i32 @printf(ptr, ...)
declare void @exit(i32)

@stderr = external global ptr, align 8

; chocopy-stdlib
declare ptr @alloc(ptr)
declare void @Main()

%class.object = type <{
  i32,  ; Type tag
  i32,  ; Size
  ptr   ; Pointer to dispatch table
}>

%class.int = type <{
  %class.object,
  i32
}>

%class.bool = type <{
  %class.object,
  i1
}>

%class.str = type <{
  %class.object,
  i32
}>

@$fmt.int = private constant [4 x i8] c"%d\0A\00", align 1
@$fmt.bool.true = private constant [6 x i8] c"True\0A\00", align 1
@$fmt.bool.false = private constant [7 x i8] c"False\0A\00", align 1
@$fmt.str = private constant [4 x i8] c"%s\0A\00", align 1

@$fmt.err.print.unknown.tag = private constant [39 x i8] c"Try to print object with unknow type!\0A\00", align 1

define void @$print(ptr %obj) {
  %tag = load i32, ptr %obj
  switch i32 %tag, label %unknown.tag [ i32 0, label %unknown.tag
                                        i32 1, label %int.tag
                                        i32 2, label %bool.tag
                                        i32 3, label %str.tag ]
unknown.tag:
  call void (ptr) @$chpy$abort(ptr @$fmt.err.print.unknown.tag)
  unreachable

int.tag:
  call void(ptr) @$chpy.print.int(ptr %obj)
  br label %exit

bool.tag:
  call void(ptr) @$chpy.print.bool(ptr %obj)
  br label %exit

str.tag:
  call void(ptr) @$chpy.print.str(ptr %obj)
  br label %exit

exit:
  ret void
}

define void @$chpy.print.int(ptr %obj) {
  %int.ptr = getelementptr %class.int, ptr %obj, i64 0, i32 1
  %val = load i32, ptr %int.ptr
  call i32 (ptr, i32) @printf(ptr @$fmt.int, i32 %val)
  ret void
}

define void @$chpy.print.bool(ptr %obj) {
  %int.ptr = getelementptr %class.bool, ptr %obj, i64 0, i32 1
  %val = load i1, ptr %int.ptr

  %cond = icmp eq i1 %val, 1
  br i1 %cond, label %if.true, label %exit

if.true:
  call i32 (ptr) @printf(ptr @$fmt.bool.true)
  ret void

exit:
  call i32 (ptr) @printf(ptr @$fmt.bool.false)
  ret void
}

define void @$chpy.print.str(ptr %obj) {
  %str.ptr = getelementptr %class.str, ptr %obj, i64 1
  call i32 (ptr, ptr) @printf(ptr @$fmt.str, ptr %str.ptr)
  ret void
}

define void @$chpy$abort(ptr %err.msg) noreturn {
  %stderr.ptr = load ptr, ptr @stderr
  call i32 (ptr, ptr) @fprintf(ptr %stderr.ptr, ptr %err.msg)
  call void @abort()
  unreachable
}

declare ptr @"$alloc"(ptr %o)

@"$int.class.prototype" =  constant %class.int
  <{
      %class.object
      <{
          i32 1, i32 20, ptr null
      }>,
      i32 0
  }>

@"$bool.class.prototype" =  constant %class.bool
  <{
      %class.object
      <{
          i32 1, i32 20, ptr null
      }>,
      i1 0
  }>

@"$str.class.prototype" =  constant <{ %class.str, [1 x i8] }>
  <{
    %class.str
    <{
      %class.object <{ i32 3, i32 25, ptr null }>,
      i32 4
    }>,
    [1 x i8] c"\00"
  }>

define i32 @main() {
   call void @Main()
  ret i32 0
}
