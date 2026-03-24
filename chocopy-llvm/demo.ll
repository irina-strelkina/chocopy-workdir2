; ModuleID = 'test/demo/demo.py'
source_filename = "test/demo/demo.py"

@"$int.class.prototype" = external externally_initialized constant <{ <{ i32, i32, ptr }>, i32 }>
@"$bool.class.prototype" = external externally_initialized constant <{ <{ i32, i32, ptr }>, i1 }>
@"$str.class.prototype" = external externally_initialized constant <{ <{ <{ i32, i32, ptr }>, i32 }>, [1 x i8] }>
@"$a" = private global i32 0
@"$c" = private global i32 11

declare void @"$abort"(ptr)

declare void @"$print"(ptr)

declare ptr @"$alloc"(ptr)

define void @Main() {
Entry:
  store i32 220, ptr @"$c", align 4
  %0 = call ptr @"$alloc"(ptr @"$int.class.prototype")
  %1 = getelementptr inbounds nuw <{ <{ i32, i32, ptr }>, i32 }>, ptr %0, i32 0, i32 1
  %2 = load i32, ptr @"$c", align 4
  store i32 %2, ptr %1, align 4
  call void @"$print"(ptr %0)
  store i32 123, ptr @"$a", align 4
  %3 = call ptr @"$alloc"(ptr @"$int.class.prototype")
  %4 = getelementptr inbounds nuw <{ <{ i32, i32, ptr }>, i32 }>, ptr %3, i32 0, i32 1
  %5 = load i32, ptr @"$a", align 4
  store i32 %5, ptr %4, align 4
  call void @"$print"(ptr %3)
  %6 = load i32, ptr @"$a", align 4
  %7 = add i32 %6, 2
  %8 = load i32, ptr @"$a", align 4
  %9 = load i32, ptr @"$a", align 4
  %10 = mul i32 %8, %9
  %11 = mul i32 %10, 100
  %12 = add i32 %7, %11
  %13 = sub i32 %12, 50
  store i32 %13, ptr @"$a", align 4
  %14 = call ptr @"$alloc"(ptr @"$int.class.prototype")
  %15 = getelementptr inbounds nuw <{ <{ i32, i32, ptr }>, i32 }>, ptr %14, i32 0, i32 1
  %16 = load i32, ptr @"$a", align 4
  store i32 %16, ptr %15, align 4
  call void @"$print"(ptr %14)
  ret void
}
