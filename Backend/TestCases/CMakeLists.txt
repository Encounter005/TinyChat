add_executable(Test test.cpp)
target_link_libraries(Test PRIVATE backend_core ${HIREDIS_LIBRARIES} ${Boost_LIBRARIES}
                                   ${JSONCPP_LIBRARIES} ${_GRPC_GRPCPP} )

add_executable(UploadClient UploadClient.cpp)
target_link_libraries(UploadClient PRIVATE backend_core ${HIREDIS_LIBRARIES} ${Boost_LIBRARIES}
                                   ${JSONCPP_LIBRARIES} ${_GRPC_GRPCPP} )
