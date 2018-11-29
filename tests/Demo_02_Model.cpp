/* Demo_02_Model.cpp */
#include <stdio.h>
#include <vector>
#include <stdlib.h> // srand, rand; could also use Repast's Random.h
#include <time.h> // time() function used to initiallize srand w/ a runtime value
#include <boost/mpi.hpp>
#include "repast_hpc/AgentId.h"
#include "repast_hpc/RepastProcess.h"
#include "repast_hpc/Utilities.h"
#include "repast_hpc/Properties.h"
#include "repast_hpc/initialize_random.h"
#include "repast_hpc/SVDataSetBuilder.h"
#include "Demo_02_Model.h"
BOOST_CLASS_EXPORT_GUID(repast::SpecializedProjectionInfoPacket<repast::RepastEdgeContent<RepastHPCDemoAgent> >, "SpecializedProjectionInfoPacket_EDGE");
RepastHPCDemoAgentPackageProvider::RepastHPCDemoAgentPackageProvider(repast::SharedContext<RepastHPCDemoAgent>* agentPtr): agents(agentPtr){ }
/*provide pkg*/
void RepastHPCDemoAgentPackageProvider::providePackage(RepastHPCDemoAgent * agent, std::vector<RepastHPCDemoAgentPackage>& out){
    repast::AgentId id = agent->getId(); RepastHPCDemoAgentPackage package(id.id(),id.startingRank(),id.agentType(),id.currentRank(),agent->getState()); out.push_back(package);
}
/*provide content*/
void RepastHPCDemoAgentPackageProvider::provideContent(repast::AgentRequest req, std::vector<RepastHPCDemoAgentPackage>& out){
    std::vector<repast::AgentId> ids = req.requestedAgents();
    for(size_t i = 0; i < ids.size(); i++){providePackage(agents->getAgent(ids[i]), out);}
}
/*Receive pkg*/
RepastHPCDemoAgentPackageReceiver::RepastHPCDemoAgentPackageReceiver(repast::SharedContext<RepastHPCDemoAgent>* agentPtr): agents(agentPtr){}
RepastHPCDemoAgent * RepastHPCDemoAgentPackageReceiver::createAgent(RepastHPCDemoAgentPackage package){
    repast::AgentId id(package.id, package.rank, package.type, package.currentRank); return new RepastHPCDemoAgent(id, 2);
}
/*Update Agent w/ pkg*/
void RepastHPCDemoAgentPackageReceiver::updateAgent(RepastHPCDemoAgentPackage package){
    repast::AgentId id(package.id, package.rank, package.type); RepastHPCDemoAgent * agent = agents->getAgent(id); agent->set(package.currentRank,package.state);
}
/*Connect Agent Net*/
void RepastHPCDemoModel::connectAgentNetwork(){
	repast::SharedContext<RepastHPCDemoAgent>::const_local_iterator iter    = context.localBegin(); repast::SharedContext<RepastHPCDemoAgent>::const_local_iterator iterEnd = context.localEnd();
	while(iter != iterEnd) {
		RepastHPCDemoAgent* ego = &**iter; std::vector<RepastHPCDemoAgent*> agents;
		agents.push_back(ego); context.selectAgents(5, agents, true); // Omit self & choose 4 other agents randomly
		for(size_t i = 0; i < agents.size(); i++){ // Make an undirected connection
         	    // std::cout << "CONNECTING: " << ego->getId() << " to " << agents[i]->getId() << std::endl; // uncomment to see agents are being connected
  	  	    agentNetwork->addEdge(ego, agents[i]);	
		}
		iter++;
	}	
}
/*create model/world*/
RepastHPCDemoModel::RepastHPCDemoModel(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm): context(comm){
	props = new repast::Properties(propsFile, argc, argv, comm);
	stopAt = repast::strToInt(props->getProperty("stop.at")); countOfAgents = repast::strToInt(props->getProperty("count.of.agents")); // read 'properties' file inputs
	initializeRandom(*props, comm);
	if(repast::RepastProcess::instance()->rank() == 0) props->writeToSVFile("./output/record.csv");
	provider = new RepastHPCDemoAgentPackageProvider(&context); receiver = new RepastHPCDemoAgentPackageReceiver(&context);
	agentNetwork = new repast::SharedNetwork<RepastHPCDemoAgent, repast::RepastEdge<RepastHPCDemoAgent>, repast::RepastEdgeContent<RepastHPCDemoAgent>, 
		repast::RepastEdgeContentManager<RepastHPCDemoAgent> >("agentNetwork", false, &edgeContentManager); // agentNetwork will go into 'context' which will go into 'play'
	context.addProjection(agentNetwork);
	// Data collection
	std::string fileOutputName("./output/agent_total_data.csv"); // Create the data set builder
	repast::SVDataSetBuilder builder(fileOutputName.c_str(), ",", repast::RepastProcess::instance()->getScheduleRunner().schedule());
	// Create the individual data sets to be added to the builder
	// Use the builder to create the data set
	agentValues = builder.createDataSet();
}
/*kill model*/
RepastHPCDemoModel::~RepastHPCDemoModel(){delete props; delete provider; delete receiver; delete agentValues;}
/*Randomize state*/
int randomize() {srand(time(NULL)); int randNum = rand() % 3 + 1; return randNum;} // initialize random seed & generate random number between 1 & 3
/*init model*/
void RepastHPCDemoModel::init(){
	int rank = repast::RepastProcess::instance()->rank(); // current rank
	//RepastHPCDemoAgent* agent = context.getAgent(toDisplay)
	//agent->getState();
	for(int i = 0; i < countOfAgents; i++){
		repast::AgentId id(i, rank, 0); id.currentRank(rank); int tempNum = randomize();
		RepastHPCDemoAgent* agent = new RepastHPCDemoAgent(id,tempNum); // Create Agent w/ RepastHPCDemoAgent::RepastHPCDemoAgent(repast::AgentId id);
		int state = agent->getState(); // set the state w/ RepastHPCDemoAgent::set
		agent->set(rank,state); // randomly sets the states for all agents in currentRank
		context.addAgent(agent);
	}
}
/*requestAgents*/
void RepastHPCDemoModel::requestAgents(){
	int rank = repast::RepastProcess::instance()->rank();
	int worldSize= repast::RepastProcess::instance()->worldSize();
	repast::AgentRequest req(rank);
	for(int i = 0; i < worldSize; i++){                     // For each process
		if(i != rank){                                      // ... except this one
			std::vector<RepastHPCDemoAgent*> agents;        
			context.selectAgents(4, agents);                // Choose 4 local agents randomly
			for(size_t j = 0; j < agents.size(); j++){
				repast::AgentId local = agents[j]->getId();          // Transform each local agent's id into a matching non-local one
				repast::AgentId other(local.id(), i, 0); other.currentRank(i); req.addRequest(other); // Add it to the agent request
			}
		}
	}
    repast::RepastProcess::instance()->requestAgents<RepastHPCDemoAgent, RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, req, *provider, *receiver, *receiver);
}
/*cancelAgentRequests*/
void RepastHPCDemoModel::cancelAgentRequests(){
	int rank = repast::RepastProcess::instance()->rank();
	if(rank == 0) std::cout << "CANCELING AGENT REQUESTS" << std::endl;
	repast::AgentRequest req(rank);	
	repast::SharedContext<RepastHPCDemoAgent>::const_state_aware_iterator non_local_agents_iter  = context.begin(repast::SharedContext<RepastHPCDemoAgent>::NON_LOCAL);
	repast::SharedContext<RepastHPCDemoAgent>::const_state_aware_iterator non_local_agents_end   = context.end(repast::SharedContext<RepastHPCDemoAgent>::NON_LOCAL);
	while(non_local_agents_iter != non_local_agents_end){
		req.addCancellation((*non_local_agents_iter)->getId());non_local_agents_iter++;}
    repast::RepastProcess::instance()->requestAgents<RepastHPCDemoAgent, RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, req, *provider, *receiver, *receiver);
	std::vector<repast::AgentId> cancellations = req.cancellations();
	std::vector<repast::AgentId>::iterator idToRemove = cancellations.begin();
	while(idToRemove != cancellations.end()){context.importedAgentRemoved(*idToRemove);idToRemove++;}
}
/*remove & synchronize local agents*/
void RepastHPCDemoModel::removeLocalAgents(){
	int rank = repast::RepastProcess::instance()->rank();
	if(rank == 0) std::cout << "REMOVING LOCAL AGENTS" << std::endl;
	for(int i = 0; i < 4; i++){repast::AgentId id(i, rank, 0); repast::RepastProcess::instance()->agentRemoved(id); context.removeAgent(id);}
    repast::RepastProcess::instance()->synchronizeAgentStatus<RepastHPCDemoAgent, RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, *provider, *receiver, *receiver);
}
/*Move agents*/
void RepastHPCDemoModel::moveAgents(){
	int rank = repast::RepastProcess::instance()->rank();
	if(rank == 0){ // moveAgent() notifies 'this' RepastProcess the specified agent should be moved from 'this' process to a specified process; moveAgents() does this for multiple agents & processes
		repast::AgentId agent0(0, 0, 0); repast::AgentId agent1(1, 0, 0); repast::AgentId agent2(2, 0, 0); repast::AgentId agent3(3, 0, 0);
		repast::AgentId agent4(4, 0, 0); repast::RepastProcess::instance()->moveAgent(agent0, 1); repast::RepastProcess::instance()->moveAgent(agent1, 2);
		repast::RepastProcess::instance()->moveAgent(agent2, 3); repast::RepastProcess::instance()->moveAgent(agent3, 3); repast::RepastProcess::instance()->moveAgent(agent4, 1);
	} 
    repast::RepastProcess::instance()->synchronizeAgentStatus<RepastHPCDemoAgent, RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, *provider, *receiver, *receiver);
}
const char * initialPrint = "initial"; const char * finalPrint = "final"; int whichRank = 0; // vars used in the following function
/* Print Agent Information */
void printAgentInfo(int whichRank,const char* text,repast::SharedContext<RepastHPCDemoAgent>* context){ // on current rank and tick
if(repast::RepastProcess::instance()->rank() == whichRank) std::cout << " TICK " << repast::RepastProcess::instance()->getScheduleRunner().currentTick() << std::endl;
	if(repast::RepastProcess::instance()->rank() == whichRank){
		std::cout << "LOCAL AGENTS:" << std::endl;
		for(int r = 0; r < 4; r++){
			for(int i = 0; i < 10; i++){
				repast::AgentId toDisplay(i, r, 0); RepastHPCDemoAgent* agent = context->getAgent(toDisplay); // gets the id of the agent to display as a ptr
				if((agent != 0) && (agent->getId().currentRank() == whichRank)) {
					int getTheState = agent->getState(); std::cout << agent->getId() << std::endl; //printf(" %s state is %d\n",text,getTheState);
// ^ Two print statement on one line because I couldn't figure out how to; tried unboxing and then casting, didn't work ((int)(repast::AgentId)agent->getId())
					//std::cout << agent->getId() << " %c state is " << text << getTheState << std::endl; 
				}
			}
		}	
		std::cout << "NON LOCAL AGENTS:" << std::endl;
		for(int r = 0; r < 4; r++){
			for(int i = 0; i < 10; i++){
				repast::AgentId toDisplay(i, r, 0); RepastHPCDemoAgent* agent = context->getAgent(toDisplay);
				if((agent != 0) && (agent->getId().currentRank() != whichRank)) {
					int getTheState = agent->getState(); std::cout << agent->getId() << std::endl; //printf(" %s state is %d\n",text,getTheState);
				}
			}
		}
	}
}
/*Where the Actual Thresholding's done; also provides scheduling ability*/
void RepastHPCDemoModel::doSomething(){
	void printAgentInfo(int whichRank,const char* initialPrint,repast::SharedContext<RepastHPCDemoAgent>& context); // also needs context & agents
	std::vector<RepastHPCDemoAgent*> agents; context.selectAgents(repast::SharedContext<RepastHPCDemoAgent>::LOCAL, countOfAgents, agents); 
// ^ select 5 agents from the current rank; gives a ptr vector/set of local agents, drawing the sample from 5 local agents 	
	std::vector<RepastHPCDemoAgent*>::iterator it = agents.begin(); //start iterating through the selected agents who are represented by a vector of ptrs to agents pointed to by it
	std::advance(it, 1); //start from the second agent
	while(it != agents.end()){(*it)->play(&context); it++;} //printf("That was iteration # %d\n",iter); // should be 5 iterations; 
	//void printAgentsStates(int whichRank,const char* finalPrint,repast::SharedContext<RepastHPCDemoAgent>* context);
	repast::RepastProcess::instance()->synchronizeAgentStates<RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(*provider, *receiver);
	//void printAgentsStates(int whichRank,const char* finalPrint,repast::SharedContext<RepastHPCDemoAgent>* context);
}
/*init schedule*/
void RepastHPCDemoModel::initSchedule(repast::ScheduleRunner& runner){
	runner.scheduleEvent(1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::requestAgents)));
	runner.scheduleEvent(1.1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::connectAgentNetwork)));
	runner.scheduleEvent(2, 1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::doSomething)));
	runner.scheduleEvent(3, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::moveAgents)));
	runner.scheduleEndEvent(repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::recordResults)));
	runner.scheduleStop(stopAt);
	/* Data collection - NOT IMPLEMENTED
	runner.scheduleEvent(1.5, 5, repast::Schedule::FunctorPtr(new repast::MethodFunctor<repast::DataSet>(agentValues, &repast::DataSet::record)));
	runner.scheduleEvent(10.6, 10, repast::Schedule::FunctorPtr(new repast::MethodFunctor<repast::DataSet>(agentValues, &repast::DataSet::write)));
	runner.scheduleEndEvent(repast::Schedule::FunctorPtr(new repast::MethodFunctor<repast::DataSet>(agentValues, &repast::DataSet::write)));
	*/
}
/*record results*/
void RepastHPCDemoModel::recordResults(){
	if(repast::RepastProcess::instance()->rank() == 0){
		props->putProperty("Result","Passed");
		std::vector<std::string> keyOrder;
		keyOrder.push_back("RunNumber"); keyOrder.push_back("stop.at"); keyOrder.push_back("Result");
		props->writeToSVFile("./output/results.csv", keyOrder);
    }
}
// ELIMINATED
/* DAQ
//DataSource_AgentTotals* agentTotals_DataSource = new DataSource_AgentTotals(&context);
	//builder.addDataSource(createSVDataSource("Total", agentTotals_DataSource, std::plus<int>()));
	//DataSource_AgentCTotals* agentCTotals_DataSource = new DataSource_AgentCTotals(&context);
	//builder.addDataSource(createSVDataSource("C", agentCTotals_DataSource, std::plus<int>()));
//getCData
DataSource_AgentTotals::DataSource_AgentTotals(repast::SharedContext<RepastHPCDemoAgent>* c) : context(c){ }
int DataSource_AgentTotals::getData(){
	int sum = 0;
	repast::SharedContext<RepastHPCDemoAgent>::const_local_iterator iter    = context->localBegin();
	repast::SharedContext<RepastHPCDemoAgent>::const_local_iterator iterEnd = context->localEnd();
	while( iter != iterEnd) {
		sum+= (*iter)->getTotal();
		iter++;
	}
	return sum;
}
//getCData
DataSource_AgentCTotals::DataSource_AgentCTotals(repast::SharedContext<RepastHPCDemoAgent>* c) : context(c){ }
int DataSource_AgentCTotals::getData(){
	int sum = 0;
	repast::SharedContext<RepastHPCDemoAgent>::const_local_iterator iter    = context->localBegin();
	repast::SharedContext<RepastHPCDemoAgent>::const_local_iterator iterEnd = context->localEnd();
	while( iter != iterEnd) {
		sum+= (*iter)->getC();
		iter++;
	}
	return sum;
}
*/
