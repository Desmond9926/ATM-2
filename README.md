# ATM 银行模拟系统

## 项目简介

本项目是一个基于 `C++ + MySQL + TCP` 的 ATM 银行模拟系统，运行于 Windows 平台，采用客户端/服务端架构。

系统由以下部分组成：

- `client/`：控制台客户端
- `server/`：业务服务端
- `common/`：客户端与服务端共享的协议和数据结构
- `sql/`：数据库初始化脚本
- `scripts/`：自动化联调测试脚本

项目重点在于完整打通 ATM 核心业务流程，包括鉴权、资金操作、流水记录、管理员功能和联调测试。

## 功能列表

- 用户注册与开户
- 用户登录与退出
- 余额查询
- 存款
- 取款
- 转账
- 修改密码
- 交易流水查询
- 密码采用 `hash + salt` 存储
- 连续输错密码 3 次自动锁定
- 管理员查看用户、解锁账户、重置密码、查询流水
- 用户执行查询余额、存款、取款、转账前需要再次输入密码验证

## 项目结构

```text
ATM-2/
├─ client/     控制台客户端
├─ server/     服务端业务逻辑
├─ common/     共享协议与模型
├─ sql/        数据库脚本
├─ scripts/    自动化测试脚本
└─ docs/       设计与演示文档
```

## 运行环境

- Windows
- MySQL 8.0
- Visual Studio 或 CMake + MinGW/Ninja

## 数据库初始化

先确保 MySQL 服务已经启动，然后依次执行：

1. `sql/schema.sql`
2. `sql/seed.sql`

默认演示账号：

- 管理员：`admin001 / admin123`
- 普通用户1：`62220001 / 123456`
- 普通用户2：`62220002 / 123456`

## 构建说明

### 使用 Visual Studio

1. 在 Windows 下使用 Visual Studio 打开项目。
2. 将 `client`、`server`、`common` 中的源文件加入工程。
3. 配置 MySQL C API 的头文件和库目录。
4. 链接以下库：

- `ws2_32.lib`
- `bcrypt.lib`
- `libmysql.lib`

当前项目默认使用的本机 MySQL 路径：

- 头文件目录：`C:\Program Files\MySQL\MySQL Server 8.0\include`
- 库文件：`C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.lib`
- 运行时 DLL：`C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.dll`

### 使用 CMake

在项目根目录执行：

```powershell
cmake --preset msvc-debug
cmake --build --preset build-msvc-debug
```

构建完成后会生成：

- `build/msvc/Debug/atm_server.exe`
- `build/msvc/Debug/atm_client.exe`

并且构建流程会自动将 `libmysql.dll` 复制到输出目录，避免服务端手动启动时报缺少 DLL。

### 使用 VS Code

- 安装 `C/C++` 和 `CMake Tools` 扩展
- 直接打开项目根目录，VS Code 会默认使用 `msvc-debug` 预设
- IntelliSense 通过 `CMake Tools` 同步 MSVC 的头文件、宏和编译选项

## 启动方式

### 1. 启动服务端

```powershell
.\build\msvc\Debug\atm_server.exe
```

正常输出示例：

```text
ATM server started at 127.0.0.1:8888
```

### 2. 启动客户端

```powershell
.\build\msvc\Debug\atm_client.exe
```

客户端启动后会显示主菜单。

如果之前用过 MinGW/Ninja 打开过本项目，`build/` 根目录下可能还残留旧缓存；这不会影响新的 MSVC 预设构建，但如果 VS Code 仍显示旧工具链信息，删除旧缓存后重新打开工作区即可。

## 运行配置

服务端支持通过环境变量覆盖数据库与监听配置：

- `ATM_DB_HOST`
- `ATM_DB_PORT`
- `ATM_DB_USER`
- `ATM_DB_PASSWORD`
- `ATM_DB_SCHEMA`
- `ATM_SERVER_HOST`
- `ATM_SERVER_PORT`

如果未设置环境变量，服务端会使用 `server/ServerMain.cpp` 中的默认值。

## 编码说明

客户端和服务端启动时会自动将 Windows 控制台切换到 UTF-8，以减少中文乱码问题。

建议使用以下终端运行：

- Windows Terminal
- 新版 PowerShell

## 客户端使用说明

### 主菜单

客户端启动后可选择：

1. `Register`：注册新用户并开户
2. `Login`：登录系统
3. `Exit`：退出客户端

### 普通用户功能

登录普通用户后，可执行：

1. 查询余额
2. 存款
3. 取款
4. 转账
5. 修改密码
6. 查询流水
7. 退出登录

注意：

- 查询余额前需要再次输入密码
- 存款前需要再次输入密码
- 取款前需要再次输入密码
- 转账前需要再次输入密码

### 管理员功能

使用管理员账号登录后，可执行：

1. 查看用户列表
2. 解锁用户
3. 重置用户密码
4. 查询指定账户流水
5. 退出登录

## 自动化联调测试

项目提供了一套可执行的 PowerShell 联调脚本：`scripts/Test-ATMIntegration.ps1`

它会自动完成以下流程：

- 重建测试数据库
- 导入 `schema.sql` 和 `seed.sql`
- 启动服务端
- 注册用户
- 登录
- 查询余额
- 存款、取款、转账
- 查询流水
- 验证 3 次输错密码后的锁定逻辑
- 管理员解锁用户
- 管理员重置密码
- 管理员查询流水
- 验证异常输入

运行方式：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\Test-ATMIntegration.ps1
```

如果你的本机 MySQL 账号、密码或安装路径不同，可以覆盖脚本参数，例如：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\Test-ATMIntegration.ps1 `
  -DbUser root `
  -DbPassword 你的密码
```

成功输出示例：

```text
Preparing database...
Starting server...
Running integration flow...
Integration flow completed successfully.
--- server stdout ---
ATM server started at 127.0.0.1:8888
```

## 常见问题

### 1. 服务端提示缺少 `libmysql.dll`

请重新执行构建。当前 `CMakeLists.txt` 已配置在构建后自动复制 `libmysql.dll` 到 `build/` 目录。

### 2. 客户端启动后无法连接服务端

请检查：

- 服务端是否已经启动
- 服务端监听地址和端口是否正确
- 数据库连接是否成功

### 3. 中文显示乱码

请优先使用 Windows Terminal 或新版 PowerShell 运行程序。

### 4. 账户被锁定怎么办

连续输错密码 3 次后账户会被锁定，需要管理员登录后执行解锁操作。

## 演示建议

推荐演示顺序：

1. 启动 MySQL
2. 运行自动化联调脚本，证明系统整体可用
3. 手动启动 `atm_server.exe` 和 `atm_client.exe`
4. 使用普通用户演示登录、余额查询、存款、取款、转账、流水查询
5. 演示密码输错 3 次后的锁定逻辑
6. 使用管理员演示查看用户、解锁账户、重置密码、查询流水
7. 说明技术亮点：`hash + salt`、事务处理、流水落库、管理员日志

更简洁的答辩摘要见：`docs/Demo Summary.md`

## 当前状态

当前项目已经完成：

- 核心 ATM 业务功能
- 管理员功能
- 分页查询
- 自动化联调测试脚本
- 启动与使用文档

后续仍可继续优化：

- 更强的客户端输入校验
- 更友好的控制台界面
- 更完善的异常提示与日志输出
