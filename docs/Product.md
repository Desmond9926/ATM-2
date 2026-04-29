## 总体方案
1. 采用 客户端/服务器 架构
2. 客户端是控制台菜单程序，负责输入与结果展示
3. 服务端负责所有业务逻辑与数据库操作
4. 数据库存储用户、账户、交易明细、登录失败次数、锁定状态
5. 通信采用 TCP + Winsock
6. 密码采用 salt + SHA-256 哈希存储
7. 所有资金类操作写入交易流水，并用数据库事务保证一致性
## 建议目录结构
ATM-2/
├─ client/
│  ├─ ClientMain.cpp
│  ├─ ClientApp.h
│  ├─ ClientApp.cpp
│  ├─ ClientSocket.h
│  └─ ClientSocket.cpp
├─ server/
│  ├─ ServerMain.cpp
│  ├─ TcpServer.h
│  ├─ TcpServer.cpp
│  ├─ RequestHandler.h
│  ├─ RequestHandler.cpp
│  ├─ AuthService.h
│  ├─ AuthService.cpp
│  ├─ AccountService.h
│  ├─ AccountService.cpp
│  ├─ TransactionService.h
│  ├─ TransactionService.cpp
│  ├─ Database.h
│  ├─ Database.cpp
│  ├─ PasswordUtil.h
│  └─ PasswordUtil.cpp
├─ common/
│  ├─ Protocol.h
│  ├─ Protocol.cpp
│  ├─ JsonUtil.h
│  └─ Models.h
├─ sql/
│  ├─ schema.sql
│  └─ seed.sql
└─ README.md

## 功能清单
1. 开户/注册
2. 登录
3. 退出登录
4. 查询余额
5. 存款
6. 取款
7. 转账
8. 修改密码
9. 查询交易明细
10. 连续输错 3 次锁定
11. 管理员解锁账户
12. 管理员查看用户与流水
## 数据库设计
### users
id
card_no
user_name
password_hash
password_salt
failed_attempts
is_locked
role
created_at
updated_at

### accounts
id
user_id
account_no
balance
status
created_at
updated_at

### transactions
id
account_id
tx_type
amount
balance_before
balance_after
related_account_no
remark
created_at

### login_logs
id
user_id
card_no
is_success
fail_reason
client_ip
created_at
## 关键业务规则
1. 登录时先检查 is_locked
2. 密码错误一次，failed_attempts + 1
3. 达到 3 次，自动锁定 is_locked = 1
4. 登录成功后将 failed_attempts 清零
5. 存款/取款/转账后必须写 transactions
6. 转账必须使用数据库事务
7. 取款和转账前必须检查余额是否足够
8. 修改密码时要验证旧密码
## 密码存储方案
注册时：
1. 生成随机盐 salt
2. 计算 SHA256(password + salt)
3. 数据库保存 password_hash 和 password_salt
登录时：
4. 取出数据库中的 salt
5. 重新计算 SHA256(inputPassword + salt)
6. 与 password_hash 比较
## 通信协议建议
为了课程项目简单稳定，建议使用“文本 JSON 协议”：
请求示例：
{
  "action": "login",
  "cardNo": "62220001",
  "password": "123456"
}
响应示例：
{
  "success": true,
  "message": "登录成功",
  "userId": 1
}

## 服务端模块职责
1. TcpServer
监听端口，接收客户端连接
2. RequestHandler
解析请求，根据 action 分发到业务层
3. AuthService
注册、登录、改密、锁定、解锁
4. AccountService
余额查询、存款、取款
5. TransactionService
转账、流水记录、明细查询
6. Database
封装 MySQL C API 连接与 SQL 执行
7. PasswordUtil
随机盐生成、哈希计算
## 客户端模块职责
1. ClientSocket
连接服务器、发送请求、接收响应
2. ClientApp
控制台菜单和用户交互
3. 登录后显示不同菜单：
普通用户：
- 查询余额
- 存款
- 取款
- 转账
- 修改密码
- 查询明细
- 退出
管理员：
- 查看用户
- 解锁账户
- 查看交易明细
- 退出
## 开发顺序
1. 建立 VS 解决方案，分 client/server/common
2. 配置 MySQL C API
3. 编写 schema.sql
4. 实现数据库连接封装
5. 实现密码哈希工具
6. 实现注册与登录
7. 实现 3 次错误锁定
8. 实现余额查询、存款、取款
9. 实现转账事务
10. 实现交易明细查询
11. 实现管理员解锁
12. 联调客户端与服务端
13. 补充 README 和演示数据
