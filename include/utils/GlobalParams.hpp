//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef GLOBALPARAMS_HPP_
#define GLOBALPARAMS_HPP_

namespace dfdb
{

namespace params
{

/* Compile time constant: can be used directly to enable loop unrolling and other compiler optimisations. */
#ifdef WORKERS
const size_t NUM_OF_WORKERS = WORKERS;
/* Runtime variable: can be used across different compilation units; must be defined in a source file. */
#else
extern size_t NUM_OF_WORKERS;
#endif

/* Compile time constant: can be used directly to enable loop unrolling and other compiler optimisations. */
#ifdef FEATURES
const size_t NUM_OF_FEATURES = FEATURES;
/* Runtime variable: can be used across different compilation units; must be defined in a source file. */
#else
extern size_t NUM_OF_FEATURES;
#endif

/* Compile time constant: can be used directly to enable loop unrolling and other compiler optimisations. */
#ifdef LFEATURES
const size_t NUM_OF_LINEAR_FEATURES = LFEATURES;
/* Runtime variable: can be used across different compilation units; must be defined in a source file. */
#else
extern size_t NUM_OF_LINEAR_FEATURES;
#endif

/* Compile time constant: can be used directly to enable loop unrolling and other compiler optimisations. */
#ifdef MANUAL
const size_t NUM_OF_MANUAL_INTERACTIONS = MANUAL;
/* Runtime variable: can be used across different compilation units; must be defined in a source file. */
#else
extern size_t NUM_OF_MANUAL_INTERACTIONS;
#endif

/* Compile time constant: can be used directly to enable loop unrolling and other compiler optimisations. */
#ifdef DEGREE
const size_t DEGREE_OF_INTERACTIONS = DEGREE;
/* Runtime variable: can be used across different compilation units; must be defined in a source file. */
#else
extern size_t DEGREE_OF_INTERACTIONS;
#endif

/* Compile time constant: can be used directly to enable loop unrolling and other compiler optimisations. */
#ifdef TABLES
const size_t NUM_OF_TABLES = TABLES;
/* Runtime variable: can be used across different compilation units; must be defined in a source file. */
#else
extern size_t NUM_OF_TABLES;
#endif

/* Compile time constant: can be used directly to enable loop unrolling and other compiler optimisations. */
#ifdef ATTRIBUTES
const size_t NUM_OF_ATTRIBUTES = ATTRIBUTES;
/* Runtime variable: can be used across different compilation units; must be defined in a source file. */
#else
extern size_t NUM_OF_ATTRIBUTES;
#endif

}
}

#endif /* GLOBALPARAMS_HPP_ */
