message(STATUS "[Target]      AiServer (gRPC ai bridge server)")

add_executable(AiServer main.cpp AiServiceImpl.cpp VaneClient.cpp)

target_link_libraries(
  AiServer PRIVATE backend_core spdlog::spdlog ${Boost_LIBRARIES}
                   ${JSONCPP_LIBRARIES} ${_GRPC_GRPCPP} ${HIREDIS_LIBRARIES})
