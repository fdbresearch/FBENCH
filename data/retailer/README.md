# Retailer Dataset

The dataset contains inventory data of a large retailer.

## Data Overview
* 5 Relations
* Star Schema: 1 main relation which the other ones are joined into using PK/FK joins.
* 84 Million tuples in main relation

## Relations

The relations ([full schema](schema.md)) have the following sizes (cardinality: number of tuples, arity: number of columns).

 Relation     | Cardinality | Arity             | Uncompressed File Size 
--------------|-------------|-------------------|-----------------------
 Inventory    | 84,055,817  | 4  | 2 GB      
 Items        | 5,618       | 5  | 129 KB    
 Stores       | 1,317       | 15 | 139 KB    
 Demographics | 1,302       | 16 | 161 KB    
 Weather      | 1,159,457   | 8  | 33 MB     

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
* Arity: 43
