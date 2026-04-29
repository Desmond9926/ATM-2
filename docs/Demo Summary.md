# Demo Summary

## Project Positioning

ATM Banking System is a Windows-based C++ course project that implements a console client, a TCP business server, and MySQL persistence. The project focuses on completing the full banking flow rather than building a large framework.

## Architecture

- Client: console menus, input collection, request sending, and result display
- Server: request parsing, action dispatch, authentication, account operations, transfer logic, and admin operations
- Database: users, accounts, transactions, login logs, and admin logs
- Communication: TCP + JSON text protocol

## Core Features

- user registration and account opening
- login and logout
- balance query
- deposit and withdrawal
- transfer with database transaction protection
- password change
- transaction history query with pagination
- lock after 3 failed password attempts
- administrator list, unlock, password reset, and transaction query

## Technical Highlights

- password storage uses random salt plus SHA-256 hash instead of plaintext
- transfer uses database transactions to keep debit and credit consistent
- every balance-changing operation is persisted to `transactions`
- login attempts are recorded in `login_logs`
- administrator actions are recorded in `admin_logs`
- server/database settings can be overridden by environment variables for local testing
- a PowerShell integration script can rebuild the database, start the server, and verify the full business flow automatically

## Demo Flow Recommendation

1. Show the database tables in `sql/schema.sql` and explain the role of `users`, `accounts`, and `transactions`.
2. Run `scripts/Test-ATMIntegration.ps1` to show the system passes an end-to-end flow.
3. Open the client and demonstrate one normal user flow: login, balance query, deposit, withdrawal, transfer, and transaction query.
4. Demonstrate the 3-time password lock rule.
5. Open the admin flow: list users, unlock the locked user, reset password, and query account transactions.
6. Conclude with security and consistency points: hash + salt, transaction logs, and transfer transaction handling.

## Key Files

- `client/ClientApp.cpp`: console menus and interaction
- `client/ClientSocket.cpp`: client TCP communication
- `server/ServerMain.cpp`: runtime configuration and startup
- `server/RequestHandler.cpp`: action routing
- `server/AuthService.cpp`: registration, login, password, and admin logic
- `server/AccountService.cpp`: balance, deposit, and withdrawal
- `server/TransactionService.cpp`: transfer and transaction queries
- `server/Database.cpp`: MySQL C API wrapper
- `server/PasswordUtil.cpp`: salt generation and SHA-256 hashing
- `scripts/Test-ATMIntegration.ps1`: automated end-to-end integration flow

## Completion Status

- core business functions: completed
- pagination support for admin/user queries: completed
- executable integration testing flow: completed
- README and run instructions: completed
- remaining improvement direction: stronger client-side validation and richer UI presentation
