//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_RUN_LAUNCHER_H_
#define INCLUDE_RUN_LAUNCHER_H_

#include <DataHandler.hpp>
#include <DTree.h>
#include <Engine.h>
#include <MachineLearningModel.hpp>
// #include <Shuffler.hpp>

/**
 * Class that takes care of assembling the different components of the database and launching the tasks.
 */
class Launcher: public std::enable_shared_from_this<Launcher>
{

public:

    /**
     * Constructor; need to provide path to the schema, table and other configuration files.
     */
    Launcher(const std::string& pathToFiles);

    ~Launcher();

    /**
     * Launches the database operations.
     */
    int launch(unsigned int numOfThreads, unsigned int numOfPartitions,
               std::string model);

    /**
     * Returns a pointer to the DataHandler module.
     */
    std::shared_ptr<DataHandler> getDataHandler();

    /**
     * Returns a pointer to the d-tree.
     */
    std::shared_ptr<DTree> getDTree();

    /**
     * Returns a pointer to the Engine module.
     */
    std::shared_ptr<Engine> getAggregateEngine();

    /**
     * Returns a pointer to the Engine module.
     */
    std::shared_ptr<MachineLearningModel> getModel();


private:

    //! DataHandler module of the database.
    std::shared_ptr<DataHandler> _dataHandler;

    //! Engine module of the database.
    std::shared_ptr<MachineLearningModel> _model;


    //! Engine module of the database.
    std::shared_ptr<DTree> _dTree;

    //! Engine module of the database.
    std::shared_ptr<Engine> _engine;

    //! Path to the files used by the database.
    std::string _pathToFiles;

    /**
     * Returns the local IP of the node.
     */
    std::string getLocalIP();

};

#endif /* INCLUDE_RUN_LAUNCHER_H_ */
