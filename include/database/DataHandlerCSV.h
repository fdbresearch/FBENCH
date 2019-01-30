//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//
//
#ifndef INCLUDE_DATAHANDLERCSV_H_
#define INCLUDE_DATAHANDLERCSV_H_

#include <DataHandler.hpp>

/**
 * Class that enables to build the in-memory representation of the database using CSV files, and to deal with the data to be processed by the engine.
 */
class DataHandlerCSV: public DataHandler
{

public:

	/**
	 * Constructor; need to provide path to database files.
	 */
	DataHandlerCSV(const std::string& pathToFiles, const char delimiter);

	~DataHandlerCSV();

	/**
	 * Loads the tables from CSV files into memory.
	 */
	void loadAllTables();

	/**
	 * Returns the data to process, which will be a merger between _localDataToProcess and _receivedDataToProcess.
	 */
	dfdb::types::Database getDataToProcess();

};

#endif /* INCLUDE_DATAHANDLERCSV_H_ */
