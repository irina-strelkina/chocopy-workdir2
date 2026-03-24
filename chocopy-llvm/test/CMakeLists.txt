add_subdirectory(parser)
add_subdirectory(sema)

add_custom_target(check-chpy)
add_dependencies(check-chpy
  check-chpy-parser
  check-chpy-sema)
