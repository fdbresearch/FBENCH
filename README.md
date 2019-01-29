# FBench

This repository contains an implementation of factorised databases focused on computing the size of a join result and the resulting compression factor, based on the TODS 2015 paper [Size Bounds for Factorised Representations of Query Results](http://www.cs.ox.ac.uk/dan.olteanu/papers/oz-tods15.pdf).

The aim of factorised databases is to reduce the redundancy appearing in query results of conjunctive queries. By distributing the products over unions it is often possible to avoid enumerating Cartesian products that appear in a join result. This is achieved using analysis of the query given an order of the variables used (d-tree). The result is a linear representation of an otherwise quadratic result. This representation is amenable for further computation and can be used o achieve significant performance improvements when training certain machine learning models directly on the database.

The implementation in this repository focuses on computing the size of a factorised representation given a database and a query/variable order. It returns a compression factor computed as  `C = (A*T)/V` where

* `A` is the number of attributes in the query result (i.e., the number of variables),
* `T` is the number of tuples in the query result, and
* `V` is the number of singleton values in the factorised representation of the result.

More information on the principles of factorised databases can be found [on the main page](https://fdbresearch.github.io/principles.html).

## Compiling

Ensure you have a working development environment. Windows is not supported by the current code base. The project is known to run on recent versions of the GCC and Clang compilers, but may also be successfully built with other compilers. 

If not already installed, use one of the following commands to install the GNU C/C++ compiler and other utilities.

For Fedora, Red Hat, CentOS, or Scientific Linux:
```
$ sudo yum groupinstall 'Development Tools'
```

For Debian or Ubuntu:
```
$ sudo apt-get install build-essential
```

For Mac OS X install Xcode from the app store and then run
```
$ xcode-select --install
```
from a terminal to install the command-line tools.

The build-process uses CMake to configure the makefile. To install CMake, please refer to [this page](https://cmake.org/install/). The code requires the Boost library to work. Visit [Boost Getting started guide for Linux](http://www.boost.org/doc/libs/1_61_0/more/getting_started/unix-variants.html) for information on how to install it, in particular step 5.1. The package management of your system may also have binaries available. This project uses the *system*, *iostreams*, *program_options* and *spirit* libraries of Boost.

Once you fulfil the prerequisites, to compile FBench for the first time (or after adding new files to the project) navigate to the directory the repository is cloned in and run:
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Running FDB

The simplest way to run FBench is to type the following command:
```
./fbench --path pathToData
```

*pathToData* must contain the database files as well as the configuration files required (*schema.conf* and *dtree.txt*, see below). Make sure the settings specified in the files are correct, though some sanity checks for the tree specified by *dtree.txt* are done by FBench. The output of this process will include the size of the listing representation of the join-result (`A*T` in the formula for the compression factor above), the number of singleton values in the factorised representation (`V`), and the resulting compression factor.

Other command line options are available. Run the following command to view them:
```
./dfdb --help
```

In particular there are two options for the model: the default model "count" only counts the sizes of the join result and yields a compression factor, but without explicitly constructing the result. The alternative "countJoin" constructs the join explicitly and then prints the sizes to the standard output.

## Data and Configuration Files

The following three sections briefly describe the input format and configuration files needed to define the query and the variable order of the factorised representation. A simple example is included in the `example` directory. To run it use
```
$ fbench --path ../example
```

### D-Tree configuration

The variable order (*d-tree*) is given in the file "dtree.txt". It has the following format:

1 The first line contains two numbers: number_of_variables and number_of_relations, separated by space.
1 Then list each variable in the dtree using the following format:
  id name type parent dtree-key caching-boolean
  * The id is used to identify this attribute later on, and should be ordered from 0 to number_of_variables-1.
  * name is the name of the variable as specified in schema.conf and below.
  * dtree-key is of the format "{list_of_ids}", where the list of id's is a comma-separated list of node-id's of the tree.
  * The parent of the root set to -1, otherwise the id of the parent node.
  * The caching-boolean indicates if we can cache at this node in the dtree.
1 List each [relation_name], with the id of the variable in this relation that is the lowest node in the dtree, and
  the list of variables in this relation (comma-separated, no whitespace).

### Relational Schema

The query is implied by the schema given: a natural join is computed over all relations. In *schema.conf* the relations are listed in the following format (note, the names given here to relations and attributes need to be the same as used in dtree.txt). There is one relation per line.
```
[relation_name]:{List_of_variables}
```

The list of variables is a comma-separated list, without any whitespace. It is possible to list a relation multiple times with different variable names to implement joins on different attributes, or cyclic queries.

### Data input format

The data is expected by FBench to be in text-files of the following format:
```
r11|r12|r13|r14
r21|r22|r23|r24
...
rn1|rn2|rn3|rn4
```

This specifies a table of `n` rows and 4 columns. Only integer and numeric datatypes are supported. The name of the file is expected to be "[relation_name].tbl" using the same relation_name that is also used in schema.conf and dtree.txt.
