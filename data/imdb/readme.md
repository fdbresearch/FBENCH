# IMDB dataset

Database with information about movies and people involved in their production. Original data along with licensing information can be found [on the IMDB website](https://www.imdb.com/interfaces/). The website also includes a description of the dataset. The folder includes a python script "prepare.py", which converts the data from the IMDB website into a normalised form that can be used with FBENCH (see section [Relations](#Relations) below). 

## Data Overview

* 7 Relations (13 after normalisation)
* 8.5 million names
* 4.9 million titles
* 828k ratings

### Relations

The data as given is not normalised: many of the relations include "array"-fields that contain more than one datum. Further, most fields in the data are strings, which is not a datatype supported by DFDB. The script "prepare.py" converts the original data into a database containing only fields of type int (or double in one case) and normalises the array-fields. This results in the following relations (see also the [full schema](schema.md) for a summary and domain sizes):

Relation | Cardinality | Arity | Uncompressed File Size
---------|-------------|-------|-----------------------
title.basics | 4,973,640 | 8 | 186 MB
title.basics_genres | 7,570,686 | 2 | 80 MB
title.akas | 3,587,385 | 6 | 86 MB
title.akas_types | 907,393 | 3 | 11 MB
title.akas_attributes | 158,731 | 3 | 2 MB
title.crew_directors | 3,567,579 | 2 | 52 MB
title.crew_writers | 5,539,884 | 2 | 80 MB
title.episode | 3,338,054 | 4 | 68 MB
title.principals | 28,062,515 | 6 | 741 MB
title.ratings | 828,661 | 3 | 12 MB
name.basics | 8,582,320 | 4 | 186 MB
name.basics_profession | 11,170,036 | 2 | 119 MB
name.basics_knownfor | 14,227,604 | 2 | 219 MB

### Data Types

With the exception of "averageRating" (which is a double) every attribute after running "prepare.py" is of type int. Identical values that appear in different tables (but are within the same domain, i.e., titleID or nameID) are always mapped to the same integer. The conversion used for categorical attributes is thus a simple bijection mapping a categorical value to a number. Booleans are mapped to 0 and 1. Values that were already numbers (e.g. the years and runtimeMinutes) remain unchanged.

The raw data includes a numerous amount of NULL-entries, which are converted to "-1". Exception: If an array-field is NULL in the original data, the derived table for this array (title.akas_types, title.akas_attributes, etc.) does not have a matching entry.

## Queries

The folder contains four queries, each in their own folder. Copying the dtree.txt and schema.conf out of the respective queries folder into the directory also containing the data tables makes it possible to run the query using FBENCH. All the joins are primary key/foreign key joins, but some are many-to-many. 

The Queries are as follows:

### Query 1
Joins titles with their AKA's (also known as, i.e., foreign language versions). For every title we then retrieve the genre, the director (including the basic information) and the directors profession.

In SQL: 
```SQL
SELECT * FROM takas NATURAL JOIN tbasics NATURAL JOIN tgenre NATURAL JOIN directors NATURAL JOIN name NATURAL JOIN profession;
```
### Query 2
Joins the basic information for every person in the database with their professions and the titles (including AKA's) they are known for.

In SQL:
```SQL
SELECT * FROM takas NATURAL JOIN tprincipals NATURAL JOIN name NATURAL JOIN profession NATURAL JOIN tbasics;
```
### Query 3
Join the titles with their principal cast (including basic information) and their rating.

In SQL:
```SQL
SELECT * FROM tbasics NATURAL JOIN tprincipals NATURAL JOIN name NATURAL JOIN tratings;
```
### Query 4
For every title get the principal cast and for every cast member which other titles they are known for.

In SQL:
```SQL
SELECT * FROM tbasics t1, tprincipals p, name n, knownfor k, tbasics t2
  WHERE t1.tID = p.tID and p.nID = n.nID and k.nID = n.nID and k.tID = t2.tID;
```

### Query Statistics

Of these queries, the first three were intended to achieve particularly high compression factors, by blowing the result up with lots of redundant information (hence the inclusion of title.akas). Queries 4 and 5 are more natural. Query 4 could be of interest if one wants to find a correlation between cast members and a films rating. Query 5 is something one might expect on an information page (though likely with an additional selection of specific titles in the first join).

Compression statistics for these queries:

Query | #tuples in join | #values (singletons) | Compression Factor
------|-----------------|----------------------|-------------------
[1](#Query-1) | 38161177  | 22372840  | 11.14
[2](#Query-2) | 70584013  | 132069502 | 33.68
[3](#Query-3) | 47664792  | 6978828   | 2.64
[4](#Query-4) | 244895933 | 101099366 | 9.91

