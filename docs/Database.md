## 一、数据库设计稿

1. 数据库名称
建议：atm_system

2. 建表 SQL
CREATE DATABASE IF NOT EXISTS atm_system CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
USE atm_system;
-- 用户表
CREATE TABLE IF NOT EXISTS users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    card_no VARCHAR(32) NOT NULL UNIQUE,              -- 登录卡号
    user_name VARCHAR(64) NOT NULL,                   -- 用户姓名
    password_hash VARCHAR(128) NOT NULL,              -- 哈希后的密码
    password_salt VARCHAR(64) NOT NULL,               -- 随机盐
    failed_attempts INT NOT NULL DEFAULT 0,           -- 连续失败次数
    is_locked TINYINT(1) NOT NULL DEFAULT 0,          -- 是否锁定：0否 1是
    role VARCHAR(16) NOT NULL DEFAULT 'user',         -- user / admin
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
-- 账户表
CREATE TABLE IF NOT EXISTS accounts (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    account_no VARCHAR(32) NOT NULL UNIQUE,           -- 银行账户号
    balance DECIMAL(15,2) NOT NULL DEFAULT 0.00,      -- 余额
    status VARCHAR(16) NOT NULL DEFAULT 'active',     -- active / frozen / closed
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    CONSTRAINT fk_accounts_user_id FOREIGN KEY (user_id) REFERENCES users(id)
);
-- 交易流水表
CREATE TABLE IF NOT EXISTS transactions (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    account_id INT NOT NULL,
    tx_type VARCHAR(32) NOT NULL,                     -- deposit / withdraw / transfer_in / transfer_out
    amount DECIMAL(15,2) NOT NULL,
    balance_before DECIMAL(15,2) NOT NULL,
    balance_after DECIMAL(15,2) NOT NULL,
    related_account_no VARCHAR(32) DEFAULT NULL,      -- 转账对方账户
    remark VARCHAR(255) DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_transactions_account_id FOREIGN KEY (account_id) REFERENCES accounts(id)
);
-- 登录日志表
CREATE TABLE IF NOT EXISTS login_logs (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id INT DEFAULT NULL,
    card_no VARCHAR(32) NOT NULL,
    is_success TINYINT(1) NOT NULL,
    fail_reason VARCHAR(128) DEFAULT NULL,
    client_ip VARCHAR(64) DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);
-- 管理员操作日志表
CREATE TABLE IF NOT EXISTS admin_logs (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    admin_user_id INT NOT NULL,
    action VARCHAR(64) NOT NULL,                      -- unlock_user / reset_password / freeze_account 等
    target_user_id INT DEFAULT NULL,
    detail VARCHAR(255) DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_admin_logs_admin_user_id FOREIGN KEY (admin_user_id) REFERENCES users(id)
);

3. 初始化管理员 SQL
管理员密码不建议明文存库。课程设计阶段你可以先约定默认管理员密码，比如 admin123，再用程序生成哈希后插入。
如果只是展示 SQL 结构，可以先占位：
INSERT INTO users (card_no, user_name, password_hash, password_salt, role)
VALUES ('admin001', 'SystemAdmin', 'REPLACE_WITH_HASH', 'REPLACE_WITH_SALT', 'admin');

4. 字段设计说明
+ users.card_no
用于登录，等价于银行卡号/用户卡号
+ users.failed_attempts
连续输错密码次数
+ users.is_locked
超过 3 次输错后置为 1
+ users.role
区分普通用户和管理员
+ accounts.balance
使用 DECIMAL(15,2)，不要用浮点型
+ transactions
每一笔存取转都必须落库，便于审计和查询
+ login_logs
记录每次登录成功或失败，答辩时是加分点
+ admin_logs
记录管理员操作，体现系统完整性

5. 关键约束和规则
+ 一个用户对应一个主账户即可，课程设计足够
+ 每次资金操作都要先查账户状态 active
+ 转账必须用事务
+ 登录成功后清零 failed_attempts
+ 密码错误：
	+ failed_attempts + 1
	+ 如果达到 3，设置 is_locked = 1
+ 管理员可以解锁账户，解锁时应将 failed_attempts = 0
---