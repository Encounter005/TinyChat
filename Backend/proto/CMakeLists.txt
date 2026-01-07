# 获取编译器
find_program(PROTOC_EXECUTABLE protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)
# 需要编译的 proto 文件
file(GLOB PROTO_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

set(PROTO_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROTO_CXX_OUT "${CMAKE_CURRENT_BINARY_DIR}/gen_cxx")

file(MAKE_DIRECTORY ${PROTO_CXX_OUT})

# 使用 protoc 处理 proto 文件
foreach(input_proto ${PROTO_SOURCE_FILES})
  get_filename_component(DIR ${input_proto} DIRECTORY)
  get_filename_component(FILE_NAME ${input_proto} NAME_WE)

  set(OUTPUT_CXX_HEADER "${PROTO_CXX_OUT}/${FILE_NAME}.pb.h")
  set(OUTPUT_CXX_SOURCE "${PROTO_CXX_OUT}/${FILE_NAME}.pb.cc")
  set(OUTPUT_GRPC_CXX_HEADER "${PROTO_CXX_OUT}/${FILE_NAME}.grpc.pb.h")
  set(OUTPUT_GRPC_CXX_SOURCE "${PROTO_CXX_OUT}/${FILE_NAME}.grpc.pb.cc")

  list(APPEND OUTPUT_SOURCES_CXX ${OUTPUT_CXX_HEADER} ${OUTPUT_CXX_SOURCE}
       ${OUTPUT_GRPC_CXX_HEADER} ${OUTPUT_GRPC_CXX_SOURCE})
endforeach()

add_custom_command(
  OUTPUT ${OUTPUT_SOURCES_CXX}
  COMMAND
    ${PROTOC_EXECUTABLE} ARGS --cpp_out=${PROTO_CXX_OUT}
    --grpc_out=${PROTO_CXX_OUT}
    # 使用之前找到的执行文件路径，而不是 shell 的 which
    --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
    --proto_path=${PROTO_PATH} ${PROTO_SOURCE_FILES}
  DEPENDS ${PROTO_SOURCE_FILES}
  WORKING_DIRECTORY ${PROTO_PATH}
  COMMENT "Generating C++ and gRPC files from ${PROTO_SOURCE_FILES}"
  VERBATIM # 建议加上，处理参数中的特殊字符
)

add_custom_target(compile_protos DEPENDS ${OUTPUT_SOURCES_CXX})

# 设置生成源文件包含目录变量供上层引用
set(PROTO_GEN_INCLUDE_DIRS
    ${PROTO_CXX_OUT}
    PARENT_SCOPE)

# 将生成的文件打包为库 proto_gen 程序可以链接到该库
add_library(proto_gen STATIC ${OUTPUT_SOURCES_CXX})
target_link_libraries(proto_gen protobuf)
target_include_directories(proto_gen PUBLIC ${PROTO_CXX_OUT})
add_dependencies(proto_gen compile_protos)
