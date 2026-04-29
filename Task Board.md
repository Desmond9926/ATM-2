# Task Board

## Project Overview

- Project name: ATM Banking System
- Goal: Build a C++ and MySQL based ATM simulation system with client-server architecture.
- Client: Console application on Windows.
- Server: C++ business server on Windows.
- Database: MySQL with MySQL C API.
- IDE/Toolchain: Visual Studio + Windows.

## Confirmed Scope

- Implement client-server separation.
- Client provides interactive console UI.
- Server handles all banking business logic.
- Support user registration/account opening.
- Support login and logout.
- Support balance query.
- Support deposit.
- Support withdrawal.
- Support transfer.
- Support password change.
- Support transaction detail storage and query.
- Password must be stored using hash + salt, not plaintext.
- Lock account after more than 3 incorrect password attempts.
- Include administrator functions.

## Architecture

### Client

- Console menu interaction.
- Collect user input.
- Send requests to server over TCP.
- Display server responses.

### Server

- Accept client TCP connections using Winsock.
- Parse JSON requests.
- Dispatch requests by action.
- Execute authentication, account, transaction, and admin logic.
- Read/write persistent data in MySQL.

### Database

- Store users.
- Store accounts.
- Store transaction records.
- Store login logs.
- Store administrator operation logs.

## Technology Choices

- Language: C++
- OS: Windows
- IDE: Visual Studio
- Communication: TCP with Winsock
- Protocol: JSON text protocol
- Database: MySQL
- DB library: MySQL C API
- Password strategy: random salt + SHA-256 hash

## Recommended Project Structure

```text
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
```

## Database Design Summary

### `users`

- `id`
- `card_no`
- `user_name`
- `password_hash`
- `password_salt`
- `failed_attempts`
- `is_locked`
- `role`
- `created_at`
- `updated_at`

### `accounts`

- `id`
- `user_id`
- `account_no`
- `balance`
- `status`
- `created_at`
- `updated_at`

### `transactions`

- `id`
- `account_id`
- `tx_type`
- `amount`
- `balance_before`
- `balance_after`
- `related_account_no`
- `remark`
- `created_at`

### `login_logs`

- `id`
- `user_id`
- `card_no`
- `is_success`
- `fail_reason`
- `client_ip`
- `created_at`

### `admin_logs`

- `id`
- `admin_user_id`
- `action`
- `target_user_id`
- `detail`
- `created_at`

## Key Business Rules

- Login checks whether the account is locked before password verification.
- Each failed login increments `failed_attempts`.
- When failed attempts reach 3, set `is_locked = 1`.
- Successful login resets `failed_attempts` to 0.
- Every deposit, withdrawal, and transfer must be written to `transactions`.
- Transfer must use database transactions to guarantee consistency.
- Password changes require verifying the old password.
- Administrator can unlock users and reset passwords.

## Protocol Summary

### Request format

```json
{
  "action": "login",
  "sessionId": "",
  "data": {}
}
```

### Response format

```json
{
  "success": true,
  "code": 0,
  "message": "操作成功",
  "data": {}
}
```

### Core actions

- `register`
- `login`
- `logout`
- `query_balance`
- `deposit`
- `withdraw`
- `transfer`
- `change_password`
- `query_transactions`
- `admin_list_users`
- `admin_unlock_user`
- `admin_reset_password`
- `admin_query_transactions`

## Main Modules

### Common

- `Models.h`: shared data structures
- `Protocol.h/.cpp`: request/response definitions and serialization
- `JsonUtil.h`: JSON helper wrapper

### Server

- `Database`: MySQL connection and transaction handling
- `PasswordUtil`: salt generation and password hashing
- `AuthService`: register, login, logout, password change, admin user management
- `AccountService`: balance query, deposit, withdrawal
- `TransactionService`: transfer and transaction history
- `RequestHandler`: action dispatch
- `TcpServer`: socket listening and client handling

### Client

- `ClientSocket`: TCP connection to server
- `ClientApp`: console menus and user interaction
- `ClientMain.cpp`: application startup

## Task Board

Status legend: `[x]` completed, `[~]` partially completed, `[ ]` pending.

### Phase 1: Project Foundation

- [~] Create Visual Studio solution and projects for `client`, `server`, and `common`
- [~] Configure include/lib paths for MySQL C API
- [x] Add basic Winsock initialization and cleanup
- [x] Add JSON library dependency if needed

### Phase 2: Database Layer

- [x] Create `sql/schema.sql`
- [x] Create `sql/seed.sql`
- [x] Implement `Database.h/.cpp`
- [x] Verify database connection from server

### Phase 3: Security Layer

- [x] Implement random salt generation
- [x] Implement SHA-256 hashing
- [x] Implement password verify function
- [~] Test hash storage and verification flow

### Phase 4: Authentication

- [x] Implement user registration/account opening
- [x] Implement login
- [x] Implement logout
- [x] Implement failed login counting
- [x] Implement automatic account locking after 3 failed attempts
- [x] Implement password change
- [x] Implement login log recording

### Phase 5: Account Operations

- [x] Implement query balance
- [x] Implement deposit
- [x] Implement withdrawal
- [x] Validate amount and account status
- [x] Record transaction details for each operation

### Phase 6: Transfer

- [x] Implement transfer request handling
- [x] Validate target account existence
- [x] Prevent self-transfer
- [x] Use database transaction for debit and credit
- [x] Record both transfer-out and transfer-in details

### Phase 7: Transaction Query

- [x] Implement user transaction history query
- [x] Add pagination support
- [x] Format output for console display

### Phase 8: Administrator Functions

- [x] Implement admin login recognition through `role`
- [x] Implement list users/accounts
- [x] Implement unlock user
- [x] Implement reset user password
- [x] Implement admin transaction query
- [x] Implement admin operation logs

### Phase 9: Client Console UI

- [x] Implement welcome menu
- [x] Implement user menu
- [x] Implement admin menu
- [~] Implement input validation and prompts
- [x] Display clear success/failure messages

### Phase 10: Integration and Testing

Verified by local executable flow: `scripts/Test-ATMIntegration.ps1`. The flow resets the test database, starts the server, and validates registration, login, lockout, balance changes, transfer, transaction query, admin unlock/reset, and invalid input checks.

- [x] Test registration flow end to end
- [x] Test login success and failure flow
- [x] Test lock after 3 failed passwords
- [x] Test deposit, withdrawal, and balance updates
- [x] Test transfer transaction consistency
- [x] Test transaction history query
- [x] Test administrator unlock and reset operations
- [x] Test abnormal inputs and error handling

### Phase 11: Documentation

- [x] Write `README.md`
- [x] Add database initialization instructions
- [x] Add server/client startup instructions
- [x] Add screenshots or console run examples if needed
- [x] Summarize project highlights for defense/demo

## Milestones

- Milestone 1: Database and server can connect successfully.
- Milestone 2: Registration and login work with hashed passwords.
- Milestone 3: Balance, deposit, and withdrawal are functional.
- Milestone 4: Transfer and transaction records are complete.
- Milestone 5: Administrator functions are complete.
- Milestone 6: Full client-server demo can be presented.

## Acceptance Criteria

- User can register and log in through the client.
- Password is never stored in plaintext.
- Account locks after 3 consecutive failed password attempts.
- User can query balance, deposit, withdraw, and transfer.
- All capital changes are persisted in transaction records.
- User can query personal transaction details.
- Administrator can view users, unlock accounts, and reset passwords.
- The system runs in Visual Studio on Windows and uses MySQL successfully.

## Notes For Next Steps

- Start with `schema.sql` and the server database connection first.
- Keep the first version minimal and stable before refining UI.
- Prefer completing one full business flow end to end before expanding features.
- This document should be updated as tasks move from pending to completed.
