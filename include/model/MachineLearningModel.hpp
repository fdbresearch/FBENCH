//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#ifndef INCLUDE_ML_MACHINELEARNINGMODEL_HPP_
#define INCLUDE_ML_MACHINELEARNINGMODEL_HPP_

#include <memory>

#include <DataHandler.hpp>

/**
 * Abstract class to provide a uniform interface used to process the received data.
 */
class MachineLearningModel
{

public:

	virtual ~MachineLearningModel()
	{
	}

	/**
	 * Runs the data processing task.
	 */
	virtual void run() = 0;

	/**
	 * Returns the features extracted from the configuration files.
	 */
	virtual int** getFeatures() = 0;

	/**
	 * Returns the cofactor matrix produced by the engine.
	 */
	virtual dfdb::types::ResultType* getCofactorMatrix() = 0;

protected:

	//! Method calculates the number of combinations (nCk)
	unsigned int choose(unsigned int n, unsigned int k)
	{
		if (k > n)
		{
			return 0;
		}
		unsigned int r = 1;
		for (unsigned int d = 1; d <= k; ++d)
		{
			r *= n--;
			r /= d;
		}
		return r;
	}
};

#endif /* INCLUDE_ML_MACHINELEARNINGMODEL_HPP_ */
