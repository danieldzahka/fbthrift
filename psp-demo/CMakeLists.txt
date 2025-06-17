thrift_library(
  "psp"
  "EchoService"
  "cpp2"
  "" #options
  "${CMAKE_CURRENT_SOURCE_DIR}" #file_path
  "${CMAKE_CURRENT_BINARY_DIR}" #output_path
  ".."
)

add_executable(pspserver server.cpp)
target_link_libraries(pspserver
  PRIVATE
  thriftcpp2
  psp-cpp2
  ${LIBGFLAGS_LIBRARY}
  ${GLOG_LIBRARIES}
)

add_executable(pspclient client.cpp)
target_link_libraries(pspclient
  PRIVATE
  thriftcpp2
  psp-cpp2
  ${LIBGFLAGS_LIBRARY}
  ${GLOG_LIBRARIES}
)
