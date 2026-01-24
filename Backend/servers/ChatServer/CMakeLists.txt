add_executable(ChatServer
  main.cpp
  dispatcher.cpp
  session.cpp
  SessionManager.cpp
  ChatServiceImpl.cpp
  UserManager.cpp
  LogicHandler.cpp
  ChatServer.cpp)
target_link_libraries(
  ChatServer PRIVATE backend_core spdlog::spdlog ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES}
                     ${_GRPC_GRPCPP}  ${HIREDIS_LIBRARIES})
