//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_DTREE_H_
#define INCLUDE_DTREE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <DTreeNode.hpp>

/**
 * Class that represents a FactorisationTree.
 */
class DTree
{

public:

	/**
	 * Constructor; need to provide path to configuration files.
	 */
	DTree(std::string fileName);

	~DTree();

	//! Pointer to the root node.
	DTreeNode* _root;

	//! Vector storing the IDs for each attribute in each table.
	std::vector<std::vector<int> > _attr;

	//! Vector of nodes representing the tree.
	std::vector<DTreeNode*> _nodes;

	//! Map between the attribute names and their IDs.
	std::unordered_map<std::string, int> _nodesMap;

	/**
	 * Returns the number of attributes.
	 */
	int numberOfAttributes();

	/**
	 * Returns the number of relations.
	 */
	int numberOfRelations();

	/**
	 * Returns the number of nodes in the tree.
	 */
	int numberOfNodes();

	/**
	 * Returns a node by id.
	 */
	DTreeNode* getNode(int index);

	/**
	 * Returns a node by name.
	 */
	DTreeNode* getNode(std::string relationName);

	/**
	 * Returns the index of a node.
	 */
	int getIndexByName(std::string relationName);

	/**
	 * Builds the DTree from a configuration file.
	 */
	void buildFromFile(std::string fileName);

	/**
	 * Extends the DTree with the schema information.
	 */
	void extendDTree(DTreeNode* node);

	/**
	 * Check the validation of the dtree.
	 */
	void isValid();

private:

	//! Number of attributes in the database.
	int _numOfAttributes;

	//! Number of relations in the database.
	int _numOfRelations;
};

#endif /* INCLUDE_DTREE_H_ */
