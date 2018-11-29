/* mainTest.cpp */
#include <boost/mpi.hpp>
#include "repast_hpc/RepastProcess.h"
#include "agentModelTest.h"
int main(int argc, char** argv){ // cfg file name's 'Arg 1', props file name's 'Arg 2' 
	std::string configFile = argv[1]; std::string propsFile  = argv[2]; 
	boost::mpi::environment env(argc, argv); boost::mpi::communicator world; // init MPI environment; boost mpi 'communicator' instance (var world)
	repast::RepastProcess::init(configFile); // init the Repast Process w/ the cfg file
	RepastHPCDemoModel* model = new RepastHPCDemoModel(propsFile, argc, argv, &world);
	repast::ScheduleRunner& runner = repast::RepastProcess::instance()->getScheduleRunner(); 
// ^ retrieves a handle to an obj managing the timing by which RepastHPC events take place
	model->init(); model->initSchedule(runner); // the handle's then passed to the model, so the latter can add events to the schedule
	runner.run(); // starts the event processing from the event schedule
	delete model; repast::RepastProcess::instance()->done();
}
