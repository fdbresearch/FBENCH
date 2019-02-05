# Relational Schema: IMDB

We give the schemas of the 13 relations, including some additional information on the domain sizes, primary keys, and whether an attribute is categorical.

## title.basics

Attributes|Categorical|Distinct Values (4973640)
----------|-----------|---------------------------------------:
tid| Y |4973640
ttype| Y |10
primtitle| Y |2653583
origtitle| Y |2667322
adult| Y |2
syear|   |147
eyear|   |94
runtime|   |787

Primary key: tid

Basic information for the titles in the database.
The relation "genres" is derived from an array-field in the original data and does not have non-key attributes.

### title.basics_genres

Attributes|Categorical|Distinct Values (7570686)
----------|-----------|---------------------------------------:
tid| Y |4587575
genre| Y |28

Primary key: (tid, genre)

## title.akas

Attributes|Categorical|Distinct Values (3587385)
----------|-----------|---------------------------------------:
tid| Y |2085257
ordr| Y |104
title| Y |2369422
rgn| Y |246
language| Y |92
isorig| Y |3

Primary Key: (tid, ordr)

The two relations "types" and "attributes" are derived from array-fields in the original data and do not have non-key attributes.

These relations contain the "also known as" information for titles in the database, including the appropriate region and language. Since every title can have more than one such entry, titleID alone is not sufficient as a key, hence the "ordering" attribute is added.

### title.akas_types

Attributes|Categorical|Distinct Values (907393)
----------|-----------|---------------------------------------:
tid| Y |355337
ordr| Y |84
type| Y |15

Primary key: (tid, ordr)

### title.akas_attributes

Attributes|Categorical|Distinct Values (158731)
----------|-----------|---------------------------------------:
tid| Y |114429
ordr| Y |92
att| Y |183

Primary Key: (tid, ordr)

## title.episode

Attributes|Categorical|Distinct Values (3338054)
----------|-----------|---------------------------------------:
epid| Y |3338054
tid| Y |106712
season| Y |163
episode| Y |15233

Primary key: epid

TitleID is a foreign key referencing title.basics. The attributes epid and tid are from the same domain. The relation is for episodes of a TV show. Thus epid is the tid of the specific episode and tid is the ID of the title for the entire show.

## title.ratings

Attributes|Categorical|Distinct Values (828661)
----------|-----------|---------------------------------------:
tid| Y |828661
average|   |91
numvotes|   |16020

Primary key: tid
  
Contains ratings for a number of titles.

## Name.basics

Attributes|Categorical|Distinct Values (8582320)
----------|-----------|---------------------------------------:
nid| Y |8582320
primname| Y |6778032
birthyear|   |388
deathyear|   |371

Primary Key: nid

Both "name.basics_profession" and "name.basics_knownfor" are derived from array-fields in the original data and do not have non-key attributes. This relation contains basic information about people (actors, directors, writers, producers, etc.) in the database.

### name.basics_knownfor

Attributes|Categorical|Distinct Values (14227604)
----------|-----------|---------------------------------------:
nid| Y |7611501
tid| Y |1290808

Primary Key: (tid, nid)

### name.basics_profession

Attributes|Categorical|Distinct Values (11170036)
----------|-----------|---------------------------------------:
nid| Y |8582320
profession| Y |41

Primary Key: (nid, profession)

## title.principal_cast

Attributes|Categorical|Distinct Values (28062515)
----------|-----------|---------------------------------------:
tid| Y |4427494
ordr| Y |10
nid| Y |3384274
category| Y |12
job| Y |29565
characters| Y |1971591

Primary key: (tid, ordr)

nid is a foreign key relating to name.basics. Principal cast members of a title.

## title.crew_directors

Attributes|Categorical|Distinct Values (3567579)
----------|-----------|---------------------------------------:
tid| Y |2840818
nid| Y |524753

Primary Key: (tid, nid)

Both, "title.crew_directors" and "title.crew_writers" are derived from the original relation "title.crew", which only has the primary key "titleID" and two array-attributes "directors" and "writers". Hence, both these relations here have no non-key attributes.

## title.crew_writers

Attributes|Categorical|Distinct Values (5539884)
----------|-----------|---------------------------------------:
tid| Y |2453267
nid| Y |669590

Primary key: (tid, nid)
