USE atm_system;

START TRANSACTION;

-- Default admin account
-- card_no: admin001
-- password: admin123
-- hash rule used here: SHA2(password + salt, 256)
INSERT INTO users (
    card_no,
    user_name,
    password_hash,
    password_salt,
    failed_attempts,
    is_locked,
    role
)
SELECT
    'admin001',
    'SystemAdmin',
    SHA2(CONCAT('admin123', 'ATMADMIN001SALT'), 256),
    'ATMADMIN001SALT',
    0,
    0,
    'admin'
WHERE NOT EXISTS (
    SELECT 1 FROM users WHERE card_no = 'admin001'
);

-- Demo user 1
-- card_no: 62220001
-- password: 123456
INSERT INTO users (
    card_no,
    user_name,
    password_hash,
    password_salt,
    failed_attempts,
    is_locked,
    role
)
SELECT
    '62220001',
    'ZhangSan',
    SHA2(CONCAT('123456', 'ATMUSER001SALT'), 256),
    'ATMUSER001SALT',
    0,
    0,
    'user'
WHERE NOT EXISTS (
    SELECT 1 FROM users WHERE card_no = '62220001'
);

-- Demo user 2
-- card_no: 62220002
-- password: 123456
INSERT INTO users (
    card_no,
    user_name,
    password_hash,
    password_salt,
    failed_attempts,
    is_locked,
    role
)
SELECT
    '62220002',
    'LiSi',
    SHA2(CONCAT('123456', 'ATMUSER002SALT'), 256),
    'ATMUSER002SALT',
    0,
    0,
    'user'
WHERE NOT EXISTS (
    SELECT 1 FROM users WHERE card_no = '62220002'
);

INSERT INTO accounts (
    user_id,
    account_no,
    balance,
    status
)
SELECT
    u.id,
    'ACC10001',
    5000.00,
    'active'
FROM users u
WHERE u.card_no = '62220001'
  AND NOT EXISTS (
      SELECT 1 FROM accounts WHERE account_no = 'ACC10001'
  );

INSERT INTO accounts (
    user_id,
    account_no,
    balance,
    status
)
SELECT
    u.id,
    'ACC10002',
    3200.00,
    'active'
FROM users u
WHERE u.card_no = '62220002'
  AND NOT EXISTS (
      SELECT 1 FROM accounts WHERE account_no = 'ACC10002'
  );

-- Initial demo transaction records
INSERT INTO transactions (
    account_id,
    tx_type,
    amount,
    balance_before,
    balance_after,
    related_account_no,
    remark,
    created_at
)
SELECT
    a.id,
    'deposit',
    5000.00,
    0.00,
    5000.00,
    NULL,
    'initial seed balance',
    NOW()
FROM accounts a
WHERE a.account_no = 'ACC10001'
  AND NOT EXISTS (
      SELECT 1
      FROM transactions t
      WHERE t.account_id = a.id
        AND t.tx_type = 'deposit'
        AND t.amount = 5000.00
        AND t.remark = 'initial seed balance'
  );

INSERT INTO transactions (
    account_id,
    tx_type,
    amount,
    balance_before,
    balance_after,
    related_account_no,
    remark,
    created_at
)
SELECT
    a.id,
    'deposit',
    3200.00,
    0.00,
    3200.00,
    NULL,
    'initial seed balance',
    NOW()
FROM accounts a
WHERE a.account_no = 'ACC10002'
  AND NOT EXISTS (
      SELECT 1
      FROM transactions t
      WHERE t.account_id = a.id
        AND t.tx_type = 'deposit'
        AND t.amount = 3200.00
        AND t.remark = 'initial seed balance'
  );

COMMIT;
