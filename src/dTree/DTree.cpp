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
#include <sstream>
#include <algorithm>

#include <DTree.h>
#include <Logging.hpp>

#include <set>

#define BUF_SIZE 1024

static const char ATTRIBUTE_SEPARATOR_CHAR = ',';
static const char COMMENT_CHAR = '#';
static const char PARAMETER_SEPARATOR_CHAR = ' ';

using namespace std;

DTree::DTree(string fileName)
{
	buildFromFile(fileName);
	extendDTree(_root);
}

DTree::~DTree(void)
{
	for (int i = 0; i < _numOfAttributes + _numOfRelations; ++i)
		if (_nodes[i] != NULL)
			delete _nodes[i];
}

int DTree::numberOfAttributes()
{
	return _numOfAttributes;
}

int DTree::numberOfRelations()
{
	return _numOfRelations;
}

DTreeNode* DTree::getNode(int index)
{
	if (index < 0 || index >= _numOfAttributes + _numOfRelations)
		return NULL;
	if (_nodes[index] == NULL)
	{
		return _nodes[index];
	}
	return _nodes[index];
}

DTreeNode* DTree::getNode(string relationName)
{
	unordered_map<string, int>::iterator result = _nodesMap.find(relationName);
	if (result == _nodesMap.end())
	{
		return NULL;
	}
	return getNode((*result).second);
}

int DTree::getIndexByName(string relationName)
{
	unordered_map<string, int>::iterator result = _nodesMap.find(relationName);
	if (result == _nodesMap.end())
		return -1;
	return (*result).second;
}

void DTree::extendDTree(DTreeNode* node)
{
	node->_schema.push_back(node->_id);
	node->_childrenIDs = new int[node->_numOfChildren + 1];

	/* Copy the schema from all children up. */
	DTreeNode* child = node->_firstChild;
	for (int i = 0; i < node->_numOfChildren; ++i)
	{
		extendDTree(child);
		node->_schema.insert(node->_schema.end(), child->_schema.begin(),
				child->_schema.end());
		node->_childrenIDs[i] = child->_id;
		child = child->_next;
	}
}

void DTree::buildFromFile(string fileName)
{
	/* Load the DTree config file into an input stream. */
	ifstream input(fileName);

	if (!input)
	{
		ERROR(fileName+" does not exist. \n");
		exit(1);
	}

	/* Number of attributes and tables. */
	int n, m;
	string nString, mString;

	/* String and associated stream to receive lines from the file. */
	string line;
	stringstream ssLine;

	/* String to receive the attributes from the relation. */
	string attr;

	/* Extract number of attributes and tables. */
	while (getline(input, line))
	{
		if (line[0] == COMMENT_CHAR || line == "")
			continue;

		break;
	}

	ssLine << line;

	getline(ssLine, nString, PARAMETER_SEPARATOR_CHAR);
	getline(ssLine, mString, PARAMETER_SEPARATOR_CHAR);

	ssLine.clear();

	/* Convert the strings to integers. */
	n = stoi(nString);
	m = stoi(mString);

	vector<vector<int> > attributes;

	this->_numOfAttributes = n;
	this->_numOfRelations = m;
	this->_nodes.clear();
	this->_nodes.resize(n + m, NULL);
	this->_nodesMap.clear();
	this->_attr = attributes;

	/* Read all the attributes - name and value - and parent and key. */
	string name;
	string type;
	string key;
	string index;
	string parent;
	string caching;

	for (int i = 0; i < n; ++i)
	{
		/* Extract node information for each attribute. */
		while (getline(input, line))
		{
			if (line[0] == COMMENT_CHAR || line == "")
				continue;

			break;
		}

		ssLine << line;

		/* Get the six parameters specified for each attribute. */
		getline(ssLine, index, PARAMETER_SEPARATOR_CHAR);
		getline(ssLine, name, PARAMETER_SEPARATOR_CHAR);
		getline(ssLine, type, PARAMETER_SEPARATOR_CHAR);
		getline(ssLine, parent, PARAMETER_SEPARATOR_CHAR);
		getline(ssLine, key, PARAMETER_SEPARATOR_CHAR);
		getline(ssLine, caching, PARAMETER_SEPARATOR_CHAR);

		/* Clear string stream. */
		ssLine.clear();

		if (i != stoi(index))
			ERROR("Inconsistent index specified in your DTree config file.\n");

		this->_nodes[i] = new DTreeNode(name, i);
		this->_nodesMap[name] = i;
		this->_nodes[i]->_caching = (caching != "0");

		key.erase(key.begin());
		key.erase(key.end() - 1);

		ssLine << key;

		while (getline(ssLine, attr, ATTRIBUTE_SEPARATOR_CHAR))
		{
			this->_nodes[i]->_key.push_back(atoi(attr.c_str()));
		}

		if (stoi(parent) == -1)
		{
			this->_root = this->_nodes[i];
			this->_nodes[i]->_parent = NULL;
		}
		else
		{
			this->_nodes[stoi(parent)]->addChild(this->_nodes[i]);
		}
		/* Clear string stream. */
		ssLine.clear();
	}

	/* Read all the relation names, their parents and attributes. */
	for (int i = 0; i < m; ++i)
	{
		/* Extract node information for each relation. */
		while (getline(input, line))
		{
			if (line[0] == COMMENT_CHAR || line == "")
				continue;

			break;
		}

		ssLine << line;

		/* Get the three parameters specified for each relation. */
		getline(ssLine, name, PARAMETER_SEPARATOR_CHAR);
		getline(ssLine, parent, PARAMETER_SEPARATOR_CHAR);
		getline(ssLine, key, PARAMETER_SEPARATOR_CHAR);

		/* Clear string stream. */
		ssLine.clear();

		this->_nodes[n + i] = new DTreeNode(name, n + i);
		this->_nodesMap[name] = n + i;
		this->_nodes[n + i]->_parent = this->_nodes[stoi(parent)];

		ssLine << key;

		vector<int> temp;
		while (getline(ssLine, attr, ATTRIBUTE_SEPARATOR_CHAR))
		{
			temp.push_back(_nodesMap[attr]);
		}

		this->_attr.push_back(temp);

		/* Clear string stream. */
		ssLine.clear();
	}
}

/**
 * Build the ancestor map by preorder traversal 
 */
static void buildAnc(DTreeNode* node, int parentID, std::unordered_map<int, std::set<int>>& ancMap) {

	// ancMap
	// anc = anc(parent) + parent
	std::set<int> ancs;
	if (parentID != -1) {
		ancs.insert(ancMap[parentID].begin(), ancMap[parentID].end());
		ancs.insert(parentID);
	}
	ancMap.insert({node->_id, ancs});

	DTreeNode* child = node->_firstChild;
	for (int i = 0; i < node->_numOfChildren; ++i)
	{
		buildAnc(child, node->_id, ancMap);
		child = child->_next;
	}

	return;
}

/**
 * Get all root-to-leaf paths
 */
static void getPaths(DTreeNode* node, std::unordered_map<int, std::set<int>>& ancMap, std::set<std::set<int>>& paths) {
	
	// leaf node
	if (node->_numOfChildren == 0) {
		std::set<int> path(ancMap[node->_id]);
		path.insert(node->_id);
		paths.insert(path);
	}

	DTreeNode* child = node->_firstChild;
	for (int i = 0; i < node->_numOfChildren; ++i)
	{
		getPaths(child, ancMap, paths);
		child = child->_next;
	}

	return;
}

/**
 * Attributes from a relation should be on a single root-to-leaf path
 */
static void checkPaths(DTreeNode* node, DTree* dtree, std::unordered_map<int, std::set<int>>& ancMap) {

	// get all root-to-leaf paths	
	std::set<std::set<int>> paths;
	getPaths(node, ancMap, paths);
	
	// attrs from each relation
	for (auto attrs : dtree->_attr) {
		bool found = false;
		for (auto path : paths) {

			std::set<int> attrsSet(attrs.begin(), attrs.end());

			// attrs should be a subset of the path
			if (std::includes(path.begin(), path.end(), attrsSet.begin(), attrsSet.end())) {
				found = true;
				break;
			}
		}
		if (!found) {
			// report the relation
			std::string s;
			for (auto attrId : attrs) {
				s += dtree->_nodes[attrId]->_name;
				s += ", ";
			}
			std::cout << "The attributes " << s << "come from the same relation but they are not in a single root-to-leaf path." << std::endl;
		}
	}
}

/**
 * Return all attributes that are in the same relation with A
 */
static std::set<int> schemaOf(int id, DTree* dtree) {
	// union of all _attr that contain the id
	std::set<int> schema;
	for (auto attrs : dtree->_attr) {
		// if the attr vector contains the target id
		if (std::find(attrs.begin(), attrs.end(), id) != attrs.end()) {
			schema.insert(attrs.begin(), attrs.end());
		}
	}
	return schema;
}

/**
 * Check the key sets of attributes:
 * 1. key(A) is a subset anc(A)
 * 2. schema(A) intersects anc(A) is a subset of key(A)
 * 3. for any child B of A, key(B) is a subset of key(A) \union A
 * schema(A) means the attributes that are in the same relation with A
 */
static void checkKey(DTreeNode* node, DTree* dtree, std::unordered_map<int, std::set<int>>& ancMap) {

	// process the node
	// 1. key(A) is a subset anc(A)
	std::set<int> keys(node->_key.begin(), node->_key.end());
	std::set<int> ancs = ancMap[node->_id];
	if (!std::includes(ancs.begin(), ancs.end(), keys.begin(), keys.end())) {
		// assertion 1 is violated
		std::cout << "The key(" << node->_name << ") should be the subset of anc(" << node->_name << ")." << std::endl;
	}

	// 2. schema(A) intersects anc(A) is a subset of key(A)
	std::set<int> schema = schemaOf(node->_id, dtree);
	std::set<int> intersection;
	std::set_intersection(schema.begin(), schema.end(), ancs.begin(), ancs.end(), std::inserter(intersection, intersection.begin()));

	if (!std::includes(keys.begin(), keys.end(), intersection.begin(), intersection.end())) {
		// assertion 2 is violated
		std::cout << "The schema attributes of " << node->_name << " should be a subset of key(" << node->_name << ") set." << std::endl;
	}

	// key(A) union A
	std::set<int> unionKeys(keys.begin(), keys.end());
	unionKeys.insert(node->_id);

	// recursion
	DTreeNode* child = node->_firstChild;
	for (int i = 0; i < node->_numOfChildren; ++i)
	{
		// 3. for any child B of A, key(B) is a subset of key(A) \union A
		
		if (!std::includes(unionKeys.begin(), unionKeys.end(), child->_key.begin(), child->_key.end())) {
			// assertion 3 is violated
			std::cout << "The key(" << child->_name << ") should be a subset of key(" << node->_name << ")." << std::endl;
		}

		checkKey(child, dtree, ancMap);
		child = child->_next;
	}

	return;
}



/**
 * Check the validation of the dtree
 */
void DTree::isValid() {

	// add the anc set to each variable
	// use a hashmap to mapping from attr->ancs
	std::unordered_map<int, std::set<int>> ancMap = {}; // attr -> anc set

	// TODO: sanity check of spelling and indexing

	// build the anc sets for attrs
	buildAnc(this->_root, -1, ancMap);

	// Attributes from a relation should be on a single root-to-leaf path
	checkPaths(this->_root, this, ancMap);

	// three conditions of dtree nodes
	checkKey(this->_root, this, ancMap);

	std::cout << "Completed the dtree checking." << std::endl;
}

