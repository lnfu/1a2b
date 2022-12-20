TRUNCATE TABLE users;
SELECT * FROM users;
-- SELECT * FROM users ORDER BY username;

DROP TABLE rooms;
DROP TABLE users;
DROP TABLE invitations;
SHOW TABLES;

UPDATE users SET online_fd=2 WHERE username = 'Alice';
SELECT * FROM users;
SELECT * FROM rooms;



SELECT * FROM users WHERE username='Alice' AND passwd='2';



UPDATE rooms SET current_serial_number=current_serial_number+1 WHERE id=101;
SELECT * FROM invitations;
