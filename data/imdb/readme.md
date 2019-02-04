# IMDB dataset

Database with information about movies and people involved in their production. Original data along with licensing information can be found [on the IMDB website](https://www.imdb.com/interfaces/). The website also includes a description of the dataset. The folder includes a python script "prepare.py", which converts the data from the IMDB website into a normalised form that can be used with FBENCH (see section [Relations](#Relations) below). 

## Data Overview

* 7 Relations (13 after normalisation)
* 8.5 million names
* 4.9 million titles
* 828k ratings

## Relations

The data as given is not normalised: many of the relations include "array"-fields that contain more than one datum. Further, most fields in the data are strings, which is not a datatype supported by DFDB. The script "prepare.py" converts the original data into a database containing only fields of type int (or double in one case) and normalises the array-fields. This results in the following relations:

* title.akas(titleID, ordering, title, region, language, isOriginalTitle)
  * title.akas_types(titleID, ordering, type)
  * title.akas_attributes(titleID, ordering, attribute)

  The pair (titleID, ordering) is a primary key in "title.akas".
  The two relations "types" and "attributes" are derived from array-fields in the original data and do not have non-key attributes.
  These relations contain the "also known as" information for titles in the database, including the appropriate region and language. Since every title can have more than one such entry, titleID alone is not sufficient as a key, hence the "ordering" attribute is added.
* title.basics(titleID, titleType, primaryTitle, originalTitle, isAdult, startYear, endYear, runtimeMinutes)
  * title.basics_genres(titleID, genre)

  TitleID is a primary key in "title.basics". 
  The relation "genres" is derived from an array-field in the original data and does not have non-key attributes.
  Basic information for the titles in the database.
* title.crew_directors(titleID, nameID)
* title.crew_writers(titleID, nameID)
  
  Both, "title.crew_directors" and "title.crew_writers" are derived from the original relation "title.crew", which only has the primary key "titleID" and two array-attributes "directors" and "writers". Hence, both these relations here have no non-key attributes.
* title.episode(episodeID, titleID, seasonNumber, episodeNumber)
  
  EpisodeID is the primary key. TitleID is a foreign key from title.basics. Both are from the same domain, however.
  The relation is for episodes of a TV show. Thus episodeID is the titleID of the specific episode and titleID is the ID of the title for the entire show.
* title.principals(titleID, ordering, nameID, category, job, characters)
  
  The pair (titleID,ordering) is the primary key. NameID is a foreign key relating to name.basics.
  Principal cast members of a title.
* title.ratings(titleID, averageRating, numVotes)
  
  The primary key is titleID.
  This relation has ratings for a number of titles.
* name.basics(nameID, primaryName, birthYear, deathYear)
  * name.basics_profession(nameID, profession)
  * name.basics_knownFor(nameID, titleID)
  
  The primary key of "name.basics" is nameID. Both "name.basics_profession" and "name.basics_knownFor" are derived from array-fields in the original data and do not have non-key attributes.
  This relation includes basic information about people (actors, directors, writers, producers, etc.) in the database.

### Data Types

With the exception of "averageRating" (which is a double) every attribute after running "prepare.py" is of type int. Identical values that appear in different tables (but are within the same domain, i.e., titleID or nameID) are always mapped to the same integer. The conversion used for categorical attributes (which are: region, language, type, attribute, titleType, genre, category, job, and profession) is thus a simple bijection mapping a categorical value to a number. Booleans (isOriginalTitle, isAdult) are mapped to 0 and 1. Values that were already numbers (e.g. the years and runtimeMinutes) remain unchanged.

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

