# TinyChat Agent Guide
This file is for coding agents working in this repository.

## Rule Files Check
- Cursor rules: none found (`.cursor/rules/` and `.cursorrules`).
- Copilot rules: none found (`.github/copilot-instructions.md`).
- Architecture reference: `AGENT.md` (system-level design background).

## Repo Map
- `Backend/`: C++17 backend, CMake-based.
- `Backend/src/`: layered core code (`controller -> service -> repository/dao`).
- `Backend/servers/GateServer`: HTTP gateway server.
- `Backend/servers/StatusServer`: status/token service.
- `Backend/servers/ChatServer`: chat TCP + gRPC bridge.
- `Backend/servers/FileServer`: file upload/download gRPC service.
- `Backend/servers/VarifyServer`: Node.js verification service.
- `Backend/proto/`: protobuf contracts.
- `Backend/TestCases/`: standalone backend test binaries.
- `QTClient/`: Qt Widgets desktop client.
- `QTClient/test/`: QtTest suite (`tst_messagecache`).

## Build Commands
### Backend (full)
```bash
cmake -S Backend -B Backend/build
cmake --build Backend/build -j$(nproc)
```

### Backend (single target)
```bash
cmake --build Backend/build --target GateServer -j$(nproc)
cmake --build Backend/build --target ChatServer -j$(nproc)
cmake --build Backend/build --target FileServer -j$(nproc)
```

### Qt Client
```bash
qmake QTClient/QTClient.pro -o QTClient/Makefile
make -C QTClient -j$(nproc)
```

### Qt Tests Build
```bash
qmake QTClient/test/test.pro -o QTClient/test/Makefile
make -C QTClient/test -j$(nproc)
```

### VarifyServer
```bash
npm --prefix Backend/servers/VarifyServer install
npm --prefix Backend/servers/VarifyServer run serve
```

## Test Commands
There is no unified `ctest` in this repo.

### Backend tests (standalone executables)
```bash
./Backend/build/TestCases/Test
./Backend/build/TestCases/Test_db
./Backend/build/TestCases/Test_FileClient
./Backend/build/TestCases/UploadClient
./Backend/build/TestCases/DownloadClient
```

### Build + run a single backend test target
```bash
cmake --build Backend/build --target Test_db -j$(nproc)
./Backend/build/TestCases/Test_db
```

### Qt tests (full suite)
```bash
./QTClient/test/tst_messagecache
```

### Qt single test function (important)
```bash
./QTClient/test/tst_messagecache opensAndCreatesSchema
```

### Useful QtTest discovery/debug flags
```bash
./QTClient/test/tst_messagecache -functions
./QTClient/test/tst_messagecache -v2
```

## Lint / Format
No dedicated clang-tidy pipeline is configured.

### Format C/C++ files
```bash
find Backend -name "*.h" -o -name "*.hpp" -o -name "*.cpp" | xargs clang-format -i
find QTClient -name "*.h" -o -name "*.hpp" -o -name "*.cpp" | xargs clang-format -i
```

### Check one file formatting only
```bash
clang-format --dry-run --Werror Backend/src/controller/UserController.cpp
```

## Code Style Guidelines
### Language / Tooling
- Use C++17 for backend and Qt client code.
- Keep existing build systems: CMake for backend, qmake for Qt.
- Do not hand-edit generated gRPC/protobuf outputs unless regenerating intentionally.

### Includes / Imports
- Match nearby file include order first.
- Typical order: project headers, third-party headers, STL/Qt headers.
- Prefer explicit includes; avoid umbrella or unused headers.

### Formatting
- Run clang-format before finalizing changes.
- Repository style from `.clang-format`: 4 spaces, no tabs, ~80 columns.
- Preserve existing brace/wrapping patterns in touched files.

### Naming
- Types/classes/enums: `PascalCase`.
- Enum members: `UPPER_SNAKE_CASE`.
- Backend locals: usually `snake_case`.
- Qt members: often `_prefix` (`_uid`, `_icon`, `_chat_msgs`).
- Qt signals/slots: `sig_*` / `slot_*`.

### Types / Data Modeling
- Prefer `enum class` for protocol/error domains.
- Use fixed-width integer types on wire/protocol boundaries.
- Keep `const` correctness for params and locals.
- Backend service/repo flows commonly return `Result<T>` / `Result<void>`.
- Keep JSON/protobuf keys backward-compatible.

### Error Handling
- HTTP handlers: map failures to `HttpResponse::Error(...)`.
- Reuse centralized `ErrorCodes`; avoid one-off numeric codes.
- Always check `Result::IsOK()` before using `Value()`.
- Log operational issues with context (`LOG_ERROR`, `LOG_WARN`).
- Catch top-level exceptions in `main` and exit non-zero.
- Qt side: surface failures via signals and visible UI state.

### Architecture Boundaries
- Preserve layering: controller -> service -> repository/dao.
- Do not bypass repository/dao from controllers unless file pattern already does.
- Keep protocol constants centralized (`Backend/src/common/const.h`, `QTClient/global.h`).
- Avatar `icon` is a server-side name, not a local absolute path.

## Testing Expectations for Agents
- Prefer smallest meaningful verification first (single target or single test function).
- For backend changes: build affected server + relevant `TestCases` binary.
- For Qt message/cache changes: run `tst_messagecache` at minimum.
- Expand to broader build/tests only after targeted checks pass.

## Agent Workflow Guidance
- Check git working tree and avoid reverting unrelated user changes.
- Keep diffs focused; avoid opportunistic refactors.
- Favor minimal-risk changes, then validate with concrete commands.
- When uncertain about style, copy local patterns in the same file/module.
- Update this guide if repo structure or canonical commands change.
