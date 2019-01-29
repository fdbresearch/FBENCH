//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_NODEINFO_HPP_
#define INCLUDE_NODEINFO_HPP_

#include <string>

/**
 * Class that provides a container and methods to hold the information of a given node.
 */
class NodeInfo
{

public:

	/**
	 * Constructor; need to provide ID, name, IP and port.
	 */
	NodeInfo(uint16_t nodeID, const std::string& nodeName,
			const std::string& nodeIP, const std::string& syncPort = "11110") :
			_nodeID(nodeID), _nodeName(nodeName), _nodeIP(nodeIP), _syncPort(
					(uint16_t) std::stoul(syncPort))
	{
	}

	/**
	 * Returns the ID of the node.
	 */
	uint16_t getNodeID()
	{
		return _nodeID;
	}

	/**
	 * Returns the name of the node.
	 */
	std::string getNodeName()
	{
		return _nodeName;
	}

	/**
	 * Returns the IP of the node.
	 */
	std::string getNodeIP()
	{
		return _nodeIP;
	}

	/**
	 * Returns the synchronisation port of the node.
	 */
	uint16_t getSyncPort()
	{
		return _syncPort;
	}

	/**
	 * Returns the data port of the node: syncPort + 1.
	 */
	uint16_t getDataPort()
	{
		return _syncPort + (uint16_t) 1;
	}

private:

	//! ID of the node.
	uint16_t _nodeID;

	//! Name of the node; used for logging purposes only.
	std::string _nodeName;

	//! IP address of the node.
	std::string _nodeIP;

	//! Port used for synchronisation.
	uint16_t _syncPort;

};

#endif /* INCLUDE_NODEINFO_HPP_ */
