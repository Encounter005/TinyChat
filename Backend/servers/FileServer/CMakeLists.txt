add_executable(FileServer
  main.cpp
  TaskQueue.h
  FileWorker.cpp
  FileWorkerPool.cpp
  FileServer.cpp
  UploadCallData.cpp
  DownloadCallData.cpp
  BackPressureManager.cpp
  const.h
  FileTransportServiceImpl.cpp
  FileIndexManager.cpp
  )
target_link_libraries(
  FileServer PRIVATE backend_core spdlog::spdlog ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES}
                     ${_GRPC_GRPCPP}  ${HIREDIS_LIBRARIES})
