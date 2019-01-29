//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_ENGINE_HPP_
#define INCLUDE_ENGINE_HPP_

#include <memory>

#include <DataHandler.hpp>
#include <CompilerUtils.hpp>
#include <GlobalParams.hpp>

namespace std
{
/**
 * Custom hash function for cache data stucture.
 */
template<> struct hash<vector<double>>
{
	HOT inline size_t operator()(const vector<double>& p) const
	{
		size_t seed = 0;
		hash<double> h;
		for (double d : p)
			seed ^= h(d) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};
/**
 * Custom hash function for cofactor aggregate index data stucture.
 */
template<> struct hash<vector<int>>
{
	HOT inline size_t operator()(const vector<int>& p) const
	{
		size_t seed = 0;
		hash<int> h;
		for (int d : p)
			seed ^= h(d) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};
}

/**
 * Abstract class to provide a uniform interface used to process the received data.
 */
class Engine
{

public:

	virtual ~Engine()
	{
	}

protected:

};

#endif /* INCLUDE_ENGINE_HPP_ */
