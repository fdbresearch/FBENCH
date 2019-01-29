//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_ML_MATJOIN_H_
#define INCLUDE_ML_MATJOIN_H_

#include <string>
#include <vector>

#include <CompilerUtils.hpp>
#include <DataTypes.hpp>
#include <DTree.h>
#include <DTreeNode.hpp>
#include <MachineLearningModel.hpp>
#include <FactorisedJoin.h>

//! Forward declarations to allow pointer to launcher and DTree without cyclic includes.
class DTree;
class FactorisedJoin;
class Launcher;

/**
 * Class that launches regression model on the data, using d-trees.
 */
class CountOverJoin: public MachineLearningModel
{
public:

	/**
	 * Constructor; need to provide path to configuration files, pointer to
	 * Launcher module in order to retrieve DataHandler instance, and number
	 * of threads and partitions to work on.
	 */
	CountOverJoin(
            const std::string& pathToFiles, std::shared_ptr<Launcher> launcher);

	~CountOverJoin();

	/**
	 * Runs F to compute a regression model.
	 */
	void run();

        /**
	 * Returns the features extracted from the configuration files.
	 */
	int** getFeatures(){ return NULL;};

	// /**
	//  * Returns the cofactor matrix produced by the engine.
	//  */
	dfdb::types::ResultType* getCofactorMatrix(){ return NULL; };

private:

	//! Physical path to the schema and table files.
	std::string _pathToFiles;

	std::shared_ptr<Launcher> _launcher;
	
	//! Aggregate Engine used to compute the aggregates for cofactor matrices. 
	std::shared_ptr<FactorisedJoin> _joinEngine;

	//! Pointer to the d-tree.
	std::shared_ptr<DTree> _dTree;

	node::Union* rootNode; 

	unsigned int count(node::Union* factorisationNode, DTreeNode* dTreeNode);

	void valueCount(node::Union* factorisationNode, DTreeNode* dTreeNode); 

	unsigned int *cachedCounts;

	unsigned long numberOfValues = 0; 
};

#endif /* INCLUDE_ML_MATJOIN_H_ */
