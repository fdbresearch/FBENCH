//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_ML_COUNT_H_
#define INCLUDE_ML_COUNT_H_

#include <string>
#include <vector>

#include <CompilerUtils.hpp>
#include <DataTypes.hpp>
#include <MachineLearningModel.hpp>
#include <Fade.h>

//! Forward declarations to allow pointer to launcher and DTree without cyclic includes.
class DTree;
class Launcher;

/**
 * Class that launches regression model on the data, using d-trees.
 */
class CountAggregate: public MachineLearningModel
{
public:

    /**
     * Constructor; need to provide path to configuration files, pointer to Launcher module in order to retrieve DataHandler instance,
     * and number of threads and partitions to work on.
     */
    CountAggregate(const std::string& pathToFiles, std::shared_ptr<Launcher> launcher);

    ~CountAggregate();

    /**
     * Runs F to compute a regression model.
     */
    void run();

    /**
     * Retrieves the full cofactor matrix.
     */
    dfdb::types::ResultType* getCofactorMatrix()
    {return NULL;}

    /**
     * Retrieve the features array.
     */
    int** getFeatures()
    {
        return _features;
    }


private:

    //! Physical path to the schema and table files.
    std::string _pathToFiles;

    std::shared_ptr<Launcher> _launcher;
	
    //! Array of pointers to the linear regression modules (one for each partition).
    fade::FadeAggregates _fadeAggregates;

    //! Array containing cofactor matrix.
    dfdb::types::ResultType* _cofactorMatrix = NULL;

    //! Array containing the features used by F.
    int** _features = NULL;

    //! Aggregate Engine used to compute the aggregates for cofactor matrices. 
    std::shared_ptr<Fade> _fade;

    //! Pointer to the d-tree.
    std::shared_ptr<DTree> _dTree;

    /* Auxihiliarry arrays to avoid creating excessive arrays. */
    int **aggPlaceholder, **lineagePlaceholder;

    //! Double vector stores the precompiled information about aggregates to be computed.
    std::vector<std::vector<int*> > _aggregateRegister;
    std::vector<std::vector<std::vector<int> > > _lineageRegister;

    //! Array stores the degree of the local aggregates at each node.
    int* _degreeOfLocalAggregates;

    //! Array defines the number of aggregates at each node in dtree
    int* _numberOfAggregatesPerNode;

    bool* onehotfeatures = nullptr;

    bool* isDetermined = nullptr;
    int* determinedBy = nullptr;

    void createAggregateRegister(); 

    /**
     * Method to register to d-tree nodes by generating aggregates bottom up.
     */
    void registerAggregatesFromModel(DTreeNode* node);
};

#endif /* INCLUDE_ML_COUNT_H_ */
