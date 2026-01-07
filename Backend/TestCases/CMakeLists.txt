add_executable(Test test.cpp)
target_link_libraries(Test PRIVATE backend_core ${HIREDIS_LIBRARIES} ${Boost_LIBRARIES}
                                   ${JSONCPP_LIBRARIES} ${_GRPC_GRPCPP} )
