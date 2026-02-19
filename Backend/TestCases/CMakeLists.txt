# ============================================================================
# Test Executables
# ============================================================================
message(STATUS "[Configuring] Test Programs")

# ============================================================================
# Test Executable
# ============================================================================
message(STATUS "[Target]      Test (test client)")
add_executable(Test test.cpp)

target_link_libraries(Test
    PRIVATE
        backend_core
        ${HIREDIS_LIBRARIES}
        ${Boost_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        ${_GRPC_GRPCPP}
)

# ============================================================================
# Upload Client Executable (with resumable upload support)
# ============================================================================
message(STATUS "[Target]      UploadClient (file upload test utility)")
add_executable(UploadClient UploadClient.cpp)

target_link_libraries(UploadClient
    PRIVATE
        backend_core
        ${HIREDIS_LIBRARIES}
        ${Boost_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        ${_GRPC_GRPCPP}
        ${OPENSSL_LIBRARIES}
)

# ============================================================================
# Download Client Executable (NEW)
# ============================================================================
message(STATUS "[Target]      DownloadClient (file download test utility)")
add_executable(DownloadClient DownloadClient.cpp)

target_link_libraries(DownloadClient
    PRIVATE
        backend_core
        ${HIREDIS_LIBRARIES}
        ${Boost_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        ${_GRPC_GRPCPP}
        ${OPENSSL_LIBRARIES}
)

# ============================================================================
# Build Information
# ============================================================================

message(STATUS "")
message(STATUS "=========================================================================")
message(STATUS "Test Programs")
message(STATUS "=========================================================================")
message(STATUS "  Executable:         UploadClient")
message(STATUS "  Description:       File upload client with resumable upload support")
message(STATUS "  Linked Libraries:   backend_core, Hiredis, Boost, JSONCpp, gRPC, OpenSSL")
message(STATUS "")
message(STATUS "  Executable:         DownloadClient")
message(STATUS "  Description:       File download client with resumable download support")
message(STATUS "  Linked Libraries:   backend_core, Hiredis, Boost, JSONCpp, gRPC, OpenSSL")
message(STATUS "=========================================================================")
message(STATUS "")
