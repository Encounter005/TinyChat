#!/usr/bin/env sh

# gRPC 代码生成脚本
# 用法: ./generate_grpc.sh

set -e

PROTO_FILE="./file.proto"
OUTPUT_DIR="./"

echo "=== Generating gRPC files for $PROTO_FILE ==="

# 检查 protoc
if ! command -v protoc &> /dev/null; then
    echo "Error: protoc not found"
    exit 1
fi

# 检查 grpc_cpp_plugin
GRPC_PLUGIN=$(which grpc_cpp_plugin 2>/dev/null || echo "")
if [ -z "$GRPC_PLUGIN" ]; then
    # 尝试常见路径
    for path in /usr/local/bin/grpc_cpp_plugin /usr/bin/grpc_cpp_plugin; do
        if [ -f "$path" ]; then
            GRPC_PLUGIN="$path"
            break
        fi
    done
fi

if [ -z "$GRPC_PLUGIN" ]; then
    echo "Error: grpc_cpp_plugin not found"
    exit 1
fi

echo "Using protoc: $(which protoc)"
echo "Using grpc_cpp_plugin: $GRPC_PLUGIN"

# 生成 protobuf 和 gRPC 代码
protoc --cpp_out="${OUTPUT_DIR}" \
       --grpc_out="${OUTPUT_DIR}" \
       --plugin=protoc-gen-grpc="${GRPC_PLUGIN}" \
       --proto_path=. \
       "$PROTO_FILE"

echo "=== Generation complete ==="
echo "Output files:"
ls -la "$OUTPUT_DIR"/file.*.cc "$OUTPUT_DIR"/file.*.h 2>/dev/null || true
