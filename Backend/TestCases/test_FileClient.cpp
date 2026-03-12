#include "grpcClient/FileClient.h"
#include "repository/UserRepository.h"
#include <iostream>
#include <vector>

int main(int argc, char *argv[]) {
    auto res = UserRepository::UpdateUserIcon(1062, "avatar_1001_xxx.png");
    assert(res.IsOK());
    return 0;
}
