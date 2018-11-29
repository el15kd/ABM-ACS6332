/* Demo_02_Main.cpp */
#include <boost/mpi.hpp> // includes all Boost.MPI library headers
#include "repast_hpc/RepastProcess.h" // Encapsulates the process in which repast is running and manages interprocess communication
#include "Demo_02_Model.h"
int main(int argc, char** argv){ // cfg file name's 'Arg 1', props file name's 'Arg 2'
	std::string configFile = argv[1]; // The name of the configuration file is Arg 1
	std::string propsFile  = argv[2]; // The name of the properties file is Arg 2
	boost::mpi::environment env(argc, argv);
	boost::mpi::communicator world; // init MPI environment; boost mpi 'communicator' instance (var world)
	repast::RepastProcess::init(configFile); // init the Repast Process w/ the cfg file
	RepastHPCDemoModel* model = new RepastHPCDemoModel(propsFile, argc, argv, &world);
	repast::ScheduleRunner& runner = repast::RepastProcess::instance()->getScheduleRunner(); // retrieves a handle to an obj managing the timing by which Repast events take place
	model->init(); model->initSchedule(runner); // the handle's then passed to the model, so the latter can add events to the schedule
	runner.run(); // starts the event processing from the event schedule
	delete model; repast::RepastProcess::instance()->done();
}
