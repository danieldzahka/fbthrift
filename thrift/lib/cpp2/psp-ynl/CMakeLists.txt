include(ExternalProject)

if(NOT DEFINED KSRC)
  message(FATAL_ERROR "KSRC is not set. Need kernel uapi to generate netlink code")
endif()

ExternalProject_Add(ynl
  PREFIX ynl
  GIT_REPOSITORY https://github.com/linux-netdev/ynl-c.git
  GIT_TAG de557a12b608e24c5a21ac7c90d4c45d9c3c0c9e
  CONFIGURE_COMMAND pushd <SOURCE_DIR> && ./update-from-kernel.sh ${KSRC} && popd
  BUILD_COMMAND pushd <SOURCE_DIR> && make && popd
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/ynl/src/ynl/libynl.a
)

add_library(psp_ynl psp_ynl.c)
add_dependencies(psp_ynl ynl)
target_link_libraries(psp_ynl PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/ynl/src/ynl/libynl.a)
target_include_directories(psp_ynl
  PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/ynl/src/ynl/generated
  ${CMAKE_CURRENT_BINARY_DIR}/ynl/src/ynl/include/ynl-c
  ${CMAKE_CURRENT_BINARY_DIR}/ynl/src/ynl/include/
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:>
)

set_target_properties(psp_ynl PROPERTIES LINKER_LANGUAGE C)
