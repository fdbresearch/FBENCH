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
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <fstream>
#include <unordered_set>
#include <set>

#include <GlobalParams.hpp>
#include <Launcher.h>
#include <Logging.hpp>
#include <CountAggregate.h>

static const std::string FEATURE_CONF = "/variables.txt";

static const char COMMENT_CHAR = '#';
static const char NUMBER_SEPARATOR_CHAR = ',';
static const char ATTRIBUTE_SEPARATOR_CHAR = ',';
static const char ATTRIBUTE_NAME_CHAR = ':';

namespace phoenix = boost::phoenix;
using namespace boost::spirit;
using namespace dfdb::params;
using namespace dfdb::types;
using namespace std;
using namespace std::chrono;

CountAggregate::CountAggregate(
    const string& pathToFiles, shared_ptr<Launcher> launcher) :
    _pathToFiles(pathToFiles), _launcher(launcher)
{
    _dTree = launcher->getDTree();

#ifdef BENCH
    int64_t startPrecompute = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
#endif

    /* creates the aggregate Register that is passed to FADE. */
    createAggregateRegister();
    
#ifdef BENCH
    int64_t endPrecompute = duration_cast<milliseconds>
        (system_clock::now().time_since_epoch()).count() - startPrecompute;    
#endif
    
    BINFO( "TIME - Count precomputation: "
           + to_string(endPrecompute) + "ms.\n");
}

CountAggregate::~CountAggregate()
{
    for (size_t i = 0; i < NUM_OF_FEATURES; ++i)
        delete[] _features[i];
    delete[] _features;

    for (size_t i = 0; i < _aggregateRegister.size(); ++i)
        for (size_t j = 0; j < _aggregateRegister[i].size(); ++j)
            delete[] _aggregateRegister[i][j];

    delete[] _numberOfAggregatesPerNode;
    delete[] _degreeOfLocalAggregates;
    
    for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
    {
        if (i != (size_t) _dTree->_root->_id)
        {
            delete[] aggPlaceholder[i];
            delete[] lineagePlaceholder[i];
        }
    }

    delete[] aggPlaceholder;
    delete[] lineagePlaceholder;
    delete[] _cofactorMatrix;
}

void CountAggregate::run()
{

#ifdef BENCH
    int64_t startF = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
#endif
    
    /* Calls Fade to compute the aggregates */
    (dynamic_pointer_cast<Fade>(_launcher->getAggregateEngine()))->run(
        _fadeAggregates, _aggregateRegister, _numberOfAggregatesPerNode, 
        _degreeOfLocalAggregates, onehotfeatures, isDetermined, determinedBy);


#ifdef BENCH
    int64_t endF = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count() - startF;
#endif

    BINFO("TIME - Count calculation: " + to_string(endF) + "ms.\n");
    
    std::cout << "DATA - Number of Tuples in join result: " <<
        (size_t) _fadeAggregates.aggregates[0] << std::endl;
}

void CountAggregate::createAggregateRegister()
{
    _aggregateRegister.resize(NUM_OF_ATTRIBUTES);
    _lineageRegister.resize(NUM_OF_ATTRIBUTES);
    
    _numberOfAggregatesPerNode = new int[NUM_OF_ATTRIBUTES * 4]();
    _degreeOfLocalAggregates = new int[NUM_OF_ATTRIBUTES]();

    aggPlaceholder = new int*[NUM_OF_ATTRIBUTES];
    lineagePlaceholder = new int*[NUM_OF_ATTRIBUTES];

    onehotfeatures = new bool[NUM_OF_ATTRIBUTES]();
    isDetermined = new bool[NUM_OF_ATTRIBUTES]();
    determinedBy = new int[NUM_OF_ATTRIBUTES + 1]();

    /* Read in features from file and finds the labels. */
    _features = new int*[NUM_OF_FEATURES]();

    
    for (size_t i = 0; i < NUM_OF_ATTRIBUTES; ++i)
    {        
        if (i != (size_t) _dTree->_root->_id)
        {
            int numOfChildren = _dTree->getNode(i)->_parent->_numOfChildren;
            aggPlaceholder[i] = new int[numOfChildren + 2]();
            lineagePlaceholder[i] = new int[NUM_OF_ATTRIBUTES + 1]();
        }
    }

    /* Registers aggregates to the d-tree nodes following the structure of the model. */
    registerAggregatesFromModel(_dTree->_root);

    /* Setting the upper half of _numberOfAggregatesPerNode, which provides offsets */
    _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES] = 0;
    for (size_t attID = 0; attID < NUM_OF_ATTRIBUTES - 1; ++attID)
        _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES + attID + 1] =
            _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES + attID]
            + _numberOfAggregatesPerNode[attID];

    _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3] = 0;
    for (size_t attID = 0; attID < NUM_OF_ATTRIBUTES - 1; ++attID)
        _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + attID + 1] =
            _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 3 + attID]
            + _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + attID];
}




void CountAggregate::registerAggregatesFromModel(DTreeNode* node)
{
    int nodeID = node->_id;
    int numOfChildren = node->_numOfChildren;

    if (numOfChildren > 0)
    {
        _degreeOfLocalAggregates[nodeID] = 0;

        DTreeNode* child = node->_firstChild;
        for (int i = 0; i < numOfChildren; ++i)
        {
            registerAggregatesFromModel(child);
            child = child->_next;
        }

        _numberOfAggregatesPerNode[nodeID] = 1;
        _numberOfAggregatesPerNode[NUM_OF_ATTRIBUTES * 2 + nodeID] = 0;
        
        _aggregateRegister[nodeID].resize(1);
        _lineageRegister[nodeID].resize(1);

        // aggReg
        int* aggRegister = new int[numOfChildren + 2 + NUM_OF_ATTRIBUTES + 1]();
        aggRegister[0] = -1;

        _aggregateRegister[nodeID][0] = aggRegister;
        _lineageRegister[nodeID][0] = vector<int>(NUM_OF_ATTRIBUTES + 1, 0);
    }
    else
    {
        _numberOfAggregatesPerNode[nodeID] = 1;

        _aggregateRegister[nodeID].resize(1);
        _lineageRegister[nodeID].resize(1);
        
        int* aggRegister = new int[2]();
        aggRegister[0] = -1;
        aggRegister[1] = 0;

        vector<int> lineageRegister(NUM_OF_ATTRIBUTES + 1, 0);

        _aggregateRegister[nodeID][0] = aggRegister;
        _lineageRegister[nodeID][0] = lineageRegister;
    }
}
