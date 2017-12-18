CREATE DATABASE orderDB_small;

USE orderDB_small;

CREATE TABLE customer(
	id int(10) NOT NULL,
	name varchar(25) NOT NULL,
	gender char(1) NOT NULL,
	PRIMARY KEY (id)
);

CREATE TABLE book (
  id int(10) NOT NULL,
  title varchar(100) NOT NULL,
  authors varchar(200),
  publisher varchar(100),
  copies int(10),
  PRIMARY KEY (id)
);

CREATE TABLE website(
	id int(10) NOT NULL,
	name varchar(25) NOT NULL,
	url varchar(100),
	PRIMARY KEY (id)
);

CREATE TABLE price(
	website_id int(10) NOT NULL,
	book_id int(10) NOT NULL,
	price float NOT NULL,
	PRIMARY KEY (website_id,book_id),
	FOREIGN KEY (website_id) REFERENCES website(id),
	FOREIGN KEY (book_id) REFERENCES book(id)
);

CREATE TABLE orders(
	id int(10) NOT NULL,
	website_id int(10) NOT NULL,
	customer_id int(10) NOT NULL,
	book_id int(10) NOT NULL,
	order_date date,
	quantity int(10),
	PRIMARY KEY (id),
	FOREIGN KEY (website_id) REFERENCES website(id),
	FOREIGN KEY (customer_id) REFERENCES customer(id),
	FOREIGN KEY (book_id) REFERENCES book(id)
);