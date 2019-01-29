//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_ENGINE_FADE_H_
#define INCLUDE_ENGINE_FADE_H_

#include <atomic>
#include <cmath>
// #include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <map>
#include <set>

// #include <CompilerUtils.hpp>
#include <DataTypes.hpp>
#include <DTree.h>
#include <DTreeNode.hpp>
#include <Engine.h>
#include <GlobalParams.hpp>

//! Forward declarations to allow pointer to launcher, dataHandler and FTree without cyclic includes.
class DTree;
class Launcher;
class DataHandler;

#define BUF_SIZE 1024

namespace fade 
{
    struct FadeAggregates
    {
        dfdb::types::DataType* aggregates = NULL;
        int* categoricalOffsets = NULL;
        std::vector<dfdb::types::DataType> categoricalAggregates;

        std::map<double, std::set<double>>* functionalDependencies = nullptr;
        std::set<double>* determinantValueSet = nullptr; 

        FadeAggregates() {};
        FadeAggregates(dfdb::types::DataType* aggs, int* offs, 
                       std::vector<dfdb::types::DataType> catAggs) : 
            aggregates(aggs), 
            categoricalOffsets(offs), 
            categoricalAggregates(catAggs)
        {};

        ~FadeAggregates()
        {
        }
    };

    struct CacheAggregates
    {
        dfdb::types::DataType* aggregates = nullptr;
        dfdb::types::DataType* categoricalAggregates = nullptr;
        int* categoricalOffsets = nullptr;

        CacheAggregates() {}; 
        CacheAggregates(dfdb::types::DataType* aggs, int* offs, 
			dfdb::types::DataType* catAggs) : 
            aggregates(aggs),
            categoricalAggregates(catAggs),
            categoricalOffsets(offs)
        {};

        ~CacheAggregates()
        {
        }
    };
}

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
            /* In general the compared tuples will be different earlier in the
             * comparison. */
            if (unlikely(i == 0))
            {
                break;
            }
            --i;
        }

        return a[priority[i]] < b[priority[i]];
    }
};

/**
 * Class imported from F engine.
 */
class Fade : public Engine
{

public:

    /**
     * Constructor; need to provide pointer to Launcher module.
     */
    Fade(std::shared_ptr<Launcher> launcher, unsigned int numOfThreads,
         unsigned int numOfPartitions);

    ~Fade();

    /**
     *	Precomputes aggregate info based on bottom up generation.
     */
    void run(fade::FadeAggregates& finalAggregates,
             std::vector<std::vector<int*> >& aggRegister, 
             int* agg, int* deg, bool* onehot, bool* isDetermined, int* determinedBy);
	
    void mergeCategoricalValues(std::vector<double>& out, int* outOffsets, 
                                std::vector<std::vector<double> > const &in,
                                std::vector<int*> const &inOffsets);

private:

    /**
     * Defines all the fields that are changed in a thread.
     */
    struct FadeAttributes
    {
        double* varMap;
        int** LOWERBOUND;
        int** UPPERBOUND;
        int** ordering;

        double* aggregates;
        double* localAggregates;
        double* computedAggregates;

        int* categoricalOffsets = nullptr;
        int* newOffsets = nullptr; 

        std::map<double, std::set<double>>* functionalDependencies = nullptr; 

        std::vector<double>* categoricalAggregates = nullptr;
        std::vector<double> newAggregates;
        std::vector<double> mergedAggregates;

        ~FadeAttributes()
        {
            for (size_t i = 0; i < dfdb::params::NUM_OF_ATTRIBUTES; ++i)
            {
                delete[] ordering[i];
                delete[] LOWERBOUND[i];
                delete[] UPPERBOUND[i];
            }
            delete[] ordering;
            delete[] LOWERBOUND;
            delete[] UPPERBOUND;
            delete[] aggregates;
            delete[] varMap;
            delete[] localAggregates;
            delete[] computedAggregates;

            if (categoricalOffsets != nullptr)
                delete[] categoricalOffsets;
            if (newOffsets != nullptr)
                delete[] newOffsets;
            if (categoricalAggregates != nullptr)
                delete[] categoricalAggregates;

            delete[] functionalDependencies;
        }
    };

    //! Pointer to Dtree.
    std::shared_ptr<DTree> _dTree;

    //! Data to process.
    dfdb::types::Database _data;

    //! DataHandler module of the current DFDB instance.
    std::shared_ptr<DataHandler> _dataHandler;

    //! The ID vectors will contain the pairs of INDEXES in the array
    //! attributes-attr.  For each attribute KEY-index we append the pairs of
    //! indexes having that key so we will have all the pairs of
    //! <relationIndex-attributeIndex> for each attribute KEY.
    std::vector<std::pair<int, int> >* _ids;

    //! Cache related objects.
    std::unordered_map<std::vector<double>, fade::CacheAggregates> _caches;

    //! Array stores the degree of the local aggregates at each node.
    int* _degreeOfLocalAggregates;

    //! Array defines the number of aggregates at each node in dtree
    int* _numberOfAggregatesPerNode;

    //! aggregateInfo is used as a performance improvement for better
    //! accessing of aggregateInfoPerNode
    int** _aggregateRegister;

    //! mutex used to lock thread safe cache map.
    std::mutex _cacheMutex;

    //! Number of threads to spawn in multithreaded mode.
    unsigned int _numOfThreads;

    //! Number of partitions to work on in multithreaded mode.
    unsigned int _numOfPartitions;

    //! Count the number of Values in the factorized join.
    unsigned int _numberOfValues;
    
    //! Current partition for a worker to deal with; atomic to enable lock-free
    //! usage across several threads.
    std::atomic_uint_fast32_t _nextPartition;

    //! Array of pointers to the linear regression modules (one for each
    //! partition).
    FadeAttributes* _aggregatesPerPartition;

    /* For each feature states whether it is one hot encoded */
    bool* _onehotfeatures;

    int* _degreeOfCategoricalAggregateGroup; 

    bool* isDetermined; 
    int* determinedBy; 

    /**
     *	Computes the aggregates based on aggregate register.
     */
    void runAggregator(int* lowerBounds, int* upperBounds, unsigned int partition);

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
    HOT size_t leapfroggingJoin(DTreeNode* node, int* lower, int* upper,
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

    void computeAggregatesNonLeaf(DTreeNode* node, double val, unsigned int partition);

    struct Pointers {
        int id; 
        int degreeCategorical;
        std::vector<double>::const_iterator begin;
        std::vector<double>::const_iterator end;
        std::vector<double>::const_iterator me;
    };

    HOT inline void cartesianProduct(
        std::vector<double>& out, std::vector<Pointers>& in, double contAgg,
        bool nodeIsCat, double nodeValue);

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

};

#endif /* INCLUDE_ENGINE_FADE_H_ */
