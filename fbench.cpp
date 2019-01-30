//===----------------------------------------------------------------------===//
//
// FBench
//
// https://fdbresearch.github.io/
//
// Copyright (c) 2019, FDB Research Group, University of Oxford
// 
//===----------------------------------------------------------------------===//

#include <boost/program_options.hpp>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <thread>

#include <Launcher.h>
#include <Logging.hpp>

int main(int argc, char *argv[])
{
    /* Object containing a description of all the options available in the
       program. */
    boost::program_options::options_description desc("FBench - allowed options");
    desc.add_options()
	/* Option to show help. */
	("help", "produce help message.")
	/* Option to show some info. */
	("info", "show some information about the program.")
	/* Option for path to data and configuration files. */
	("path", boost::program_options::value<std::string>(),
         "set path for data and configuration files - required.")
	/* Option for machine learning model. */
	("model",
         boost::program_options::value<std::string>()->default_value("count"),
         "operation to be computed: count (default) or countJoin.")
	/* Option for number of threads used for F cofactor calculation; default
    * set to number of hardware thread contexts. */
    ("threads", boost::program_options::value<unsigned int>()->default_value(1),
         "set number of threads for count calculation; default is 1 for single-threading.")
	/* Option for number of partitions used for F cofactor calculation. */
	// ("partitions", boost::program_options::value<unsigned int>(),
    //   "set number of partitions for cofactor calculation; default is one partition per thread.")
    /* Option for number of partitions used for F cofactor calculation. */
    ("delim", boost::program_options::value<char>()->default_value('|'),
         "Set the delimiter for the input tables; default is '|'.");

    /* Register previous options and do command line parsing. */
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);

    /* Display help. */
    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return EXIT_SUCCESS;
    }

    /* Display info. */
    if (vm.count("info"))
    {
        std::cout << "FBench\n";
        std::cout << "Compiled on " << __DATE__ << " at " << __TIME__ << ".\n";
#ifdef BENCH
        std::cout << "Build type: Benchmark.\n";
#elif defined NDEBUG
        std::cout << "Build type: Release.\n";
#else
        std::cout << "Build type: Debug.\n";
#endif
        return EXIT_SUCCESS;
    }

    std::string pathString;

    /* Retrieve compulsory path. */
    if (vm.count("path"))
    {
        pathString = vm["path"].as<std::string>();
    }
    else
    {
        ERROR("You must specify a path containing the database and "<<
              "configuration files.\n");
        ERROR("Run the program with --help for more information about "<<
              "command line options.\n");
        return EXIT_FAILURE;
    }

    int numOfPartitions = vm["threads"].as<unsigned int>();
    /* Retrieve number of partitions; either given by option or default of one partition per thread. */
    /*    
    if (vm.count("partitions"))
    {
        numOfPartitions = vm["partitions"].as<unsigned int>();
    }
    else
    {
        numOfPartitions = vm["threads"].as<unsigned int>();
    }
    */

    std::shared_ptr<Launcher> launcher(new Launcher(pathString));

#ifdef BENCH
    int64_t start = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
#endif

    /* Launch program. */
    int result = launcher->launch(vm["threads"].as<unsigned int>(),
                                  numOfPartitions,
                                  vm["model"].as<std::string>(),
                                  vm["delim"].as<char>());
    
#ifdef BENCH
    int64_t end = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - start;    
#endif

    BINFO("TIME - overall time: " + std::to_string(end) + "ms.\n");

    /* Make sure the output buffer is flushed (no need to flush cerr as it
     * is unbuffered). */
    std::cout.flush();

    return result;
}
