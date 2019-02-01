//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_DTREENODE_HPP_
#define INCLUDE_DTREENODE_HPP_

#include <string>
#include <vector>

/**
 * This class represents one node in the factorisation tree. Node itself can contain number of children nodes.
 * All children nodes are stored as a linked list.
 */
class DTreeNode
{

public:

	/**
	 * Constructor by name and ID sets valueType to zero (Integer)
	 * valueType must be updated for non-integer variables.
	 */
	DTreeNode(std::string name, int ID)
	{
		this->_name = name;
		this->_id = ID;
		this->_parent = NULL;
		this->_numOfChildren = 0;
		this->_firstChild = NULL;
		this->_lastChild = NULL;
		this->_next = NULL;
		this->_prev = NULL;
		this->_caching = false;
	}

	~DTreeNode()
	{
		if (this->_childrenIDs != NULL)
			delete[] this->_childrenIDs;
	}

	//! Name of the node (attribute/relation name).
	std::string _name;

	//! ID of the node.
	int _id;

	//! Parent of the current node
	DTreeNode* _parent;

	//! Number of children of current node.
	int _numOfChildren;

	//! Pointer to the first child.
	DTreeNode* _firstChild;

	//! Pointer to the last child.
	DTreeNode* _lastChild;

	//! Pointer to next (from current) child inside its parent (sibling).
	DTreeNode* _next;

	// Pointer to previous (from current) child inside it's parent (sibling)
	DTreeNode* _prev;

	//! Schema to store the children of that node.
	std::vector<int> _schema;
	std::vector<int> _key;

	//! Indicates whether caching is used in this node.
	bool _caching;

	//! IDs of the children.
	int* _childrenIDs = NULL;

	/**
	 * Adds node to child list by appending to the end of linked list.
	 */
	void addChild(DTreeNode* node)
	{
		if (node == NULL)
			return;
		if (this->_firstChild == NULL)
		{
			/* It is first child. */
			this->_firstChild = node;
			this->_lastChild = node;
			node->_next = NULL;
			node->_prev = NULL;
		}
		else
		{
			/* We know there is already at least one child. */
			this->_lastChild->_next = node;
			node->_prev = this->_lastChild;
			this->_lastChild = node;
			node->_next = NULL;
		}
		++this->_numOfChildren;
		node->_parent = this;
	}
};

#endif /* INCLUDE_DTREENODE_HPP_ */
