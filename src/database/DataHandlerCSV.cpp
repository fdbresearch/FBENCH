//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <cassert>
#include <fstream>

#include <DataHandlerCSV.h>
#include <GlobalParams.hpp>
#include <Logging.hpp>

/* Constant chars used for parsing the config and data files. */
static const char ATTRIBUTE_SEPARATOR_CHAR = ',';
static const char COMMENT_CHAR = '#';
static const char TABLE_NAME_CHAR = ':';
static const char VALUE_SEPARATOR_CHAR = '|';

static const std::string SCHEMA_CONF = "/schema.conf";

/* Declaration of the extern variable in GlobalParams.hpp. */
#ifndef TABLES
size_t dfdb::params::NUM_OF_TABLES;
#endif

namespace phoenix = boost::phoenix;
using namespace boost::spirit;
using namespace dfdb::params;
using namespace dfdb::types;
using namespace std;

DataHandlerCSV::DataHandlerCSV(const string& pathToFiles, const char delimiter) :
		DataHandler(pathToFiles, delimiter)
{
	/* Load the database schema file into an input stream. */
	ifstream input(_pathToFiles + SCHEMA_CONF);
    
    if (!input)
    {
        ERROR (_pathToFiles + SCHEMA_CONF + " does is not exist. \n");
        exit(1);
    }
    
	/* String and associated stream to receive lines from the file. */
	string line;
	stringstream ssLine;

	size_t numOfTables = 0;

	/* Count the number of tables in the database. */
	while (getline(input, line))
	{
		if (line[0] == COMMENT_CHAR || line == "")
			continue;
		++numOfTables;
	}

	/* Check consistency between compile time flag and runtime value issued from configuration file. */
#ifdef TABLES
	if(TABLES != numOfTables)
	{
		ERROR("Value of compiler flag -DTABLES and number of tables specified in schema.conf inconsistent. Aborting.\n")
		exit(1);
	}
	/* Assign value to extern variable in GlobalParams.hpp. */
#else
	NUM_OF_TABLES = numOfTables;
#endif

	/* Initialise arrays; each element will correspond to one database table. */
	_tableNames = new string[NUM_OF_TABLES];
	_attrIDs = new vector<uint_fast16_t> [NUM_OF_TABLES];

	/* Reset input stream to beginning of file. */
	input.clear();
	input.seekg(0, ios::beg);

	/* Variable defining the ID of each attribute. */
	uint_fast16_t attributeID = 0;

	size_t table = 0;
	/* Scan through the input stream line by line. */
	while (getline(input, line))
	{
		if (line[0] == COMMENT_CHAR || line == "")
			continue;

		ssLine << line;

		string tableName;
		/* Extract the name of the table in the current line. */
		getline(ssLine, tableName, TABLE_NAME_CHAR);
		_tableNames[table] = tableName;

		string attrName;
		/* Scan through the attributes in the current line. */
		while (getline(ssLine, attrName, ATTRIBUTE_SEPARATOR_CHAR))
		{
			/* Attribute encountered for the first time in the schema: give it a new ID. */
			if (_namesMapping.count(attrName) == 0)
			{
				_namesMapping[attrName] = attributeID;
				_attrIDs[table].push_back(attributeID);
				++attributeID;
			}
			/* Attribute already encountered: just add its ID to the vector of IDs for the current table. */
			else
			{
				_attrIDs[table].push_back(_namesMapping[attrName]);
			}
		}
		++table;
		/* Clear string stream. */
		ssLine.clear();
	}
	assert(
			table == NUM_OF_TABLES
					&& "The same number of lines must be processed during the first and the second pass over the schema file.");

	/* Initialise array; each element will correspond to one orker in the network. */
	_receivedDataToProcess = new Database[NUM_OF_WORKERS];
	for (size_t worker = 0; worker < NUM_OF_WORKERS; ++worker)
	{
		_receivedDataToProcess[worker] = new Table[NUM_OF_TABLES];
	}
}

DataHandlerCSV::~DataHandlerCSV()
{
	/* Clean the database tuples. We must not clean the _localDataToProcess structure, as it contains tuples from the other structures. */
	for (size_t table = 0; table < NUM_OF_TABLES; ++table)
	{
		for (Tuple tuple : _database[table])
		{
			delete[] tuple;
		}

		for (size_t worker = 0; worker < NUM_OF_WORKERS; ++worker)
		{
			for (Tuple tuple : _receivedDataToProcess[worker][table])
			{
				delete[] tuple;
			}
		}
	}

	for (size_t worker = 0; worker < NUM_OF_WORKERS; ++worker)
	{
		delete[] _receivedDataToProcess[worker];
	}

	/* Clean the database tables. */
	delete[] _database;
	delete[] _localDataToProcess;
	delete[] _receivedDataToProcess;

	/* Clean other structures. */
	delete[] _attrIDs;
	delete[] _tableNames;

	INFO("MAIN: database structures cleaned up; ready to shutdown.\n");
}

void DataHandlerCSV::loadAllTables()
{
	/* Initialise the database and DataToProcess as arrays of tables. */
	_database = new Table[NUM_OF_TABLES];
	_localDataToProcess = new Table[NUM_OF_TABLES];

	/* String to receive lines (ie. tuples) from the file. */
	string line;

	/* Column index in the current tuple. */
	int_fast16_t column = 0;

	/* Bool to check whether parsing was successful. */
	bool parsingSuccess = true;

	for (size_t table = 0; table < NUM_OF_TABLES; ++table)
	{
		/* Load the current table file into an input stream. */
		ifstream input(_pathToFiles + "/" + _tableNames[table] + ".tbl");
        
        if (!input)
        {
            ERROR (_pathToFiles + "/" + _tableNames[table] + ".tbl" + " does is not exist. \n");
            exit(1);
        }

        DINFO("Loading "+_tableNames[table]+". \n");

		/* Scan through the tuples in the current table. */
		while (getline(input, line))
		{
			/* Create a new tuple. */
			Tuple tuple = new DataType[_attrIDs[table].size()];

			column = -1;

			/* Parse each double one by one; skip _delimiter. */
			parsingSuccess = qi::phrase_parse(line.begin(), line.end(),

					/*  Begin Boost Spirit grammar. */
					(repeat(_attrIDs[table].size())[qi::double_[phoenix::ref(
							tuple)[++phoenix::ref(column)] = qi::_1]]),
					/*  End grammar. */
					_delimiter);

			assert(parsingSuccess && "The parsing of a tuple has failed.");

			/* Add the tuple to the current table. */
			_database[table].push_back(tuple);
		}

	}
	INFO("MAIN: " + to_string(NUM_OF_TABLES) + " tables loaded into memory.\n");
}

Database DataHandlerCSV::getDataToProcess()
{
    return _database;
                
    /* Only one worker: no data distribution, return local database. */
	if (NUM_OF_WORKERS == 1)
		return _database;
	/*
	 * We have to merge all the received databases and the local one in order to obtain a unique database needed for processing.
	 * The first step consists in reallocating the needed capacity in _localDataToProcess; doing a single reallocation beforehand
	 * is more efficient than letting the insert method handle it with a worse case scenario of one reallocation per worker (one per insert below).
	 */
	for (size_t table = 0; table < NUM_OF_TABLES; ++table)
	{
		size_t sizeToReallocate = _localDataToProcess[table].size();

		for (size_t worker = 0; worker < NUM_OF_WORKERS; ++worker)
		{
			sizeToReallocate += _receivedDataToProcess[worker][table].size();
		}
		/* Increase capacity of the vector by adding up the sizes of all the vectors we are going to insert. */
		_localDataToProcess[table].reserve(sizeToReallocate);
	}

	/* Merge all the tables of _receivedDataToProcess into _localDataToProcess. */
	for (size_t table = 0; table < NUM_OF_TABLES; ++table)
	{
		for (size_t worker = 0; worker < NUM_OF_WORKERS; ++worker)
		{
			_localDataToProcess[table].insert(_localDataToProcess[table].end(),
					_receivedDataToProcess[worker][table].begin(),
					_receivedDataToProcess[worker][table].end());
		}
	}
	return _localDataToProcess;

}
