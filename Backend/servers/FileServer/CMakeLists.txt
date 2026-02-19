# ============================================================================
# FileServer - File Transfer Service
# ============================================================================
# FileServer handles file upload and download operations for the TinyChat
# system. It manages file storage, transmission, and provides back-pressure
# control for efficient resource management.
#
# Responsibilities:
#   - File upload handling
#   - File download serving
#   - File storage and indexing
#   - Back-pressure management
#   - Worker pool management for concurrent file operations
#   - Breakpoint-based resumable upload/download (NEW)
# ============================================================================

message(STATUS "[Target]      FileServer (file transfer server with resumable support)")

# Create the FileServer executable with all source files
# Note: Header files (.h) should not be listed in add_executable
add_executable(FileServer
    main.cpp
    FileWorker.cpp
    FileWorkerPool.cpp
    FileServer.cpp
    UploadCallData.cpp
    DownloadCallData.cpp
    QueryUploadCallData.cpp      # 新增
    QueryDownloadCallData.cpp    # 新增
    BackPressureManager.cpp
    FileIndexManager.cpp
)

# ============================================================================
# Link Libraries
# ============================================================================

# Link libraries required by FileServer
target_link_libraries(FileServer
    PRIVATE
        backend_core         # Core business logic library
        spdlog::spdlog       # Logging framework
        ${Boost_LIBRARIES}   # Boost libraries (filesystem, etc.)
        ${JSONCPP_LIBRARIES} # JSON parsing and generation
        ${_GRPC_GRPCPP}      # gRPC C++ library
        ${HIREDIS_LIBRARIES} # Redis client library
        OpenSSL::Crypto      # OpenSSL for MD5 calculation (新增)
)

# ============================================================================
# Build Information
# ============================================================================

message(STATUS "  Features:           Resumable upload/download, back-pressure control")
message(STATUS "")
