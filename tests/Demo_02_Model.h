/* Demo_02_Model.h */
#ifndef DEMO_02_MODEL
#define DEMO_02_MODEL
#include <boost/mpi.hpp> // includes all Boost.MPI library headers
#include "repast_hpc/Schedule.h" // Simulation schedule queue w/ repast events
#include "repast_hpc/Properties.h" // Map type object containing key, value(string) properties
#include "repast_hpc/SharedContext.h" // provides context (local and foreign to a process agent collection, each w/ an ID); specalized for parallelized simulations
#include "repast_hpc/AgentRequest.h" // Encapsulates a request made by one process for agents in another; includes a requests list and previous requests cancellations list
#include "repast_hpc/TDataSource.h" // Interface for classes that act as datasoures for DataSets
#include "repast_hpc/SVDataSet.h" // Data collector
#include "repast_hpc/SharedNetwork.h"// Network implementation that can be shared across processes
#include "Demo_02_Agent.h"
//int randNum; 
int randomize();
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
class DataSource_AgentState : public repast::TDataSource<int>{
private:
	repast::SharedContext<RepastHPCDemoAgent>* context;
public:
	DataSource_AgentState(repast::SharedContext<RepastHPCDemoAgent>* state);
	int getData();
};
class DataSource_AgentNewState : public repast::TDataSource<int>{
private:
	repast::SharedContext<RepastHPCDemoAgent>* context;	
public:
	DataSource_AgentNewState(repast::SharedContext<RepastHPCDemoAgent>* newState);
	int getData();
};
/* Print Agent Information */
void printAgentInfo(int whichRank,const char* text,repast::SharedContext<RepastHPCDemoAgent>* context);
/* World */
class RepastHPCDemoModel{
	int stopAt, countOfAgents; repast::Properties* props; // Ptr to a properties file & its variables
	repast::SharedContext<RepastHPCDemoAgent> context; repast::SVDataSet* agentValues;
	RepastHPCDemoAgentPackageProvider* provider; RepastHPCDemoAgentPackageReceiver* receiver;
	repast::RepastEdgeContentManager<RepastHPCDemoAgent> edgeContentManager;
	repast::SharedNetwork<RepastHPCDemoAgent, repast::RepastEdge<RepastHPCDemoAgent>, repast::RepastEdgeContent<RepastHPCDemoAgent>, repast::RepastEdgeContentManager<RepastHPCDemoAgent> >* agentNetwork;
public:
RepastHPCDemoModel(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm); ~RepastHPCDemoModel();
	void init(); void initSchedule(repast::ScheduleRunner& runner); void recordResults(); void connectAgentNetwork();
	void requestAgents(); void cancelAgentRequests(); void removeLocalAgents(); void moveAgents(); void doSomething();
};
#endif
