//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#include <fstream>

#include <DataHandlerCSV.h>
//#include <FactorizationMachine.h>
#include <FactorisedJoin.h>
#include <Fade.h>
#include <GlobalParams.hpp>
#include <Launcher.h>
#include <Logging.hpp>
#include <CountAggregate.h>
#include <CountOverJoin.h>

static const std::string DTREE_CONF = "/dtree.txt";

/* Declaration of the extern variable in GlobalParams.hpp. */
#ifndef ATTRIBUTES
size_t dfdb::params::NUM_OF_ATTRIBUTES;
#endif
#ifndef WORKERS
size_t dfdb::params::NUM_OF_WORKERS;
#endif
#ifndef FEATURES
size_t dfdb::params::NUM_OF_FEATURES;
#endif
#ifndef LFEATURES
size_t dfdb::params::NUM_OF_LINEAR_FEATURES;
#endif
#ifndef DEGREE
size_t dfdb::params::DEGREE_OF_INTERACTIONS;
#endif
#ifndef MANUAL
size_t dfdb::params::NUM_OF_MANUAL_INTERACTIONS;
#endif

//using namespace boost::asio;
//using namespace boost::asio::ip;
using namespace dfdb::params;
using namespace std;
using namespace std::chrono;

Launcher::Launcher(const string& pathToFiles) :
		_pathToFiles(pathToFiles)
{
}

Launcher::~Launcher()
{
}

int Launcher::launch(unsigned int numOfThreads, unsigned int numOfPartitions,
                     string model)
{
#ifdef BENCH
    int64_t start = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
#endif


    /* Check consistency between compile time flag and runtime value issued from
     * configuration file. */

    /* Build d-tree. */
    _dTree.reset(new DTree(_pathToFiles + DTREE_CONF));

    /* Check the validation of the d-tree */
    _dTree->isValid();

    DINFO("Constructed the Dtree \n");

    /* Check consistency between compile time flag and runtime value issued
     * from configuration file. */
#ifdef ATTRIBUTES
    if(ATTRIBUTES != _dTree->numberOfAttributes())
    {
        ERROR("Value of compiler flag -DATTRIBUTES and number of attributes"<<
              " specified in Dtree inconsistent. Aborting.\n")
            exit(1);
    }
    /* Assign value to extern variable in GlobalParams.hpp. */
#else
    NUM_OF_ATTRIBUTES = _dTree->numberOfAttributes();
#endif
    
    if (model == "countJoin")
    {
        _model.reset(
            new CountOverJoin(_pathToFiles, shared_from_this()));
    }
    else if (model == "count")
    {
        _model.reset(
            new CountAggregate(_pathToFiles, shared_from_this()));
    }
    else
    {
        ERROR(
            "Model does not exist. Use --help for more information.\n");
        return EXIT_FAILURE;
    }

#ifdef BENCH
    int64_t startLoading = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
#endif

    _dataHandler.reset(new DataHandlerCSV(_pathToFiles));

    _dataHandler->loadAllTables();
    
#ifdef BENCH
    int64_t endLoading = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count() - startLoading;    
#endif
    
    BINFO(
	"TIME - data loading: " + to_string(endLoading) + "ms.\n");


    if(NUM_OF_ATTRIBUTES != _dataHandler->getNamesMapping().size())
    {
        ERROR("NUM_OF_ATTRIBUTES and number of attributes specified in schema.conf "<<
              "inconsistent. Aborting.\n")
            exit(1);
    }

    if (model == "countJoin")
        _engine.reset(
            new FactorisedJoin(shared_from_this(), numOfThreads, numOfPartitions));
    else
        _engine.reset(
            new Fade(shared_from_this(), numOfThreads, numOfPartitions));


    // CALL THE MODEL RUN FUNCTION!! 

    BINFO(
        "TIME - initialisation of DFDB: " +
        to_string(
            duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()
            - start)+ "ms.\n"
        );

    _model->run();

    return EXIT_SUCCESS;
}

shared_ptr<DataHandler> Launcher::getDataHandler()
{
	return _dataHandler;
}

shared_ptr<DTree> Launcher::getDTree()
{
	return _dTree;
}

shared_ptr<Engine> Launcher::getAggregateEngine()
{
	return _engine;
}
