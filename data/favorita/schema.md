# Relational Schema: Favorita

We give the schemas of the 6 relations, including some additional information on the domain sizes, primary keys, and whether an attribute is categorical.

## Sales

Attributes|Categorical|Distinct Values (125497040)
----------|-----------|---------------------------------------:
date        | Y |1684
store       | Y |54
item        | Y |4036
unit_sales  |   |258474
onpromotion | Y |3

Primary Key: (date, store, item)

## Stores

Attributes|Categorical|Distinct Values (54)
----------|-----------|---------------------------------------:
store       | Y |54
city        | Y |22
state       | Y |16
store_type  | Y |5
cluster     | Y |17

Primary Key: store

## Item

Attributes|Categorical|Distinct Values (4100)
----------|-----------|---------------------------------------:
item        | Y |4100
family      | Y |33
class_id    | Y |337
perishable  | Y |2

Primary Key: item

## Transactions

Attributes|Categorical|Distinct Values (83606)
----------|-----------|---------------------------------------:
date        | Y |1684
store       | Y |54
transactions|   |4994

Primary Key: (date, store)

## Holiday

Attributes|Categorical|Distinct Values (1734)
----------|-----------|---------------------------------------:
date        | Y |1704
holiday_type| Y |6
locale      | Y |3
locale_name | Y |24
transferred | Y |2

Primary Key: (date, holiday_type, locale_name)

## Oil

Attributes|Categorical|Distinct Values (1704)
----------|-----------|---------------------------------------:
date        | Y |1704
oil_prize   |   |998

Primary Key: date
