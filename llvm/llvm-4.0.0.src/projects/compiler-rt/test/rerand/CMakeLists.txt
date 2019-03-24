set(RERAND_LIT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(RERAND_TESTSUITES)
set(RERAND_TEST_DEPS ${SANITIZER_COMMON_LIT_TEST_DEPS})

macro(add_rerand_testsuite test_mode sanitizer arch)
  set(RERAND_LIT_TEST_MODE "${test_mode}")
  set(CONFIG_NAME ${RERAND_LIT_TEST_MODE}-${arch})
  configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME}/lit.site.cfg)
  list(APPEND RERAND_TESTSUITES ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME})
  if(NOT COMPILER_RT_STANDALONE_BUILD)
    list(APPEND RERAND_TEST_DEPS ${sanitizer})
  endif()
endmacro()

set(RERAND_TEST_ARCH ${RERAND_SUPPORTED_ARCH})

foreach(arch ${RERAND_TEST_ARCH})
  set(RERAND_TEST_TARGET_ARCH ${arch})

  get_target_flags_for_arch(${arch} RERAND_TEST_TARGET_CFLAGS)
  string(REPLACE ";" " " RERAND_TEST_TARGET_CFLAGS "${RERAND_TEST_TARGET_CFLAGS}")

  add_rerand_testsuite("Standalone" rerand ${arch})
endforeach()

add_lit_testsuite(check-rerand "Running the Rerand tests"
  ${RERAND_TESTSUITES}
  DEPENDS ${RERAND_TEST_DEPS})
set_target_properties(check-rerand PROPERTIES FOLDER "Rerand tests")
