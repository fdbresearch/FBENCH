# Retailer Dataset

The dataset contains inventory data of a large retailer.

## Data Overview
* 5 Relations
* Star Schema: 1 main relation which the other ones are joined into using PK/FK joins.
* 84 Million tuples in main relation

## Relations

The relations have the following sizes (cardinality: number of tuples, arity: number of columns).

 Relation     | Cardinality | Arity             | Uncompressed File Size 
--------------|-------------|-------------------|-----------------------
 Inventory    | 84,055,817  | 4 (3 FK, 1 non-key)  | 2 GB      
 Items        | 5,618       | 5 (1 PK, 4 non-key)  | 129 KB    
 Stores       | 1,317       | 15 (1 PK, 1 FK, 14 non-key)  | 139 KB    
 Demographics | 1,302       | 16 (1 PK, 15 non-key) | 161 KB    
 Weather      | 1,159,457   | 8 (2 PK, 6 non-key)  | 33 MB     

## Join Query: 

The query evaluated on the dataset is the full natural join of all five relations:

```SQL
SELECT * FROM inventory NATURAL JOIN stores NATURAL JOIN weather NATURAL JOIN
demograpics NATURAL JOIN items;
```

* Number of Tuples in join result: 84,055.817
* Number of values in factorized join result: 3,614,400,131
* Number of values in listing representation: 169,231,200
* Compression Factor: 21.4
* Degree (# of columns): 43
