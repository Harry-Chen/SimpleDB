CREATE DATABASE orderDB;

USE orderDB;

CREATE TABLE customer(
	id INT(10) NOT NULL,
	name VARCHAR(25) NOT NULL,
	gender VARCHAR(1) NOT NULL,
	PRIMARY KEY (id)
);

CREATE TABLE book (
  id INT(10) NOT NULL,
  title VARCHAR(100) NOT NULL,
  authors VARCHAR(200),
  publisher VARCHAR(100),
  copies INT(10),
  PRIMARY KEY (id)
);

CREATE TABLE website(
	id INT(10) NOT NULL,
	name VARCHAR(25) NOT NULL,
	url VARCHAR(100),
	PRIMARY KEY (id)
);

CREATE TABLE price(
	website_id INT(10) NOT NULL,
	book_id INT(10) NOT NULL,
	price FLOAT NOT NULL,
	PRIMARY KEY (website_id,book_id),
	FOREIGN KEY (website_id) REFERENCES website(id),
	FOREIGN KEY (book_id) REFERENCES book(id)
);

CREATE TABLE orders(
	id INT(10) NOT NULL,
	website_id INT(10) NOT NULL,
	customer_id INT(10) NOT NULL,
	book_id INT(10) NOT NULL,
	order_date DATE,
	quantity INT(10),
	PRIMARY KEY (id),
	FOREIGN KEY (website_id) REFERENCES website(id),
	FOREIGN KEY (customer_id) REFERENCES customer(id),
	FOREIGN KEY (book_id) REFERENCES book(id)
);
