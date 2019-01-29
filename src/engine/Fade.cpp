//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#include <algorithm>
#include <cstring>
#include <fstream>

#include <Fade.h>
#include <Logging.hpp>
#include <Launcher.h>

using namespace dfdb::params;
using namespace dfdb::types;
using namespace std;
using namespace chrono;
using namespace fade;

Fade::Fade(shared_ptr<Launcher> launcher, unsigned int numOfThreads,
    unsigned int numOfPartitions) :
    _dTree(launcher->getDTree()), _dataHandler(launcher->getDataHandler()),
    _numOfThreads(numOfThreads), _numOfPartitions(numOfPartitions)
{
    _data = _dataHandler->getDataToProcess();

    /*
     * If no paramater was set by the user, the default retrieves the number of
     * hardware thread contexts; we will spawn as many threads for partitioning
     * the work done by F. The function can return 0 if value not available.
     * Use one thread in that case.
     */
    if (_numOfThreads == 0)
        _numOfThreads = 1;

    /* Do not do any partitioning if single thread. */
    if (_numOfThreads == 1)
        _aggregatesPerPartition = new FadeAttributes[1];
    else
        _aggregatesPerPartition = new FadeAttributes[_numOfPartitions];

    /* First partition to work on is number 0. */
    if (_numOfThreads != 1)
        _nextPartition.store(0, memory_order_relaxed);
    
#ifdef BENCH
    int64_t startSorting = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
#endif

    sortDataToProcess();
    
#ifdef BENCH
    int64_t endSorting = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count() - startSorting;    
#endif

    BINFO("TIME - data sorting: " + to_string(endSorting) + "ms.\n");

}

Fade::~Fade()
{
    delete[] _ids;

    for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
        delete[] _aggregateRegister[i];
    delete[] _aggregateRegister;
}



void Fade::run(
    FadeAggregates& finalAggregates, std::vector<std::vector<int*> >& aggregateRegister,
    int* numberOfAggregatesPerNode, int* degreeOfLocalAggregates, bool* _onehotfeatures,
    bool* isDetermined, int* determinedBy)
{
    int rootID = _dTree->_root->_id;
    
    _numberOfAggregatesPerNode = numberOfAggregatesPerNode;
    _degreeOfLocalAggregates = degreeOfLocalAggregates;

    this->_onehotfeatures = _onehotfeatures;
    this->isDetermined = isDetermined; 
    this->determinedBy = determinedBy; 

    _degreeOfCategoricalAggregateGroup =
        new int[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 4 - 1] +
                _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 - 1]];
    
    _numberOfValues = 0;
    
    /*
     * _aggregateRegister is used as a performance improvement for better
     * accessing of aggregateInfoPerNode
     */
    _aggregateRegister = new int*[NUM_OF_ATTRIBUTES];

    for (size_t attID = 0; attID < NUM_OF_ATTRIBUTES; ++attID)
    {
        int childCount = _dTree->getNode(attID)->_numOfChildren;

        int totalNumberOfAggregates =
            _numberOfAggregatesPerNode[attID] * (childCount + 2) +
            _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + attID] *
            (childCount + 3);

        _aggregateRegister[attID] = new int[totalNumberOfAggregates];

        int *info = _aggregateRegister[attID];
        int index = 0;

        for (int j = 0; j < 2; ++j)
        {
            for (int i = 0; i < _numberOfAggregatesPerNode[attID]; ++i)
            {
                info[index] = aggregateRegister[attID][i][j];
                ++index;
            }
        }

        for (int i = 0; i < _numberOfAggregatesPerNode[attID]; ++i)
        {
            for (int j = 2; j < childCount + 2; ++j)
            {
                info[index] = aggregateRegister[attID][i][j];
                ++index;
            }
        }

        for (int j = 0; j < 2; ++j)
        {
            for (int i = 0; i <
                     numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + attID]; ++i)
            {
                info[index] =
                    aggregateRegister[attID][_numberOfAggregatesPerNode[attID] + i][j];
                ++index;
            }
        }

        for (int i = 0; i <
                 _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + attID]; ++i)
        {
            for (int j = 2; j < childCount + 3; ++j)
            {
                info[index] =
                    aggregateRegister[attID][_numberOfAggregatesPerNode[attID] + i][j];
                ++index;
            }
        }
    }

        
    for (int attID = NUM_OF_ATTRIBUTES - 1; attID >= 0; --attID)
    {
        int childCount = _dTree->getNode(attID)->_numOfChildren;
        int *childIDs = _dTree->getNode(attID)->_childrenIDs;
        int catAggOffset = _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + attID];

        int numberOfContAggregates = _numberOfAggregatesPerNode[attID];
        int numberOfCatAggregates =
            _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES*2 + attID];

        for (int catAggNo = 0; catAggNo < numberOfCatAggregates; ++catAggNo)
        {
            int numCatParts = 0;
            for (int child = 0; child < childCount; ++child)
            {
                if (aggregateRegister[attID][numberOfContAggregates + catAggNo][child + 2] >=
                    _numberOfAggregatesPerNode[childIDs[child]])
                {

                    numCatParts +=
                        _degreeOfCategoricalAggregateGroup[
                            _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + childIDs[child]] +
                            aggregateRegister[attID][numberOfContAggregates + catAggNo][child + 2] -
                            _numberOfAggregatesPerNode[childIDs[child]]];
                }
            }

            if (aggregateRegister[attID][numberOfContAggregates + catAggNo][childCount + 2] == 1)
                numCatParts += 1;

            _degreeOfCategoricalAggregateGroup[catAggOffset + catAggNo] = numCatParts;
        }
    }
    
    /* Spawn threads and do partitioning. */
    if (_numOfThreads > 1)
    {
        /* Array containing threads to run F on different partitions. */
        thread* fWorkers = new thread[_numOfThreads];

        /* Table to partition (only one table is partitioned to guarantee join
         * correctness). */
        size_t tableToPartition = getTableToPartition();

        DINFO(
            "INFO - engine: launching " + to_string(_numOfThreads)
            + " threads to process " + to_string(_numOfPartitions)
            + " partitions on table " + to_string(tableToPartition)
            + ".\n");

        /* This is needed for the lambda expression below - does not accept a
         * variable that is out of scope. */
        Database dataToProcess = _data;
        
        /* Launch threads to work on different partitions of the input data. */
        for (unsigned int fWorker = 0; fWorker < _numOfThreads; ++fWorker)
        {
            fWorkers[fWorker] =
                thread(
                    [this, dataToProcess, tableToPartition]()
                    {
                        /* Current partition to work on; increase value for next
                         * thread to fetch. */
                        unsigned int currentPartition =
                        _nextPartition.fetch_add(1, memory_order_relaxed);

                        /* Iterate while not all the partitions were processed. */
                        while (currentPartition < _numOfPartitions)
                        {
                            /* Set bounds for the different tables. */
                            int* lowerBounds = new int[NUM_OF_TABLES];
                            int* upperBounds = new int[NUM_OF_TABLES];
                            for (size_t table = 0; table < NUM_OF_TABLES; ++table)
                            {
                                /* This is not a table to partition; it will be
                                 * fully processed. */
                                if(table != tableToPartition)
                                {
                                    lowerBounds[table] = 0;
                                    upperBounds[table] = _data[table].size() - 1;
                                }
                                /* Table to partition; define bounds based on
                                 * current partition number. */
                                else
                                {
                                    lowerBounds[table] = (int)(_data[table].size() * ((double) currentPartition/_numOfPartitions));
                                    upperBounds[table] = (int)(_data[table].size() * ((double) (currentPartition+1)/_numOfPartitions)) - 1;
                                }

                            }


                            /* Launch Fade to compute Aggregates. */
                            runAggregator(lowerBounds, upperBounds, currentPartition);

                            /* Retrieve and increase value of current partition
                             * to work on. */
                            currentPartition = _nextPartition.fetch_add(1, memory_order_relaxed);

                            /* Clean up bound arrays. */
                            delete[] lowerBounds;
                            delete[] upperBounds;
                        }
                    });
        }

        /* Wait for all threads to finish their tasks. */
        for (unsigned int fWorker = 0; fWorker < _numOfThreads; ++fWorker)
        {
            fWorkers[fWorker].join();
        }

        /* Delete array of threads. */
        delete[] fWorkers;

        vector<vector<double> > aggs(_numOfPartitions);
        vector<int*> offs(_numOfPartitions);
        /* Launch threads to work on different partitions of the input data. */
        for (unsigned int partition = 0; partition < _numOfPartitions; ++partition)
        {
            aggs[partition] = _aggregatesPerPartition[partition].categoricalAggregates[rootID];			
            offs[partition] = _aggregatesPerPartition[partition].categoricalOffsets;
        }

        finalAggregates.categoricalOffsets =
            new int[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + rootID]];
        
        mergeCategoricalValues(finalAggregates.categoricalAggregates,
                               finalAggregates.categoricalOffsets, aggs, offs);
    }
    else
    {
        /* Set bounds for the different tables: they cover the whole tables */
        int* lowerBounds = new int[NUM_OF_TABLES];
        int* upperBounds = new int[NUM_OF_TABLES];
        for (size_t table = 0; table < NUM_OF_TABLES; ++table)
        {
            lowerBounds[table] = 0;
            upperBounds[table] = _data[table].size() - 1;
        }

        /* Launch Fade to compute Aggregates. */
        runAggregator(lowerBounds, upperBounds, 0);

        /* Retrieving the final categorical aggregates. */
        finalAggregates.categoricalAggregates =
            _aggregatesPerPartition[0].categoricalAggregates[rootID];

        /* Retrieving the final number of categorical aggregate groups. */
        int numCatAggs = _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + rootID];

        /* Retrieving the final offsets for each categorical aggregate group. */
        if (numCatAggs > 0)
        {
            finalAggregates.categoricalOffsets = new int[numCatAggs];
            memcpy(finalAggregates.categoricalOffsets,
                   &_aggregatesPerPartition[0].categoricalOffsets[
                       _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + rootID]],
                   sizeof(int) * numCatAggs);
        }

        /* Clean up bound arrays. */
        delete[] lowerBounds;
        delete[] upperBounds;
    }

    int aggregateOffset = _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES + rootID];

    double numOfTuples = 0.0; 
    for (unsigned int partition = 0; partition < _numOfPartitions; ++partition)
    {
        numOfTuples += _aggregatesPerPartition[partition].aggregates[0];
    }

    /* Retrieve the aggregates from all the partitions and sum them up. */
    double* finalContinuousAggregates =
        new double[_numberOfAggregatesPerNode[rootID]]();

    for (unsigned int partition = 0; partition < _numOfPartitions; ++partition)
    {
        for (int aggregateNo = 0; aggregateNo <
                 _numberOfAggregatesPerNode[rootID]; ++aggregateNo)
            finalContinuousAggregates[aggregateNo] +=
                _aggregatesPerPartition[partition].aggregates[aggregateOffset + aggregateNo];
    }

    finalAggregates.aggregates = finalContinuousAggregates;


    finalAggregates.functionalDependencies = new map<double, set<double>>[NUM_OF_ATTRIBUTES];

    int numOfFDs = determinedBy[NUM_OF_ATTRIBUTES];
    finalAggregates.determinantValueSet = new set<double>[numOfFDs];

    if(numOfFDs > 0)
    {
        /* We combine the FDs from different partitions & collect the set of determinant values. */
        for (unsigned int partition = 0; partition < _numOfPartitions; ++partition)
        {
            for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
            {
                if (isDetermined[i])
                {
                    int determinantID = determinedBy[i];
                    for (auto& p : _aggregatesPerPartition[partition].functionalDependencies[i])
                    {
                        finalAggregates.determinantValueSet[determinedBy[determinantID]].insert(p.second.begin(), p.second.end());

                        auto insertPair = finalAggregates.functionalDependencies[i].insert(p);
                        if (!insertPair.second)
                            insertPair.first->second.insert(p.second.begin(), p.second.end());
                    }
                }
            }
        }
    }

    std::cout << "DATA - Number of values: " << _numberOfValues << "\n";
    std::cout << "DATA - Number of values in Listing Representation: " <<
        long(finalAggregates.aggregates[0] * NUM_OF_ATTRIBUTES) << std::endl;
    std::cout << std::fixed << "DATA - Compression Factor: "
              << (finalAggregates.aggregates[0] * NUM_OF_ATTRIBUTES) / _numberOfValues
              << std::endl;   
}

void Fade::runAggregator(int* lowerBounds, int* upperBounds, unsigned int partition)
{
    DTreeNode* root = _dTree->_root;

    /* Initialises the array for continuous aggregates. */
    _aggregatesPerPartition[partition].aggregates =
        new double[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 - 1]
                   + _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES - 1]];

/**********************************************************************************/
    /* Initialises the array for categorical aggregates. */
    _aggregatesPerPartition[partition].categoricalAggregates =
        new vector<double>[NUM_OF_ATTRIBUTES];

    for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
        _aggregatesPerPartition[partition].categoricalAggregates[i].reserve(10000);

    _aggregatesPerPartition[partition].categoricalOffsets =
        new int[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 4 - 1]
                + _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 - 1]]();

    _aggregatesPerPartition[partition].newOffsets =
        new int[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + root->_id]]();

/**********************************************************************************/

    /* Initialize varMap to required size and set entries to zero. */
    _aggregatesPerPartition[partition].varMap = new double[NUM_OF_ATTRIBUTES]();

    int highestDegreeLocalAggregate = 0;
    for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
        if (highestDegreeLocalAggregate < _degreeOfLocalAggregates[i])
            highestDegreeLocalAggregate = _degreeOfLocalAggregates[i];

    _aggregatesPerPartition[partition].localAggregates =
        new double[highestDegreeLocalAggregate + 1];

    _aggregatesPerPartition[partition].computedAggregates =
        new double[_numberOfAggregatesPerNode[root->_id] +
                   _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + root->_id]];

    _aggregatesPerPartition[partition].LOWERBOUND = new int*[NUM_OF_ATTRIBUTES];
    _aggregatesPerPartition[partition].UPPERBOUND = new int*[NUM_OF_ATTRIBUTES];
    for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
    {
        _aggregatesPerPartition[partition].LOWERBOUND[i] = new int[NUM_OF_TABLES];
        _aggregatesPerPartition[partition].UPPERBOUND[i] = new int[NUM_OF_TABLES];
    }

    _aggregatesPerPartition[partition].ordering = new int*[NUM_OF_ATTRIBUTES];
    for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
        _aggregatesPerPartition[partition].ordering[i] = new int[_ids[i].size()];


    _aggregatesPerPartition[partition].functionalDependencies =
        new map<double, set<double>>[NUM_OF_ATTRIBUTES];

    DINFO("Before leapfrogging join! \n");

#ifdef BENCH
    int64_t startF = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
#endif

    /* This performs the join and recursively updates the regression aggregates. */
    _numberOfValues += leapfroggingJoin(root, lowerBounds, upperBounds, partition);
        
    BINFO(
        "TIME - Aggregate computation only: " +
        to_string(duration_cast<milliseconds>(
                      system_clock::now().time_since_epoch()).count()-startF) +"ms.\n");
}

bool Fade::seekValue(DTreeNode* node, int &rel, int* ordering, int numOfRel,
		int* &l, int* u, double &val)
{
	int nodeID = node->_id;

	int index = mod(rel - 1, numOfRel);
	int i = _ids[nodeID][ordering[index]].first;
	int j = _ids[nodeID][ordering[index]].second;

	/* this is the value we are seeking. */
	double max = _data[i][l[i]][j];

	double min;
	while (true)
	{
		i = _ids[nodeID][ordering[rel]].first;
		j = _ids[nodeID][ordering[rel]].second;
		min = _data[i][l[i]][j];

		/* If we found the value then we return. */
		if (min == max)
		{
			val = min;
			return false;
		}
		/* else we seek a the least upper bound */
		else
		{
			/* If the value we seek is bigger than the last value in the relation we can return. */
			if (max > _data[i][u[i]][j])
				return true;

			/* We seek the value with increasing leaps. */
			int leap = 1;
			while (l[i] <= u[i] && min < max)
			{
				if (l[i] + leap < u[i])
				{
					l[i] += leap;
					min = _data[i][l[i]][j];
					if (min < max)
						leap *= 2;
				}
				else
				{
					leap = u[i] - l[i];
					l[i] = u[i];
					min = _data[i][l[i]][j];
					break;
				}
			}

			/*
			 * When we found an upper bound we need to find the least upper bound;
			 * we backtrack using binary search.
			 */
			if (leap > 1 && max <= _data[i][l[i] - 1][j])
			{
				int high = l[i], low = l[i] - leap, mid = 0;
				while (high > low && high != low)
				{
					mid = (high + low) / 2;
					if (max > _data[i][mid - 1][j] && max <= _data[i][mid][j])
					{
						l[i] = mid;
						break;
					}
					else if (max <= _data[i][mid][j])
						high = mid - 1;
					else
						low = mid + 1;
				}

				mid = (high + low) / 2;
				if (_data[i][mid - 1][j] >= max)
					mid -= 1;

				l[i] = mid;
			}

			/*
			 * Once the least upper bound is found we set max to that value and
			 * continue with the next relation.
			 */
			max = _data[i][l[i]][j];
			rel = (rel + 1) % numOfRel;
		}
	}

	return false;
}

size_t Fade::leapfroggingJoin(
    DTreeNode* node, int* lower, int* upper, unsigned int partition)
{
    int_fast16_t nodeID = node->_id;
    bool caching = node->_caching;
    
    double* varMap = _aggregatesPerPartition[partition].varMap;

    int numberOfAggregates = _numberOfAggregatesPerNode[nodeID];
    int numberOfCategoricalAggregates =
        _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + nodeID];

    int childrenCount = node->_numOfChildren;

    double* aggregates = _aggregatesPerPartition[partition].aggregates;

    int* categoricalOffsets = _aggregatesPerPartition[partition].categoricalOffsets;
    int* newOffsets = _aggregatesPerPartition[partition].newOffsets;

    vector<double>* categoricalAggregates =
        _aggregatesPerPartition[partition].categoricalAggregates;

    /* If this node allows for caching we check if the aggregate array has been
     * cached already. */
    if (caching)
    {
        /*
         * We initialise the elements in the vector to nodeID instead of
         * default-initialising them. The last element will not be overwritten.
         */
        vector<double> keyVals(node->_key.size() + 1, nodeID);
        for (size_t i = 0; i < node->_key.size(); ++i)
            keyVals[i] = varMap[node->_key[i]];

        lock_guard<mutex> lock(_cacheMutex);

        auto it = _caches.find(keyVals);
        if (it != _caches.end())
        {
            /* The aggregate array has been cached before. */
            CacheAggregates& cache = it->second;
            memcpy(&aggregates[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES + nodeID]],
                   cache.aggregates,sizeof(double) * numberOfAggregates);

            if (numberOfCategoricalAggregates > 0)
            {
                memcpy(&categoricalOffsets[_numberOfAggregatesPerNode[
                               NUM_OF_ATTRIBUTES * 3 + nodeID]],
                       cache.categoricalOffsets,
                       sizeof(int) * numberOfCategoricalAggregates);
            
                categoricalAggregates[nodeID].resize(
                    cache.categoricalOffsets[numberOfCategoricalAggregates - 1]);

                memcpy(&categoricalAggregates[nodeID][0], cache.categoricalAggregates,
                       sizeof(double) *  cache.categoricalOffsets[
                           numberOfCategoricalAggregates - 1]);
            }
            
            return 0;
        }
    }

    size_t numberOfValues = 0;
    size_t localNumOfValues = 0;

    /* lower range pointer for each relation. */
    int* l = _aggregatesPerPartition[partition].LOWERBOUND[nodeID];
    memcpy(l, lower, sizeof(int) * NUM_OF_TABLES);

    /* upper range pointer for each relation. */
    int* u = _aggregatesPerPartition[partition].UPPERBOUND[nodeID];
    memcpy(u, upper, sizeof(int) * NUM_OF_TABLES);

    /* Reset the agggregate array to zero. */
    memset(&aggregates[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES + nodeID]],
           0, sizeof(double) * _numberOfAggregatesPerNode[nodeID]);

    /* localAggregates contains the aggregates of varying degree for one
     * particular join value */
    double* localAggregates = _aggregatesPerPartition[partition].localAggregates;

    /* computedAggs contains all aggregate combinations for the aggregates
     * computed at children nodes */
    double* computedAggs = _aggregatesPerPartition[partition].computedAggregates;

    /****************** CATEGORICAL *******************/
    vector<double>& newAggregates = _aggregatesPerPartition[partition].newAggregates;
    vector<double>& mergedAggregates =
        _aggregatesPerPartition[partition].mergedAggregates;

    categoricalAggregates[nodeID].clear();
    newAggregates.clear();
    mergedAggregates.clear();

    memset(
        &categoricalOffsets[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + nodeID]],
        0, sizeof(int) * numberOfCategoricalAggregates);

    memset(&newOffsets[0],0, sizeof(int) * numberOfCategoricalAggregates);
    /****************** CATEGORICAL *******************/

    int* ordering = _aggregatesPerPartition[partition].ordering[nodeID];

    /* Provides order of Relations from min -> max. */
    getRelationOrdering(l, node, ordering);

    /* Value that satisfies the join query - set in seekValue. */
    double val;

    /* Indexes used by the join algorithm. */
    int i, j, rel = 0, numOfRel = _ids[nodeID].size();
    bool atEnd = false;

    while (!atEnd)
    {
        /* seek the value that satisfies the join query */
        atEnd = seekValue(node, rel, ordering, numOfRel, l, upper, val);

        if (atEnd)
            break;

        /* Get range of tuples with value equal to val. */
        for (size_t k = 0; k < _ids[nodeID].size(); ++k)
        {
            i = _ids[nodeID][k].first;
            j = _ids[nodeID][k].second;
            u[i] = l[i];

            // TODO: This could be optimized and perhaps added to seek value -->
            // seek range
            while (u[i] < upper[i] && _data[i][u[i] + 1][j] == val)
            {
                ++u[i];
            }
        }

        /*
         * Below the aggregates are updated based on the value that
         * satisfied the join query.
         */
        if (childrenCount > 0)
        {
            bool childEmpty = false;

            /*
             * Update the varMap which keeps track of the values that satisfy the
             * join query above this node
             */
            varMap[nodeID] = val;

            /* Contains the IDs of the children, used to optimize code. */
            int* childIDs = node->_childrenIDs;

            localNumOfValues = 0;
            
            DTreeNode* child = node->_firstChild;
            for (int i = 0; i < childrenCount; ++i)
            {
                /* Call join algorithm for each child */
                localNumOfValues += leapfroggingJoin(child, l, u, partition);

                /*
                 * If the first aggregate for child is zero then count = 0
                 * and the join query is not satisfied for this value.
                 */
                if (aggregates[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES
                                                          + childIDs[i]]] == 0)
                {
                    childEmpty = true;
                    break;
                }

                child = child->_next;
            }

            /* if no child is empty we update the aggregates */
            if (!childEmpty)
            {
                numberOfValues += localNumOfValues + 1;
                
                /* Computes the local aggregates for this node. */
                double agg = 1;
                for (int i = 0; i <= _degreeOfLocalAggregates[nodeID]; ++i)
                {
                    localAggregates[i] = agg;
                    agg *= val;
                }

                int* aggList = _aggregateRegister[nodeID];
                int nodeOffset = _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES + nodeID];

                for (int aggNo = 0; aggNo < numberOfAggregates; ++aggNo)
                {
                    double aggValue = 1;

                    /*
                     * If the aggregate has not be previously computed we compute it
                     * and store it in computedAggs
                     */
                    if (aggList[aggNo] == -1)
                    {
                        int index = 2 * numberOfAggregates + childrenCount * aggNo;
                        for (int j = 0; j < childrenCount; ++j)
                        {
                            aggValue *=
                                aggregates[_numberOfAggregatesPerNode[
                                    NUM_OF_ATTRIBUTES+ childIDs[j]] +
                                           aggList[index + j]];
                        }
                        computedAggs[aggNo] = aggValue;
                    }
                    else
                    {
                        /* Here we reuse a previously computed aggregate. */
                        aggValue = computedAggs[aggList[aggNo]];
                        computedAggs[aggNo] = aggValue;
                    }

                    /*
                     *  Multiply the computed aggregates by the local aggregate
                     *  and add it to the global aggregates.
                     */
                    aggregates[nodeOffset + aggNo] +=
                        aggValue * localAggregates[aggList[numberOfAggregates + aggNo]];
                }


                /*************** CATEGORICAL ***************/
                int offsetInRegister = numberOfAggregates * (childrenCount + 2);

                vector<Pointers> catAggIndexes;
                catAggIndexes.reserve(childrenCount);

                newAggregates.clear();
                mergedAggregates.clear();

                for (int catAggNo = 0; catAggNo < numberOfCategoricalAggregates; ++catAggNo)
                {
                    // TODO Need to check how many categorical aggs are in this one!
                    double contAggValue = localAggregates[aggList[offsetInRegister +
                                                                  numberOfCategoricalAggregates + catAggNo]];

                    int index = offsetInRegister + 2 * numberOfCategoricalAggregates +
                        (childrenCount + 1) * catAggNo;

                    for (int j = 0; j < childrenCount; ++j)
                    {
                        /* If condition satisfied then this child contributes with continuous aggregate */
                        if (aggList[index + j] < _numberOfAggregatesPerNode[childIDs[j]])
                            contAggValue *=
                                aggregates[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES +
                                                                      childIDs[j]] + aggList[index + j]];
                        else  /* Child contributes with categorical aggregate */
                        {
                            int offset = aggList[index + j] - _numberOfAggregatesPerNode[childIDs[j]];
                            int firstOffset = _numberOfAggregatesPerNode[childIDs[j] + NUM_OF_ATTRIBUTES * 3]; 

                            int lowerPointer =
                                (offset > 0 ? categoricalOffsets[firstOffset + offset - 1] : 0);
                            int upperPointer = categoricalOffsets[firstOffset + offset];

                            Pointers p = {
                                childIDs[j],
                                _degreeOfCategoricalAggregateGroup[firstOffset + offset],
                                categoricalAggregates[childIDs[j]].begin()+lowerPointer,
                                categoricalAggregates[childIDs[j]].begin()+upperPointer,
                                categoricalAggregates[childIDs[j]].begin()+lowerPointer
                            };

                            catAggIndexes.push_back(p);
                        }
                    }

                    computedAggs[_numberOfAggregatesPerNode[nodeID] + catAggNo] = contAggValue;

                    if (catAggIndexes.size() == 0)
                    {
                        /* In this case the aggregate from below is fully
                         * continuous so we only have to add this one */
                        newAggregates.push_back(val);
                        newAggregates.push_back(contAggValue);

                        newOffsets[catAggNo] = newAggregates.size();
                    }
                    else if (catAggIndexes.size() == 1)
                    {
                        // In this case we can avoid the cartesian product and
                        // add the aggregates right away
                        while (catAggIndexes[0].me != catAggIndexes[0].end)
                        {
                            if (aggList[index + childrenCount] == 1)
                                newAggregates.push_back(val);
							
                            for (int i = 0; i < catAggIndexes[0].degreeCategorical; ++i)
                            {
                                newAggregates.push_back(*catAggIndexes[0].me);
                                ++catAggIndexes[0].me;
                            }

                            newAggregates.push_back(
                                *(catAggIndexes[0].me) * contAggValue);
                            ++catAggIndexes[0].me;
                        }

                        newOffsets[catAggNo] = newAggregates.size();
                    }
                    else
                    {
                        cartesianProduct(newAggregates, catAggIndexes, contAggValue,
                            aggList[index + childrenCount] == 1, val);
                        newOffsets[catAggNo] = newAggregates.size();
                    }

                    catAggIndexes.clear();
                }

                /* We are now merging the new values to the already computed
                 * aggregates */
                if (numberOfCategoricalAggregates > 0)
                {
                    /* this is the case for for the first value in the union */
                    if (unlikely(categoricalAggregates[nodeID].size() == 0))
                    {
                        categoricalAggregates[nodeID].insert(
                            categoricalAggregates[nodeID].end(),
                            newAggregates.begin(),
                            newAggregates.end()
                            );

                        memcpy(&categoricalOffsets[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + nodeID]],newOffsets, sizeof(int) * numberOfCategoricalAggregates);
                    }
                    else
                    {
                        int indexForNew = 0;
                        int indexForCurrent = 0;

                        int* catOffsets = &_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3];						
                        int* numCatAggs = &_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2];

                        vector<double>& catAggsThisNode = categoricalAggregates[nodeID];

                        for (int aggGroup = 0; aggGroup < numCatAggs[nodeID]; ++aggGroup)
                        {
                            int degreeOfAggGroup = _degreeOfCategoricalAggregateGroup[catOffsets[nodeID] + aggGroup];

                            while (indexForNew < newOffsets[aggGroup] &&
                                   indexForCurrent < categoricalOffsets[catOffsets[nodeID] + aggGroup])
                            {
                                bool newIsSmaller = false;
                                bool currentIsSmaller = false;

                                /* First check if values in new or current aggreagates are smaller */
                                for (int k = 0; k < degreeOfAggGroup; ++k)
                                {
                                    if (newAggregates[indexForNew + k] < catAggsThisNode[indexForCurrent + k])
                                    {
                                        newIsSmaller = true;
                                        break;
                                    }
                                    if (newAggregates[indexForNew + k] > catAggsThisNode[indexForCurrent + k])
                                    {
                                        currentIsSmaller = true;
                                        break;
                                    }
                                }

                                /* If values in new is smaller add aggregate from new */
                                if (newIsSmaller)
                                {
                                    // printf("%d --- newIsSmaller \n", aggGroup);
                                    for (int k = 0; k < degreeOfAggGroup; ++k)
                                    {
                                        mergedAggregates.push_back(newAggregates[indexForNew]);
                                        ++indexForNew;
                                    }

                                    mergedAggregates.push_back(newAggregates[indexForNew]);
                                    ++indexForNew;
                                }
                                /* If values in current is smaller add aggregate from current */
                                else if (currentIsSmaller)
                                {
                                    // printf("%d --- currentIsSmaller \n",aggGroup);
                                    for (int k = 0; k < degreeOfAggGroup; ++k)
                                    {
                                        mergedAggregates.push_back(categoricalAggregates[nodeID][indexForCurrent]);
                                        ++indexForCurrent;
                                    }

                                    mergedAggregates.push_back(categoricalAggregates[nodeID][indexForCurrent]);
                                    ++indexForCurrent;
                                }
                                else
                                {
                                    // printf("%d --- new and current are the same \n", aggGroup);
                                    for (int k = 0; k < degreeOfAggGroup; ++k)
                                    {
                                        mergedAggregates.push_back(categoricalAggregates[nodeID][indexForCurrent]);
                                        ++indexForCurrent;
                                        ++indexForNew;
                                    }

                                    /* If values are the same we add the sum of the aggreagtes from new and current */
                                    mergedAggregates.push_back(
                                        categoricalAggregates[nodeID][indexForCurrent] +
                                        newAggregates[indexForNew]);

                                    ++indexForCurrent;
                                    ++indexForNew;
                                }
                            }

                            if (indexForNew < newOffsets[aggGroup])
                            {
                                // printf("%d --- filling up from new | index: %d offset: %d \n", aggGroup, indexForNew, newOffsets[aggGroup]);
                                mergedAggregates.insert(mergedAggregates.end(),
                                                        newAggregates.begin()+indexForNew,
                                                        newAggregates.begin()+newOffsets[aggGroup]);
                            }

                            if (indexForCurrent < categoricalOffsets[catOffsets[nodeID] + aggGroup])
                            {
                                // printf("%d --- filling up from current | index: %d offset: %d \n", aggGroup, indexForCurrent, categoricalOffsets[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + nodeID] + aggGroup]);
                                mergedAggregates.insert(mergedAggregates.end(),
                                                        categoricalAggregates[nodeID].begin() + indexForCurrent,
                                                        categoricalAggregates[nodeID].begin() + categoricalOffsets[catOffsets[nodeID] + aggGroup]);
                            }

                            indexForNew = newOffsets[aggGroup];
                            indexForCurrent = categoricalOffsets[catOffsets[nodeID] + aggGroup];

                            categoricalOffsets[catOffsets[nodeID] + aggGroup] = mergedAggregates.size();
                        }

                        /* Swap current and merge for next value */
                        // categoricalAggregates[nodeID] = mergedAggregates;
                        categoricalAggregates[nodeID].swap(mergedAggregates);    // TODO : check which one would work better ?!
                    }
                }
            }
        }
        /* Case for leaf nodes */
        else
        {
            numberOfValues += 1; 
            /*
             * Computes the local aggregates for this node
             *  -- foreach d in localdegree: val^d
             * If node is categorical, degree = 0;
             *  -- only computes count.
             */
            double agg = 1;
            for (int i = 0; i <= _degreeOfLocalAggregates[nodeID]; ++i)
            {
                localAggregates[i] = agg;
                agg *= val;
            }

            int* aggList = _aggregateRegister[nodeID];
            int nodeOffset = _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES + nodeID];

            /* Adds the value with correct degree to aggregates array. */
            for (int aggNo = 0; aggNo < numberOfAggregates; ++aggNo)
                aggregates[nodeOffset + aggNo] +=
                    localAggregates[aggList[numberOfAggregates + aggNo]];

            /* Adds categorical aggregate to exisiting aggregates. */
            if (_onehotfeatures[nodeID])
            {
                vector<double>& exisitingAggregates = categoricalAggregates[nodeID];

                if (exisitingAggregates.size() > 0)
                    assert(val > exisitingAggregates[exisitingAggregates.back() - 1]);

                /* 
                 * Since we know that the a leaf union is sorted we can simply 
                 * add the value to the end of the existingAggregates.
                 */
                exisitingAggregates.push_back(val);
                exisitingAggregates.push_back(1.0);

                categoricalOffsets[
                    _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + nodeID]] += 2;
            }
        }

        if (isDetermined[nodeID])
        {
            int determinantID = determinedBy[nodeID];
            double determinantValue = varMap[determinantID];

            map<double, set<double>>& funcDepend = 
                _aggregatesPerPartition[partition].functionalDependencies[nodeID]; 

            auto it = funcDepend.find(val);
            if (it != funcDepend.end())
                it->second.insert(determinantValue);
            else
                funcDepend[val] = {determinantValue};
        }

        for (size_t k = 0; k < _ids[nodeID].size(); ++k)
        {
            i = _ids[nodeID][k].first;
            l[i] = u[i];
        }

        i = _ids[nodeID][ordering[rel]].first;
        l[i] += 1;

        if (l[i] > upper[i])
        {
            atEnd = true;
            break;
        }
        else
        {
            rel = (rel + 1) % numOfRel;
        }
    }

    /* If we get here and caching is allowed it means that we need to add
     * aggregates to cache. */
    if (caching)
    {
        /*
         * We initialise the elements in the vector to nodeID instead of
         * default-initialising them.  The last element will not be overwritten.
         */
        vector<double> keyVals(node->_key.size() + 1, nodeID);
        for (size_t i = 0; i < node->_key.size(); ++i)
            keyVals[i] = varMap[node->_key[i]];

        double *aggs = new double[numberOfAggregates];
        memcpy(aggs,&aggregates[_numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES + nodeID]],
               (sizeof(double) * numberOfAggregates));

        int *catOffsets = new int[numberOfCategoricalAggregates];
        memcpy(catOffsets,&categoricalOffsets[
                   _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3+ nodeID]],
               (sizeof(int) * numberOfCategoricalAggregates));
        
        double *catAggregates = nullptr;
        if (numberOfCategoricalAggregates > 0)
        {
            catAggregates = new double[catOffsets[numberOfCategoricalAggregates - 1]];
            memcpy(catAggregates, &categoricalAggregates[nodeID][0],
                   (sizeof(double) * catOffsets[numberOfCategoricalAggregates - 1]));
        }
        
        /* Lock the _caches map so we can savely insert our newly cached values */
        lock_guard<mutex> lock(_cacheMutex);
        _caches.insert({ keyVals, CacheAggregates(aggs, catOffsets, catAggregates) });
    }

    return numberOfValues;
}




// ----> categorical aggregates are filled when merging!
void Fade::cartesianProduct(vector<double>& out, vector<Pointers>& in, double contAgg, bool nodeIsCat, double nodeValue)
{
    // printf("cartesianProduct\n");
    int numOfAggGroups = in.size();

    while(true)
    {
        if (nodeIsCat)
            out.push_back(nodeValue);

        /* Add the information on the values for this aggregate. */
        for (int i = 0; i < numOfAggGroups; ++i)
        {
            for (int deg = 0; deg < in[i].degreeCategorical; ++deg)
            {
                /* Push back value identifier for this cat aggregate */
                out.push_back(*( (in[i].me) + deg));
            }
        }

        /* Set the aggregate to be compute equal to the continuous part. */
        double agg = contAgg;

        /* Multiply the continuous part of the aggregate with the corresponding
         * categorical part. */
        for (int i = 0; i < numOfAggGroups; ++i)
            agg *= (*(in[i].me+in[i].degreeCategorical));

        /* Push back actual aggregate */
        out.push_back(agg);

        // When you reach the end, reset that one to the beginning and
        // increment the next-to-last one.
        for (int i = numOfAggGroups - 1 ; i >= 0; --i)
        {
            in[i].me += in[i].degreeCategorical + 1;

            if(in[i].me == in[i].end)
            {
                if(i == 0)
                    return;

            	in[i].me = in[i].begin;
            }
            else
            {
            	/* Stop the for loop */
                break;
            }
        }
    }
}

void Fade::mergeCategoricalValues(vector<double>& out, int* outOffsets,
	std::vector<std::vector<double> > const &in, std::vector<int*> const &inOffsets)
{
	int rootID = _dTree->_root->_id;
	int numOfAggGroups = _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + rootID];
	int firstCatAggOffset = _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + rootID];

	int numOfPartitions = in.size();
        
	double minVal[DEGREE_OF_INTERACTIONS * 2];
	double nextMinVal[DEGREE_OF_INTERACTIONS * 2];

	int pointer[numOfPartitions];
	memset(&pointer[0], 0, sizeof(int) * numOfPartitions);

	for (int aggGroup = 0; aggGroup < numOfAggGroups; ++aggGroup)
	{
		int degreeOfAggGroup = _degreeOfCategoricalAggregateGroup[firstCatAggOffset + aggGroup];

		/* initialize minVal and nextMinVal */
		for (int k = 0; k < degreeOfAggGroup; ++k)
		{
			minVal[k] = in[0][pointer[0] + k];
			nextMinVal[k] = std::numeric_limits<double>::max();
		}

		/* find the minimum value array */
		for (int part = 1; part < numOfPartitions; ++part)
		{
			for (int k = 0; k < degreeOfAggGroup; ++k)
			{
				if (in[part][pointer[part] + k] < minVal[k])
				{
					for (int l = k; l < degreeOfAggGroup; ++l)
						minVal[l] = in[part][pointer[part] + l];
				}
				if (in[part][pointer[part] + k] > minVal[k])
					break;
			}
		}

		bool newValue = true;
		while (newValue)
		{
			double agg = 0.0;
			newValue = false;

			bool isMinVal;

			for (int j = 0; j < numOfPartitions; ++j)
			{
				if (pointer[j] < inOffsets[j][aggGroup])
				{
					auto& a = in[j];

				 	isMinVal = true;

					for (int k = 0; k < degreeOfAggGroup; ++k)
					{
                        int x = pointer[j] + k;

						if (a[x] != minVal[k])
						{
							isMinVal = false;
							break;
						}
					}

					if (isMinVal)
					{
						// printf("isMinVal %d  pointer[j] %d  inOff[j][i] %d \n", isMinVal, pointer[j], inOffsets[j][aggGroup]);
					  	agg += a[pointer[j] + degreeOfAggGroup];
						pointer[j] += degreeOfAggGroup + 1;
					}

					isMinVal = false;

					// printf("id: %d d: %d pointer[j]: %d  childAggCounts[d][j][i]: %d \n", nodeID, d, pointer[j], childAggCounts[d][j][i]);
					if (pointer[j] < inOffsets[j][aggGroup])
					{
						isMinVal = true;

						for (int k = 0; k < degreeOfAggGroup; ++k)
						{
							if (a[pointer[j] + k] < nextMinVal[k])
							{
								for (int l = k; l < degreeOfAggGroup; ++l)
								{
									nextMinVal[l] = a[pointer[j] + l];
								}
							}
							if (a[pointer[j] + k] > nextMinVal[k])
							{
								isMinVal = false;
								break;
							}
                            // printf("nextMinVal: %f \n", nextMinVal[k]);
						}
					}
					// printf("isMinVal %d   Degree:  %lu   nodeID: %d \n", isMinVal, d, nodeID);

					if (isMinVal) newValue = true;
				}
			}

            // printf("nextMinVal: ");
            // for (int k = 0; k < degreeOfAggGroup; ++k)
            //     printf("%.2f ", nextMinVal[k]);
            // printf("\n");

			for (int k = 0; k < degreeOfAggGroup; ++k)
			{
				// printf("minVal entered:  %f \n", minVal[k]);
				out.push_back(minVal[k]);
			}

			//printf("agg entered:  %f \n", agg);
			out.push_back(agg);

			for (int k = 0; k < degreeOfAggGroup; ++k)
			{
				minVal[k] = nextMinVal[k];
				nextMinVal[k] =  std::numeric_limits<double>::max();
			}
		}

		for (int j = 0; j < numOfPartitions; ++j)
			assert(pointer[j] == inOffsets[j][aggGroup]);

		outOffsets[aggGroup] = out.size();
	}

	// printf("outAggregates:  ");	int off = 0;
	// for (size_t i = 0; i < out.size(); ++i) {
	// 	if (i == (size_t) outOffsets[off]) {
	// 		printf(" | "); ++off; }
	// 	printf("%.2f ", out[i]);
	// } printf("\n");
	// printf("outOffsets:  ");
	// for (int i = 0; i < numOfAggGroups; ++i)
	// 	printf("%d ", outOffsets[i]);
	// printf("\n");
}




void Fade::getRelationOrdering(int* left, DTreeNode* node, int* ordering)
{
	int nodeID = node->_id;
	int numberOfRelations = _ids[nodeID].size();

	if (numberOfRelations == 1)
	{
		ordering[0] = 0;
		return;
	}

	vector<pair<int, double> > values(numberOfRelations);

	for (int k = 0; k < numberOfRelations; ++k)
	{
		int i = _ids[nodeID][k].first;
		int j = _ids[nodeID][k].second;
                
		values[k] = make_pair(k, _data[i][left[i]][j]);
	}

	sort(values.begin(), values.end(), sort_pred());

	for (int k = 0; k < numberOfRelations; ++k)
		ordering[k] = values[k].first;
}

void Fade::sortDataToProcess()
{
	/* Contains an ID for each node in the DTree and for each table. */
	vector<vector<int> > treeAttributeIDs(NUM_OF_TABLES);

	/* Iterate through all tables. */
	for (size_t table = 0; table < NUM_OF_TABLES; ++table)
	{
		/* Iterate through all the table attribute IDs of the current table. */
		for (uint_fast16_t id : _dataHandler->getTableAttributes()[table])
		{
			/* Scan through the (string, ID) mapping to find the attribute name of the current attribute ID. */
			for (auto pair : _dataHandler->getNamesMapping())
			{
				if (pair.second == id)
				{
					/* Retrieve DTree ID with attribute name. */
					auto index = _dTree->getIndexByName(pair.first);

					treeAttributeIDs[table].push_back(
							index);
					break;
				}
			}
		}
	}

	/*
	 * The ids vectors will contain the pairs of INDEXES in the array attributes-attr.
	 * For each attribute KEY-index we append the pairs of indexes having that key,
	 * so we will have all the pairs of <relationIndex-attributeIndex> for each attribute KEY.
	 */
	_ids = new vector<pair<int, int> > [NUM_OF_ATTRIBUTES];
	for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
	{
		for (size_t j = 0; j < NUM_OF_TABLES; ++j)
		{
			for (size_t k = 0; k < treeAttributeIDs[j].size(); ++k)
				if (treeAttributeIDs[j][k] == (int) i)
				{
					_ids[i].push_back(make_pair(j, k));
					break;
				}
		}
	}

	vector<int_fast16_t> priority;
	for (size_t table = 0; table < NUM_OF_TABLES; ++table)
	{
		priority.clear();

		DTreeNode* node = _dTree->getNode(table + NUM_OF_ATTRIBUTES)->_parent;

		while (node != NULL)
		{
			int_fast16_t pos = -1;
			for (size_t j = 0; j < treeAttributeIDs[table].size(); ++j)
				if (node->_id == treeAttributeIDs[table][j])
				{
					pos = j;
					break;
				}
			if (pos != -1)
				priority.push_back(pos);
			node = node->_parent;
		}

		/* Launch sorting; depending on the compiler, this will or will not be done in parallel. */
		sortingAlgorithm(_data[table].begin(), _data[table].end(),
				ValueOrdering(priority.data(), priority.size()));
	}

	DINFO("INFO - engine: finished sorting data to process.\n");
}

size_t Fade::getTableToPartition()
{
	size_t tableIndex = 0;

	/* Size of the current candidate table. */
	size_t tableSize = 0;

	/* ID of the root attribute in the DTree. */
	uint_fast16_t rootID = _dataHandler->getNamesMapping()[_dTree->_root->_name];

	/* Iterate through all tables. */
	for (size_t table = 0; table < NUM_OF_TABLES; ++table)
	{
		/* Iterate through all the table attribute IDs. */
		for (uint_fast16_t id : _dataHandler->getTableAttributes()[table])
		{
			/* Check whether the current attribute id is equal to the root one;
			 * we partition preferably on the biggest table. */
			if (id == rootID && _data[table].size() > tableSize)
			{
				tableIndex = table;
				tableSize = _data[table].size();
				/* Continue to next table. */
				break;
			}
		}
	}

	return tableIndex;
}
