/* agentModelTest.h */
#ifndef agentModel
#define agentModel
#include <boost/mpi.hpp>
#include "repast_hpc/Schedule.h"
#include "repast_hpc/Properties.h"
#include "repast_hpc/SharedContext.h"
#include "repast_hpc/AgentRequest.h"
#include "repast_hpc/TDataSource.h"
#include "repast_hpc/SVDataSet.h" // Data collector
#include "repast_hpc/SharedNetwork.h"
#include "agentTest.h"
/* Agent Package Provider */
class RepastHPCDemoAgentPackageProvider {
private:
    repast::SharedContext<RepastHPCDemoAgent>* agents;
public:
    RepastHPCDemoAgentPackageProvider(repast::SharedContext<RepastHPCDemoAgent>* agentPtr);
    void providePackage(RepastHPCDemoAgent * agent, std::vector<RepastHPCDemoAgentPackage>& out);
    void provideContent(repast::AgentRequest req, std::vector<RepastHPCDemoAgentPackage>& out);	
};
/* Agent Package Receiver */
class RepastHPCDemoAgentPackageReceiver {
private:
    repast::SharedContext<RepastHPCDemoAgent>* agents;	
public:
    RepastHPCDemoAgentPackageReceiver(repast::SharedContext<RepastHPCDemoAgent>* agentPtr);
    RepastHPCDemoAgent * createAgent(RepastHPCDemoAgentPackage package);
    void updateAgent(RepastHPCDemoAgentPackage package);
};
/* Data Collection */
class DataSource_AgentTotals : public repast::TDataSource<int>{
private:
	repast::SharedContext<RepastHPCDemoAgent>* context;
public:
	DataSource_AgentTotals(repast::SharedContext<RepastHPCDemoAgent>* c);
	int getData();
};
class DataSource_AgentCTotals : public repast::TDataSource<int>{
private:
	repast::SharedContext<RepastHPCDemoAgent>* context;	
public:
	DataSource_AgentCTotals(repast::SharedContext<RepastHPCDemoAgent>* c);
	int getData();
};
/* "World" Class */
class RepastHPCDemoModel{ // model class, the 'arena' in which the agent population are stored; achieved by providing model class w/ a Repast 'SharedContext' obj 
// ^ doesn't inherit from an abstract superclass; can make Repast models w/o extending base classes
// sharedContext objs are containers aware that some agents they contain may be agents being managed by other processes
	int stopAt, countOfAgents;; // instance vars; time steps to run for; number of agents
	repast::Properties* props; 
	repast::SharedContext<RepastHPCDemoAgent> context; // sharedContext's a templated class; you must tell it using <> the class it will contain is 'RepastHPCDemoAgent'
	RepastHPCDemoAgentPackageProvider* provider; RepastHPCDemoAgentPackageReceiver* receiver;
    	repast::RepastEdgeContentManager<RepastHPCDemoAgent> edgeContentManager; 
// ^ Edge content manager responsible for packaging edge content & sharing between processes; its instantiation occurs here, in the class declaration
	repast::SVDataSet* agentValues;
	repast::SharedNetwork<RepastHPCDemoAgent, repast::RepastEdge<RepastHPCDemoAgent>, repast::RepastEdgeContent<RepastHPCDemoAgent>, repast::RepastEdgeContentManager<RepastHPCDemoAgent> >* agentNetwork;	
public:
	RepastHPCDemoModel(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm); ~RepastHPCDemoModel();
	void init(); void initSchedule(repast::ScheduleRunner& runner); void recordResults(); void connectAgentNetwork();
	void requestAgents(); void cancelAgentRequests(); void removeLocalAgents(); void moveAgents(); void doSomething();
};
#endif
