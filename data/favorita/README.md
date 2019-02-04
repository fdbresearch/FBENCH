# Favorita

Dataset of a large retailer, derived from a data available at the [kaggle competition](https://www.kaggle.com/c/favorita-grocery-sales-forecasting).

## Data Overview

* 6 Relations: Item, Stores, Transactions, Oil, Train (i.e. Training Dataset), Holiday
* 4100 Items
* 54 Stores
* 83488 Transactions
* Oil prize for 1218 days
* 125,497,040 tuples in Train(ing dataset)
* 350 Holidays/Events 

## Relations

The schemas of the relations are as follows.

* Item (item int, family string, class_id int, perishable int);
* Oil (date date, oil_prize float);
* Stores (store int, city string, state string, store_type string, cluster int);
* Train (id int, date date, store int, item int, unit_sales float, onpromotion string);
* Transactions (date date, store int, transactions int);
* Holiday(date date, holiday_type string,locale string,locale_name string, description string, transferred string);

## Changes made to the Dataset
* Added missing values to Oil relation for all dates that are not present. When a date does not have an oilprice, it was set to the same price as the previous date. Otherwise these two relations significantly reduce the size of the join.
* Added default values in Holidays for dates that are not holidays.
* Turned Strings into integers, and added a dictionary mapping for each new ID.
* Removed ID Attribute from Train-Relation since it is just a row number.

## Join Queries

We evaluated two natural joins, one over all relations and one over a selection:

### Natural join of all relations

This is the following Query in SQL:

```SQL
SELECT * FROM train NATURAL JOIN item NATURAL JOIN oil NATURAL JOIN
stores NATURAL JOIN transactions NATURAL JOIN holiday;
```

* Number of Tuples in join result: 127,872,512
* Number of values in factorized join result: 376,680,850
* Number of values in listing representation: 2,301,705,216
* Compression Factor: 6.11
* Arity: 18

### Natural Join of Train, Transaction, Item, Stores

This is the following Query in SQL:

```SQL
SELECT * FROM train NATURAL JOIN item NATURAL JOIN stores NATURAL JOIN transactions;
```

* Number of Tuples in join result: 125497040
* Number of values in factorized join result: 376672340
* Number of values in listing representation: 1631461520
* Compression Factor: 4.33
* Arity: 13
