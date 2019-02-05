# Relational Schema: Retailer

We give the schemas of the 5 tables including some additional information on the domain sizes, primary keys, and whether an attribute is categorical.

## Inventory

Attributes|Categorical|Distinct Values (84055817)
----------|-----------|---------------------------------------:
store| Y |1298
date | Y |124
item | Y |3647
units|   |3156

Primary key: (store, date, item)

## Item

Attributes|Categorical|Distinct Values (5618)
----------|-----------|---------------------------------------:
item            | Y |5618
subcategory     | Y |31
category        | Y |10
categorycluster | Y |8
prize           |   |267

Primary key: item

## Stores

Attributes|Categorical|Distinct Values (1317)
----------|-----------|---------------------------------------:
store                       | Y |1317
zip                         | Y |1302
regioncode                  | Y |3
climate zone                | Y |6
total area                  |   |952
selling area                |   |1273
avg high                    |   |1155
shop1distance               |   |258
shop1drivetime              |   |252
shop2distance               |   |771
shop2drivetime              |   |676
shop3distance               |   |667
shop3drivetime              |   |622
shop4distance               |   |798
shop4drivetime              |   |691

Primary key: store

## Weather

Attributes|Categorical|Distinct Values (1159457)
----------|-----------|---------------------------------------:
store       | Y |1317
date        | Y |881
rain        | Y |2
snow        | Y |2
maxtemp     |   |94
mintemp     |   |88
meanwind    |   |3532
thunder     | Y |2

Primary key: (store, date)

## Demographics

Attributes|Categorical|Distinct Values (1302)
----------|-----------|---------------------------------------:
zip               | Y |1302
population        |   |1283
white             |   |1271
asian             |   |897
pacific           |   |189
black             |   |1097
median_age        |   |255
occupiedhouses    |   |1254
houses            |   |1257
families          |   |1231
households        |   |1254
husbwife          |   |1205
males             |   |1262
females           |   |1262
householdschildren|   |1084
hispanic          |   |1164

Primary key: zip
