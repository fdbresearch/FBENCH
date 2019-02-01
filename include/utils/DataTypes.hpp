//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_DATATYPES_HPP_
#define INCLUDE_DATATYPES_HPP_

#include <vector>

namespace dfdb
{
/**
 * Contains type definitions for the data structures used in DFDB.
 */
namespace types
{

//! Data type used for most operations.
typedef double DataType;
//! Data type used for database tuples.
typedef DataType* Tuple;
//! Data type used for database tables; size of the tables not known in advance and can vary depending on tuples received by other nodes.
typedef std::vector<Tuple> Table;
//! Data type used to store all the tables.
typedef Table* Database;

//! Data type used for the result.
typedef double ResultType;
//! Data type used to store the results.
typedef std::vector<ResultType> ResultStructure;

}
}

#endif /* INCLUDE_DATATYPES_HPP_ */
