TRUNCATE TABLE users;
SELECT * FROM users;
-- SELECT * FROM users ORDER BY username;

DROP TABLE rooms;
DROP TABLE users;
SHOW TABLES;

UPDATE users SET online_fd=2 WHERE username = 'Alice';
SELECT * FROM users;


SELECT * FROM rooms;

SELECT * FROM users WHERE username='Alice' AND passwd='2';