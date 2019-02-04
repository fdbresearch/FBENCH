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

#include <FactorisedJoin.h>
#include <Logging.hpp>
#include <Launcher.h>
#include <assert.h>

using namespace dfdb::params;
using namespace dfdb::types;
using namespace std;
using namespace chrono;
using namespace node;

FactorisedJoin::FactorisedJoin(shared_ptr<Launcher> launcher, unsigned int numOfThreads, unsigned int numOfPartitions) :
    _dTree(launcher->getDTree()), _dataHandler(launcher->getDataHandler()), 
    _numOfThreads(numOfThreads), _numOfPartitions(numOfPartitions)
{
    _data = _dataHandler->getDataToProcess(); 

    /*
     * If no paramater was set by the user, the default retrieves the number of hardware thread contexts;
     * we will spawn as many threads for partitioning the work done by F. The function can return 0 if value not available.
     * Use one thread in that case.
     */
    if (_numOfThreads == 0)
        _numOfThreads = 1;

    /* First partition to work on is number 0. */
    if (_numOfThreads != 1)
        _nextPartition.store(0, memory_order_relaxed);


#ifdef BENCH
    int64_t startSorting = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
#endif

    sortDataToProcess();

    BINFO(
        "TIME - data sorting: " +
        to_string(duration_cast<milliseconds>(
                      system_clock::now().time_since_epoch()).count() - startSorting) +
        "ms.\n");
}

FactorisedJoin::~FactorisedJoin()
{
    delete[] _ids;
}

void FactorisedJoin::run()
{
    numberOfCachedValues = 0;

    if (_numOfThreads > 1)
    {
        cout << "The multi-threaded version is currently not supported. \n";
        cout << "Continuing with 1 thread. \n";

        _numOfThreads = 1;
    }	

    /* Spawn threads and do partitioning. */
    if (_numOfThreads > 1)
    {
        /* Array containing threads to run F on different partitions. */
        thread* fWorkers = new thread[_numOfThreads];

        /* Table to partition (only one table is partitioned to guarantee join correctness). */
        size_t tableToPartition = getTableToPartition();
        
        rootPerPartition = new Union*[_numOfPartitions];

        _partitionFields = new PartitionAttributes[_numOfPartitions];

        DINFO(
            "WORKER - engine: launching " + to_string(_numOfThreads)
            + " threads to process " + to_string(_numOfPartitions)
            + " partitions on table " + to_string(tableToPartition)
            + ".\n");

        /* This is needed for the lambda expression below - does not accept a variable that is out of scope. */
        Database dataToProcess = _data; 

        /* Launch threads to work on different partitions of the input data. */
        for (unsigned int fWorker = 0; fWorker < _numOfThreads; ++fWorker)
        {
            fWorkers[fWorker] =
                thread(
                    [this, dataToProcess, tableToPartition]()
                    {
                        /* Current partition to work on; increase value for next thread to fetch. */
                        unsigned int currentPartition = _nextPartition.fetch_add(1, memory_order_relaxed);

                        /* Iterate while not all the partitions were processed. */
                        while (currentPartition < _numOfPartitions)
                        {
                            /* Set bounds for the different tables. */
                            int* lowerBounds = new int[NUM_OF_TABLES];
                            int* upperBounds = new int[NUM_OF_TABLES];
                            for (size_t table = 0; table < NUM_OF_TABLES; ++table)
                            {
                                /* This is not a table to partition; it will be fully processed. */
                                if(table != tableToPartition)
                                {
                                    lowerBounds[table] = 0;
                                    upperBounds[table] = _data[table].size() - 1;
                                }
                                /* Table to partition; define bounds based on current partition number. */
                                else
                                {
                                    lowerBounds[table] = (int)(_data[table].size() * 
                                                               ((double) currentPartition/_numOfPartitions));

                                    upperBounds[table] = (int)(_data[table].size() * 
                                                               ((double) (currentPartition+1)/_numOfPartitions)) - 1;
                                }
                            }

                            /* Launch FactorisedJoin to compute Aggregates. */
                            rootPerPartition[currentPartition] = runAggregator(lowerBounds, upperBounds, currentPartition);

                            /* Retrieve and increase value of current partition to work on. */
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

        cout << "Now merging partitions. \n"; 

        // TODO : There is an issue with the merging if the last value from on partition = first value of next partition
        // 		  This would require a recursive merging strategy ?!

        /* Combine the root nodes for each partition. */
        size_t numberOfValues = 0; 
        for (unsigned int part = 0; part < _numOfPartitions; ++part)
            numberOfValues += rootPerPartition[part]->numberOfValues; 
		
        rootNode = new Union(); 
		
        int numOfChildren =  _dTree->_root->_numOfChildren;

        printf("numberOfValues: %lu \n", numberOfValues);
        printf("numberOfChildren: %d \n", numOfChildren);

        rootNode->values = new double[numberOfValues]; 
        rootNode->children = new Union*[numberOfValues * numOfChildren];

        int valuesIndex = 0;
        int pointersIndex = 0;

        for (unsigned int part = 0; part < _numOfPartitions; ++part)
        {
            long valueCount = rootPerPartition[part]->numberOfValues; 

            memcpy(rootNode->values+valuesIndex, rootPerPartition[part]->values,
                   valueCount * sizeof(double)); 

            memcpy(rootNode->children+pointersIndex, rootPerPartition[part]->children,
                   valueCount * numOfChildren * sizeof(Union*)); 
			
            valuesIndex += valueCount; 
            pointersIndex += valueCount * numOfChildren; 
        }

        rootNode -> numberOfValues = numberOfValues;
    }
    else
    {
        _partitionFields = new PartitionAttributes[1];

        /* Set bounds for the different tables: they cover the whole tables */
        int* lowerBounds = new int[NUM_OF_TABLES];
        int* upperBounds = new int[NUM_OF_TABLES];
        for (size_t table = 0; table < NUM_OF_TABLES; ++table)
        {
            lowerBounds[table] = 0;
            upperBounds[table] = _data[table].size() - 1;
        }

        /* Launch FactorisedJoin to compute Aggregates. */
        rootNode = runAggregator(lowerBounds, upperBounds, 0);
		
        /* Clean up bound arrays. */
        delete[] lowerBounds;
        delete[] upperBounds;
    }

    delete[] _partitionFields;

    // std::cout << "DATA - Number of Values: " << numberOfValues << endl; 
}

Union* FactorisedJoin::runAggregator(
    int* lowerBounds, int* upperBounds, unsigned int partition)
{
    DTreeNode* root = _dTree->_root;

    /* Initialize varMap to required size and set entries to zero. */
    _partitionFields[partition].varMap = new double[NUM_OF_ATTRIBUTES]();

    _partitionFields[partition].LOWERBOUND = new int*[NUM_OF_ATTRIBUTES];
    _partitionFields[partition].UPPERBOUND = new int*[NUM_OF_ATTRIBUTES];

    _partitionFields[partition].localPointerPlaceholder =
        new Union**[NUM_OF_ATTRIBUTES];

    /** Comment out for setting with vectors only **/
    _partitionFields[partition].pointers =
        new std::vector<node::Union*>[NUM_OF_ATTRIBUTES];

    _partitionFields[partition].values =
        new std::vector<double>[NUM_OF_ATTRIBUTES];

    _partitionFields[partition].localCount =
        new std::vector<double>[NUM_OF_ATTRIBUTES];
    /** Comment out for setting with vectors only **/

    for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
    {
        _partitionFields[partition].LOWERBOUND[i] = new int[NUM_OF_TABLES];
        _partitionFields[partition].UPPERBOUND[i] = new int[NUM_OF_TABLES];

        _partitionFields[partition].localPointerPlaceholder[i] =
            new Union*[_dTree->getNode(i)->_numOfChildren];
    }

    _partitionFields[partition].ordering = new int*[NUM_OF_ATTRIBUTES];
    for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
        _partitionFields[partition].ordering[i] = new int[_ids[i].size()];
    
    DINFO("Before leapfrogging join! \n");

    /* This performs the join and recursively updates the regression aggregates. */
    return leapfroggingJoin(root, lowerBounds, upperBounds, partition);
}

bool FactorisedJoin::seekValue(DTreeNode* node, int &rel, int* ordering, int numOfRel,
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
            /* If the value we seek is bigger than the last value in the
             * relation we can return. */
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

Union* FactorisedJoin::leapfroggingJoin(
    DTreeNode* node, int* lower, int* upper, unsigned int partition)
{
    int_fast16_t nodeID = node->_id;
    bool caching = node->_caching;
    double* varMap = _partitionFields[partition].varMap;
    int childrenCount = node->_numOfChildren;

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
            return it->second;			// Return pointer to cached union node.
        }
    }

    /* lower range pointer for each relation. */
    int* l = _partitionFields[partition].LOWERBOUND[nodeID];
    memcpy(l, lower, sizeof(int) * NUM_OF_TABLES);

    /* upper range pointer for each relation. */
    int* u = _partitionFields[partition].UPPERBOUND[nodeID];
    memcpy(u, upper, sizeof(int) * NUM_OF_TABLES);

    vector<double>& unionValues = _partitionFields[partition].values[nodeID];
    unionValues.clear();

    vector<double>& unionCounts = _partitionFields[partition].localCount[nodeID];
    unionCounts.clear();

    vector<Union*>& unionPointers = _partitionFields[partition].pointers[nodeID];
    unionPointers.clear();

    Union** localPointers = _partitionFields[partition].localPointerPlaceholder[nodeID];

    int* ordering = _partitionFields[partition].ordering[nodeID];

    /* Provides order of Relations from min -> max. */
    getRelationOrdering(l, node, ordering);

    /* Value that satisfies the join query - set in seekValue. */
    double val;
    unsigned int count = 0; 

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
            i = _ids[node->_id][k].first;
            j = _ids[node->_id][k].second;
            u[i] = l[i];

            // TODO: This could be optimized and perhaps added to seek value
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

            DTreeNode* child = node->_firstChild;
            for (int i = 0; i < childrenCount; ++i)
            {
                /* Call join algorithm for each child */
                localPointers[i] = leapfroggingJoin(child, l, u, partition);

                /*
                 * If the first aggregate for child is zero then count = 0
                 * and the join query is not satisfied for this value. 
                 */ 
                if (localPointers[i] == nullptr)
                {
                    childEmpty = true;
                    break;
                }

                child = child->_next;
            }

            /* if no child is empty we update the aggregates */
            if (!childEmpty)
            {
                /* Push back the value from this node */ 
                unionValues.push_back(val);

                unsigned int localCount = 1;
                for (int i = 0; i < childrenCount; ++i)
                {
                    unionPointers.push_back(localPointers[i]);
                    localCount *= localPointers[i] -> count; 
                }

                count += localCount;
                unionCounts.push_back(localCount);
            }
        }
        /* Case for leaf nodes */
        else
        {
            /* Push back the value from this node */ 
            unionValues.push_back(val);
            ++count;
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

    int numOfUnionValues = unionValues.size(); 
    /*
     * If unionValues.size() == 0 it means that no value for this node
     * has satisfied the join query, so the union is empty.
     */
    if (numOfUnionValues == 0) 
    {
        // delete unionNode; 
        return nullptr;
    }

    /* If there are values in the union we construct the union */
    Union* unionNode = new Union();
    unionNode->numberOfValues = numOfUnionValues;

    numberOfValues += numOfUnionValues; 

    if (childrenCount == 0)
    {
        /* Initialise values array */
        unionNode->values = new double[numOfUnionValues];
		
        /* Copy values into array */
        memcpy(unionNode->values, &unionValues[0],
               numOfUnionValues*sizeof(double));
    }
    else
    {
        /* Initialise values array */
        unionNode->values = new double[numOfUnionValues * 2];
		
        /* Copy values into array */
        memcpy(unionNode->values, &unionValues[0],
               numOfUnionValues*sizeof(double));

        /* Copy values into array */
        memcpy(unionNode->values+numOfUnionValues, &unionCounts[0],
               numOfUnionValues*sizeof(double));
    }

    assert( (size_t) numOfUnionValues * childrenCount == unionPointers.size());

    /* Initialise pointer array */
    unionNode->children = new Union*[numOfUnionValues * childrenCount];

    /* Copy pointer into array */
    for (size_t pointer = 0; pointer < unionPointers.size(); ++pointer)
        unionNode->children[pointer] = unionPointers[pointer];

    unionNode -> count = count; 
	
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

        /* Lock the _caches map so we can savely insert our newly cached values */
        lock_guard<mutex> lock(_cacheMutex);
        unionNode->cacheIndex = numberOfCachedValues;
        ++numberOfCachedValues;
        _caches.insert({ keyVals, unionNode });
    }

    return unionNode; 
}


void FactorisedJoin::getRelationOrdering(int* left, DTreeNode* node, int* ordering)
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

void FactorisedJoin::sortDataToProcess()
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

    DINFO("WORKER - engine: finished sorting data to process.\n");
}

size_t FactorisedJoin::getTableToPartition()
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
