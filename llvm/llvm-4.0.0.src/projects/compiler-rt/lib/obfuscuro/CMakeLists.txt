add_compiler_rt_component(obfuscuro)

set(OBFUSCURO_SOURCES
  init.cc
  rtl.cc
  oram_init.cc
  oram_ops.cc
  oram_util.cc
  oram_cmov.cc
  oram_reg.cc
  debug.cc
  )

include_directories(..)

set(OBFUSCURO_CFLAGS
  ${SANITIZER_COMMON_CFLAGS})
append_rtti_flag(OFF OBFUSCURO_CFLAGS)

foreach(arch ${OBFUSCURO_SUPPORTED_ARCH})
  add_compiler_rt_runtime(clang_rt.obfuscuro
    STATIC
    ARCHS ${arch}
    SOURCES ${OBFUSCURO_SOURCES}
    $<TARGET_OBJECTS:RTInterception.${arch}>
    $<TARGET_OBJECTS:RTSanitizerCommon.${arch}>
    $<TARGET_OBJECTS:RTSanitizerCommonNoLibc.${arch}>
    CFLAGS ${OBFUSCURO_CFLAGS}
    PARENT_TARGET obfuscuro)
  add_sanitizer_rt_symbols(clang_rt.obfuscuro
    ARCHS ${arch}
    EXTRA obfuscuro.syms.extra)
endforeach()
