//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#include <cstdlib>
#include <string>
#include <vector>

#include <GlobalParams.hpp>
#include <Launcher.h>
#include <Logging.hpp>

#include <CountOverJoin.h>

using namespace std::chrono;

/**
 * Constructor; need to provide path to configuration files, pointer to Launcher module in order to retrieve DataHandler instance,
 * and number of threads and partitions to work on.
 */
CountOverJoin::CountOverJoin(
    const std::string& pathToFiles,std::shared_ptr<Launcher> launcher) : 
    _pathToFiles(pathToFiles), _launcher(launcher), _dTree(launcher->getDTree())
{
};

CountOverJoin::~CountOverJoin(){};

/**
 * Runs F to compute a regression model.
 */
void CountOverJoin::run() 
{
    _joinEngine =
        std::dynamic_pointer_cast<FactorisedJoin>(_launcher->getAggregateEngine());

#ifdef BENCH
    int64_t startJoin = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
#endif

    _joinEngine->run();

    rootNode = _joinEngine->getFactorisationRoot();

    BINFO(
        "WORKER - join construction: " + std::to_string(
            duration_cast<milliseconds>(
                system_clock::now().time_since_epoch()).count() - startJoin
            ) + "ms.\n"
        );

    DINFO("WORKER - join has been constructed. \n");

#ifdef BENCH
    int64_t startCount = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
#endif

    unsigned short numberOfCachedValues = _joinEngine->getNumberOfCachedValues();

    cachedCounts = new unsigned int[numberOfCachedValues]();

    unsigned int tupleCount = count(rootNode, _dTree->_root); 

    BINFO(
        "WORKER - count calculation: " + std::to_string(
            duration_cast<milliseconds>(
                system_clock::now().time_since_epoch()).count() - startCount) +
        "ms.\n");


    numberOfValues = 0;
    memset(cachedCounts, 0, numberOfCachedValues * sizeof(unsigned int));
    valueCount(rootNode, _dTree->_root);

    // std::cout << "Number of cached values: "+std::to_string(numberOfCachedValues)+"\n";
    std::cout << "DATA - Number of values in Factorised Representation: "+ std::to_string(numberOfValues) +".\n";

    size_t flatValueCount = ((size_t)tupleCount) * dfdb::params::NUM_OF_ATTRIBUTES;
    std::cout << "DATA - Number of values in Listing Representation: "+std::to_string(flatValueCount)+".\n"; 

    std::cout << "DATA - Compression Factor: "+ std::to_string((double)flatValueCount / numberOfValues)+".\n";

    assert(tupleCount == rootNode -> count);

    std::cout << "DATA - Number of Tuples in join result: " <<
        (size_t) tupleCount << std::endl;
    // exit(0);
};


unsigned int CountOverJoin::count(node::Union* factorisationNode, DTreeNode* dTreeNode)
{
    int numOfChildren = dTreeNode -> _numOfChildren; 

    if (numOfChildren == 0) 
        return factorisationNode->numberOfValues;

    if (dTreeNode->_caching && cachedCounts[factorisationNode->cacheIndex] > 0)
        return cachedCounts[factorisationNode->cacheIndex];

    unsigned int totalCount = 0;
    for (unsigned long val = 0; val < factorisationNode->numberOfValues; ++val)
    {
        unsigned int valCount = 1; 
        DTreeNode* child = dTreeNode->_firstChild; 
        for (int i = 0; i < numOfChildren; ++i)
        {
            valCount *=
                count(factorisationNode->children[val*numOfChildren + i], child);

            child = child->_next;
        }
        totalCount += valCount;
    }

    if (dTreeNode->_caching)
        cachedCounts[factorisationNode->cacheIndex] = totalCount;

    return totalCount; 
};

void CountOverJoin::valueCount(node::Union* factorisationNode, DTreeNode* dTreeNode)
{
    if (dTreeNode->_caching)
    {
        if (cachedCounts[factorisationNode->cacheIndex] > 0)
            return;

        cachedCounts[factorisationNode->cacheIndex] = 1;
    }

    numberOfValues += factorisationNode->numberOfValues;

    int numOfChildren = dTreeNode -> _numOfChildren;
    for (unsigned long val = 0; val < factorisationNode->numberOfValues; ++val)
    {
        DTreeNode* child = dTreeNode->_firstChild; 
        for (int i = 0; i < numOfChildren; ++i)
        {
            valueCount(factorisationNode->children[val * numOfChildren + i], child);
            child = child->_next;
        }
    }
};
