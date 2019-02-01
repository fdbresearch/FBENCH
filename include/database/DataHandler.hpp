//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_DATAHANDLER_HPP_
#define INCLUDE_DATAHANDLER_HPP_

#include <string>
#include <unordered_map>
#include <vector>

#include <DataTypes.hpp>

/**
 * Abstract class to provide a uniform interface used to load the data in main memory and to deal with the data to be processed by the engine.
 */
class DataHandler
{

public:

	virtual ~DataHandler()
	{
	}

	/**
	 * Loads the tables into memory.
	 */
	virtual void loadAllTables() = 0;

	/**
	 * Returns the data to process, which is a merger between _localDataToProcess and _receivedDataToProcess.
	 */
	virtual dfdb::types::Database getDataToProcess() = 0;

	/**
	 * Returns the names of the tables in the database.
	 */
	virtual std::string* getTableNames()
	{
		return _tableNames;
	}

	/**
	 * Returns a pointer to the the array of table attribute vectors.
	 */
	virtual std::vector<uint_fast16_t>* getTableAttributes()
	{
		return _attrIDs;
	}

	/**
	 * Returns the local database (ie. data loaded from disk).
	 */
	virtual dfdb::types::Database getDatabase()
	{
		return _database;
	}

	/**
	 * Returns the local data to process (ie. data loaded from disk that must be kept for local processing).
	 */
	virtual dfdb::types::Database getLocalDataToProcess()
	{
		return _localDataToProcess;
	}

	/**
	 * Returns the received data to process array.
	 */
	virtual dfdb::types::Database* getReceivedDataToProcess()
	{
		return _receivedDataToProcess;
	}

	/**
	 * Returns the mapping from attribute names to their IDs.
	 */
	virtual std::unordered_map<std::string, uint_fast16_t>& getNamesMapping()
	{
		return _namesMapping;
	}

protected:

	/**
	 * Superclass constructor; used to set _pathToFiles variable.
	 */
	DataHandler(const std::string& pathToFiles, const char delimiter) :
			_pathToFiles(pathToFiles), _delimiter(delimiter)
	{
	}

	//! Physical path to the schema and table files.
	std::string _pathToFiles;

	//! Physical path to the schema and table files.
	char _delimiter;
	
	//! Array of vectors containing the attribute IDs of the different database tables.
	std::vector<uint_fast16_t>* _attrIDs;

	//! Mapping from attribute names to their IDs.
	std::unordered_map<std::string, uint_fast16_t> _namesMapping;

	//! Array containing the names of the database tables.
	std::string* _tableNames;

	//! Represents the database, in other words the tuples that are stored on the disk of the local node.
	dfdb::types::Database _database;

	//! Represents a temporary database that stores the local data for processing.
	dfdb::types::Database _localDataToProcess;

	//! Represents an array of temporary databases that store the received data for processing from each node.
	dfdb::types::Database* _receivedDataToProcess;

};

#endif /* INCLUDE_DATAHANDLER_HPP_ */
