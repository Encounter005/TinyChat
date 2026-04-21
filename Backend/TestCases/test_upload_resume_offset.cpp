#include "repository/FileRepository.h"

#include <cassert>

int main() {
    const int64_t block_size = FileRepository::RESUME_BLOCK_SIZE;

    assert(FileRepository::CompletedBlockCountToOffsetBytes(0, block_size) == 0);
    assert(FileRepository::CompletedBlockCountToOffsetBytes(1, block_size) == block_size);
    assert(
        FileRepository::CompletedBlockCountToOffsetBytes(3, block_size)
        == 3 * block_size);

    return 0;
}
