//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_JOINENGINE_FACJOIN_H_
#define INCLUDE_JOINENGINE_FACJOIN_H_

#include <atomic>
#include <cmath>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <DataTypes.hpp>
#include <DTree.h>
#include <DTreeNode.hpp>
#include <Engine.h>

//! Forward declarations to allow pointer to launcher, dataHandler and FTree without cyclic includes.
class DTree;
class Launcher;
class DataHandler;

#define BUF_SIZE 1024

namespace node
{
	struct Union
	{
		double *values;
		Union* *children;

		unsigned long numberOfValues;
		unsigned short cacheIndex;
		unsigned int count;
		unsigned int multiplicity;

		double* sums;
		int* sumIndexes;

		~Union()
		{
			delete[] values;
			delete[] children;		// TODO : how do we delete the actual children, not just pointers? 
		}
	};
}

/**
 * Class imported from F engine.
 */
class FactorisedJoin : public Engine
{

public:

	/**
	 * Constructor; need to provide pointer to Launcher module.
	 */
	FactorisedJoin(std::shared_ptr<Launcher> launcher, unsigned int numOfThreads, unsigned int numOfPartitions);

	~FactorisedJoin();

	/**
	 *	Constructs the factorisation based on the query provided by the DTree. 
	 */
	void run();

	unsigned short getNumberOfCachedValues()
	{
		return numberOfCachedValues; 
	}

	node::Union* getFactorisationRoot()
	{
		return rootNode; 
	}

private:

	/**
	 * Defines all the fields that are changed in a thread.
	 */
	struct PartitionAttributes
	{
		double* varMap;
		int** LOWERBOUND;
		int** UPPERBOUND;
		int** ordering;

		node::Union*** localPointerPlaceholder;
		std::vector<node::Union*>* pointers;
		std::vector<double>* values;
		std::vector<double>* localCount;

		~PartitionAttributes()
		{
			for (size_t i = 0; i < dfdb::params::NUM_OF_ATTRIBUTES; ++i)
			{
				delete[] ordering[i];
				delete[] LOWERBOUND[i];
				delete[] UPPERBOUND[i];
				delete[] localPointerPlaceholder[i];
			}

			delete[] ordering;
			delete[] LOWERBOUND;
			delete[] UPPERBOUND;
			delete[] varMap;

			delete[] pointers;
			delete[] values;
			delete[] localPointerPlaceholder;
		}
	};

	//! Pointer to Dtree.
	std::shared_ptr<DTree> _dTree;

	//! Data to process.
	dfdb::types::Database _data;

	//! DataHandler module of the current DFDB instance.
	std::shared_ptr<DataHandler> _dataHandler;

	//! The ID vectors will contain the pairs of INDEXES in the array attributes-attr.
	//! For each attribute KEY-index we append the pairs of indexes having that key
	//! so we will have all the pairs of <relationIndex-attributeIndex> for each attribute KEY.
	std::vector<std::pair<int, int> >* _ids;

	//! Cache related objects.	
	std::unordered_map<std::vector<double>, node::Union*> _caches; 					// TODO create the cache 

	//! mutex used to lock thread safe cache map.
	std::mutex _cacheMutex;

	//! Number of threads to spawn in multithreaded mode.
	unsigned int _numOfThreads;

	//! Number of partitions to work on in multithreaded mode.
	unsigned int _numOfPartitions;

	//! Current partition for a worker to deal with; atomic to enable lock-free usage across several threads.
	std::atomic_uint_fast32_t _nextPartition;

	PartitionAttributes* _partitionFields = nullptr;
	
	node::Union** rootPerPartition = nullptr;

	unsigned int numberOfCachedValues;

	node::Union* rootNode = nullptr;

	unsigned long numberOfValues = 0;

	/**
	 *	Computes the aggregates based on aggregate register.
	 */
	node::Union* runAggregator(int* lowerBounds, int* upperBounds, unsigned int partition);

	/**
	 * Provides ordering of relation - done once for each leapfrogging join.
	 */
	HOT void getRelationOrdering(int* left, DTreeNode* node, int* ordering);

	/**
	 * Seeks least upper bound.
	 */
	HOT bool seekValue(DTreeNode* node, int &p, int* pos, int pMax, int* &l,
			int* r, double &val);

	/**
	 * Performs leap frog join.
	 * lower / upper : lower and upper ranges for each relation
	 */
	HOT node::Union* leapfroggingJoin(DTreeNode* node, int* lower, int* upper,
			unsigned int partition);

	/**
	 * Sorts the data to process according to an order given by the FTree.
	 * Depending on the compiler, this will or will not be done in parallel.
	 */
	void sortDataToProcess();

	/**
	 * Returns the index of the table on which the partitioning must be done.
	 * We partition on a table that contains the root attribute of the FTree.
	 * Partitioning on another attribute will negatively affect F's caching system.
	 */
	size_t getTableToPartition();

	/**
	 * Implementation of mod that also works for negative numbers.
	 */
	HOT inline int mod(int x, int y)
	{
		int result = x % y;
		return result < 0 ? result + y : result;
	}

	/**
	 * Structure used for sorting relations (used in getRelationOrdering).
	 */
	struct sort_pred
	{
		bool operator()(const std::pair<int, double> &left,
				const std::pair<int, double> &right)
		{
			return left.second < right.second;
		}
	};

	/**
	 * Structure used to order the data to process.
	 */
	struct ValueOrdering
	{
		//! Priority array used to sort the data.
		const int_fast16_t* priority;
		//! Corresponds to the last index in the priority array.
		const size_t priorityMaxIndex;

		ValueOrdering(int_fast16_t* pr, size_t prSize) :
				priority(pr), priorityMaxIndex(prSize - 1)
		{
		}

		HOT inline bool operator()(dfdb::types::Tuple a, dfdb::types::Tuple b)
		{
			uint_fast16_t i = priorityMaxIndex;

			/* In general we expect values to be different. */
			while (unlikely(a[priority[i]] == b[priority[i]]))
			{
				/* In general the compared tuples will be different earlier in the comparison. */
				if (unlikely(i == 0))
				{
					break;
				}
				--i;
			}

			return a[priority[i]] < b[priority[i]];
		}
	};
};

#endif /* INCLUDE_JOINENGINE_FACJOIN_H_ */
