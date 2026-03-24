set(CHOCOPY_TOOLS_BINARY_DIR ${CHOCOPY_BINARY_DIR}/bin)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/bad/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/bad/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/bad/lit.cfg.py
)

add_lit_testsuite(check-chpy-sema-bad "Running the Chocopy bad sema tests"
  ${CMAKE_CURRENT_BINARY_DIR}/bad
)

add_custom_target(check-chpy-sema)
add_dependencies(check-chpy-sema
  check-chpy-sema-bad)
