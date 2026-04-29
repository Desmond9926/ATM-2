CREATE DATABASE IF NOT EXISTS atm_system
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_general_ci;

USE atm_system;

CREATE TABLE IF NOT EXISTS users (
    id INT NOT NULL AUTO_INCREMENT,
    card_no VARCHAR(32) NOT NULL,
    user_name VARCHAR(64) NOT NULL,
    password_hash VARCHAR(128) NOT NULL,
    password_salt VARCHAR(64) NOT NULL,
    failed_attempts INT NOT NULL DEFAULT 0,
    is_locked TINYINT(1) NOT NULL DEFAULT 0,
    role VARCHAR(16) NOT NULL DEFAULT 'user',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_users_card_no (card_no),
    KEY idx_users_role (role),
    KEY idx_users_locked (is_locked)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS accounts (
    id INT NOT NULL AUTO_INCREMENT,
    user_id INT NOT NULL,
    account_no VARCHAR(32) NOT NULL,
    balance DECIMAL(15,2) NOT NULL DEFAULT 0.00,
    status VARCHAR(16) NOT NULL DEFAULT 'active',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY uk_accounts_account_no (account_no),
    KEY idx_accounts_user_id (user_id),
    KEY idx_accounts_status (status),
    CONSTRAINT fk_accounts_user_id
        FOREIGN KEY (user_id) REFERENCES users(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS transactions (
    id BIGINT NOT NULL AUTO_INCREMENT,
    account_id INT NOT NULL,
    tx_type VARCHAR(32) NOT NULL,
    amount DECIMAL(15,2) NOT NULL,
    balance_before DECIMAL(15,2) NOT NULL,
    balance_after DECIMAL(15,2) NOT NULL,
    related_account_no VARCHAR(32) DEFAULT NULL,
    remark VARCHAR(255) DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_transactions_account_id (account_id),
    KEY idx_transactions_type (tx_type),
    KEY idx_transactions_created_at (created_at),
    CONSTRAINT fk_transactions_account_id
        FOREIGN KEY (account_id) REFERENCES accounts(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS login_logs (
    id BIGINT NOT NULL AUTO_INCREMENT,
    user_id INT DEFAULT NULL,
    card_no VARCHAR(32) NOT NULL,
    is_success TINYINT(1) NOT NULL,
    fail_reason VARCHAR(128) DEFAULT NULL,
    client_ip VARCHAR(64) DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_login_logs_user_id (user_id),
    KEY idx_login_logs_card_no (card_no),
    KEY idx_login_logs_created_at (created_at),
    CONSTRAINT fk_login_logs_user_id
        FOREIGN KEY (user_id) REFERENCES users(id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS admin_logs (
    id BIGINT NOT NULL AUTO_INCREMENT,
    admin_user_id INT NOT NULL,
    action VARCHAR(64) NOT NULL,
    target_user_id INT DEFAULT NULL,
    detail VARCHAR(255) DEFAULT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_admin_logs_admin_user_id (admin_user_id),
    KEY idx_admin_logs_target_user_id (target_user_id),
    KEY idx_admin_logs_created_at (created_at),
    CONSTRAINT fk_admin_logs_admin_user_id
        FOREIGN KEY (admin_user_id) REFERENCES users(id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,
    CONSTRAINT fk_admin_logs_target_user_id
        FOREIGN KEY (target_user_id) REFERENCES users(id)
        ON UPDATE CASCADE
        ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
