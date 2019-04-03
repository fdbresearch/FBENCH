# Favorita

Dataset of a large retailer, derived from a data available at the [kaggle competition](https://www.kaggle.com/c/favorita-grocery-sales-forecasting).

## Data Overview

* 6 Relations
* 125 million tuples in main relation (Sales)

### Relations

The relations ([full schema](schema.md)) have the following sizes (cardinality: number of tuples, arity: number of columns).

Relation    | Cardinality | Arity           | Uncompressed File Size
------------|-------------|-----------------|-----------------------
Item        | 4,100       | 4               | 68 kB
Oil         | 1,218       | 2               | 18 kB
Stores      | 54          | 5               | 676 B
Sales       | 125,497,040 | 5               | 2.5 GB
Transactions| 83,488      | 3               | 977 kB
Holiday     | 350         | 5               | 21 kB

### Changes made to the Dataset

We made some changes to the dataset linked at the top.

* Added missing values to Oil relation for all dates that are not present. When a date does not have an oilprice, it was set to the same price as the previous date. Otherwise these two relations significantly reduce the size of the join.
* Added default values in Holidays for dates that are not holidays.
* Turned Strings into integers, and added a dictionary mapping for each new ID.
* Renamed "Train"-relation into "Sales" and removed the ID Attribute from it since it is just a row number.

## Join Queries

We evaluated two natural joins, one over all relations and one over a selection:

### Natural join of all relations

This is the following Query in SQL:

```SQL
SELECT * FROM Sales NATURAL JOIN item NATURAL JOIN oil NATURAL JOIN
stores NATURAL JOIN transactions NATURAL JOIN holiday;
```

* Number of Tuples in join result: 127,872,512
* Number of values in factorized join result: 376,680,850
* Number of values in listing representation: 2,301,705,216
* Compression Factor: 6.11
* Arity: 18

### Natural Join of Sales, Transaction, Item, Stores

This is the following Query in SQL:

```SQL
SELECT * FROM Sales NATURAL JOIN item NATURAL JOIN stores NATURAL JOIN transactions;
```

* Number of Tuples in join result: 125,497,040
* Number of values in factorized join result: 376,672,340
* Number of values in listing representation: 1,631,461,520
* Compression Factor: 4.33
* Arity: 13
