/* agentModelTest.cpp */
#include <stdio.h>
#include <vector>
#include <boost/mpi.hpp>
#include "repast_hpc/AgentId.h"
#include "repast_hpc/RepastProcess.h"
#include "repast_hpc/Utilities.h" // Included for the repast:strToInt function (property as a string to int)
#include "repast_hpc/Properties.h" // Included to use the properties class which allows to use & modify the .props files (e.g. stop.at = 18)
#include "repast_hpc/initialize_random.h" // handles RNG (random number generator/generation)
#include "repast_hpc/SVDataSetBuilder.h"
#include "agentModelTest.h"
BOOST_CLASS_EXPORT_GUID(repast::SpecializedProjectionInfoPacket<repast::RepastEdgeContent<RepastHPCDemoAgent> >, "SpecializedProjectionInfoPacket_EDGE");
RepastHPCDemoAgentPackageProvider::RepastHPCDemoAgentPackageProvider(repast::SharedContext<RepastHPCDemoAgent>* agentPtr): agents(agentPtr){}
/* Pkg provider */
// providePkg takes a ptr to an agent & makes a pkg out of it;
void RepastHPCDemoAgentPackageProvider::providePackage(RepastHPCDemoAgent * agent, std::vector<RepastHPCDemoAgentPackage>& out){
    repast::AgentId id = agent->getId();
    RepastHPCDemoAgentPackage package(id.id(), id.startingRank(), id.agentType(), id.currentRank(), agent->getC(), agent->getTotal());
    out.push_back(package);
}
void RepastHPCDemoAgentPackageProvider::provideContent(repast::AgentRequest req, std::vector<RepastHPCDemoAgentPackage>& out){
    std::vector<repast::AgentId> ids = req.requestedAgents();
    for(size_t i = 0; i < ids.size(); i++){
        providePackage(agents->getAgent(ids[i]), out); // generate & pkg agent content to be sent
    }
}
RepastHPCDemoAgentPackageReceiver::RepastHPCDemoAgentPackageReceiver(repast::SharedContext<RepastHPCDemoAgent>* agentPtr): agents(agentPtr){}
// provideContent takes an agent list from the 'agent requests' obj, gets ptrs to them & uses providePkg to generate & pkg the content to be sent
/* Pkg receiver */
RepastHPCDemoAgent * RepastHPCDemoAgentPackageReceiver::createAgent(RepastHPCDemoAgentPackage package){
// ^ createAgent creates a new AgentID instance for the agent, matching its original ID, except for 'currentRank', which will be reset automatically to the current rank
    repast::AgentId id(package.id, package.rank, package.type, package.currentRank);
    return new RepastHPCDemoAgent(id, package.c, package.total);
}
/* Update Agent */
void RepastHPCDemoAgentPackageReceiver::updateAgent(RepastHPCDemoAgentPackage package){ // < If the specified agent's not found, program will crash w/ a null ptr exception 
    repast::AgentId id(package.id, package.rank, package.type);
    RepastHPCDemoAgent * agent = agents->getAgent(id);
    agent->set(package.currentRank, package.c, package.total);
}
DataSource_AgentTotals::DataSource_AgentTotals(repast::SharedContext<RepastHPCDemoAgent>* c) : context(c){ }
/* Get Data (agentTotals) */
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
// Both loop through all agents, getting a value from each & adding it to the total sum, which is returned; 
// this is run on all processes & a collective operation is performed to sum the results from all processes globally
/* Get Data (agentCTotals) */
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
/* World Class */
RepastHPCDemoModel::RepastHPCDemoModel(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm): context(comm){
// ^ Model class takes a 'Properties' file name as an arg to its constructor & a ptr to a 'world' communicator, used in the 'Properties' constructor
// 'Properties' class isntance (called 'props')'s moved from a local instance in the constructor to a persistent instance created on the heap w/ ... 
// ... the 'new' keyword; then a 'Properties' class instance's created that will contain all specified values in this file
	props = new repast::Properties(propsFile, argc, argv, comm);
	stopAt = repast::strToInt(props->getProperty("stop.at")); // calls to props' functions made via reference (ptr,->) instead of dot operator (.)
	countOfAgents = repast::strToInt(props->getProperty("count.of.agents")); // both vars read from props file/superseeded by the command line & set in the constructor; selects all agents (see selectAgents) 
	initializeRandom(*props, comm);
	if(repast::RepastProcess::instance()->rank() == 0) props->writeToSVFile("./output/record.csv");
// ^ keeps both the initial properties write to a 'record.csv' file, saves a params record used (provided by the properties obj) for every sim run
	provider = new RepastHPCDemoAgentPackageProvider(&context);
	receiver = new RepastHPCDemoAgentPackageReceiver(&context);
  agentNetwork = new repast::SharedNetwork<RepastHPCDemoAgent, repast::RepastEdge<RepastHPCDemoAgent>, repast::RepastEdgeContent<RepastHPCDemoAgent>, repast::RepastEdgeContentManager<RepastHPCDemoAgent> >("agentNetwork", false, &edgeContentManager);
	context.addProjection(agentNetwork);
	// Data collection
	// Create the data set builder
	std::string fileOutputName("./output/agent_total_data.csv");
	repast::SVDataSetBuilder builder(fileOutputName.c_str(), ",", repast::RepastProcess::instance()->getScheduleRunner().schedule());
	// Create the individual data sets to be added to the builder
	DataSource_AgentTotals* agentTotals_DataSource = new DataSource_AgentTotals(&context);
	builder.addDataSource(createSVDataSource("Total", agentTotals_DataSource, std::plus<int>()));
	DataSource_AgentCTotals* agentCTotals_DataSource = new DataSource_AgentCTotals(&context);
	builder.addDataSource(createSVDataSource("C", agentCTotals_DataSource, std::plus<int>()));
	// Use the builder to create the data set
	agentValues = builder.createDataSet();
}
/* World Destructor */
RepastHPCDemoModel::~RepastHPCDemoModel(){ 
// ^ model class has a ptr to provider & receiver objs; just like the 'properties' obj, the actual instance's created using the 'new' keyword & has to be destroyed in ...
// ... destructor w/ 'delete'
	delete props; delete provider; delete receiver; delete agentValues;
}
/* Init */
void RepastHPCDemoModel::init(){
	int rank = repast::RepastProcess::instance()->rank();
	for(int i = 0; i < countOfAgents; i++){ //agent creation: create agent ID w/ appropriate rank & type, here all agents are of type 0
		repast::AgentId id(i, rank, 0);
		id.currentRank(rank);
		RepastHPCDemoAgent* agent = new RepastHPCDemoAgent(id); //agent instance created on heap; ptr 'agent' pointing to a 'RepastHPCDemoAgent' variable
		context.addAgent(agent); // agents added to the context - ptr 'agent', pointing to a RepastHPCDemoAgent var's then passed to the addAgent method
	}
}
/* requestAgents */
void RepastHPCDemoModel::requestAgents(){ 
	int rank = repast::RepastProcess::instance()->rank(); // retrieve process rank
	int worldSize = repast::RepastProcess::instance()->worldSize(); // retrieve world size
	repast::AgentRequest req(rank);
	for(int i = 0; i < worldSize; i++){ // Loop through all processes by rank number; For each process ...
		if(i != rank){ // ... except this one (excludes current rank - no need to borrow agents from self)
			std::vector<RepastHPCDemoAgent*> agents; // create a vector of agent ptrs       
			context.selectAgents(5, agents); // Chooses 5 local agents randomly using the 'selectAgents' method of the context  & returns ptrs to them
			for(size_t j = 0; j < agents.size(); j++){ // Loop through these 5 agent ptrs
				repast::AgentId local = agents[j]->getId(); // new IDs are added to the agent request, the loops continue ubntil 6 agents are beign requested from each of the other processes
// ^ Transform each local agent's id into a matching non-local one - copied on/ borrowed by other ranks; take local agents' ID vals & transform to match an ID of an ... 
// ... agent on another process
// Each agent pointed to has an 'agentID': (id = rankCreatedOn, 'type', rankCurrentlyFoundOn, currentRank) - the first 3 vals should combine into a globally unique agent ID
// 'rank' == process
				repast::AgentId other(local.id(), i, 0);
				other.currentRank(i);
				req.addRequest(other); // Add it to the agent request
			}
		}
	}
    repast::RepastProcess::instance()->requestAgents<RepastHPCDemoAgent, RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, req, *provider, *receiver, *receiver);
// ^ calls requestAgents templated func (member of 'RepastProcess' instance); agent request's transmitted to the RepastProcess instance, which engages in communication ...
// ... with the other processes & informs them of its requests; it also receives requests coming from other processes; it then sends out agent info that was requested ...
// ... of it & receives info in reply to its requests
// ^ types used for the classes play the roles of : 'agent', 'package', 'provider', 'receiver'
// ^ actual arguments passed are :		  'context', 'request', 'provider', 'receiver'
}
/* connectAgentNetwork */
void RepastHPCDemoModel::connectAgentNetwork(){
	repast::SharedContext<RepastHPCDemoAgent>::const_local_iterator iter    = context.localBegin();
	repast::SharedContext<RepastHPCDemoAgent>::const_local_iterator iterEnd = context.localEnd();
	while(iter != iterEnd) {
		RepastHPCDemoAgent* ego = &**iter;
		std::vector<RepastHPCDemoAgent*> agents;
		agents.push_back(ego);                          // Omit self
		context.selectAgents(5, agents, true);          // Choose 5 other agents randomly
		// Make an undirected connection
		for(size_t i = 0; i < agents.size(); i++){
            if(ego->getId().id() < agents[i]->getId().id()){
              std::cout << "CONNECTING: " << ego->getId() << " to " << agents[i]->getId() << std::endl; // prints the agent IDs & other text
  	  	      agentNetwork->addEdge(ego, agents[i]); 
// ^ can set edge weight when the edge's connected by adding the weight as an additional argument; could be = i, too, to get a weight from to 1 to N
            }
		}
		iter++;
	}
}
/* cancelAgentRequests */
void RepastHPCDemoModel::cancelAgentRequests(){
	int rank = repast::RepastProcess::instance()->rank();
	if(rank == 0) std::cout << "CANCELING AGENT REQUESTS" << std::endl;
	repast::AgentRequest req(rank);	
	repast::SharedContext<RepastHPCDemoAgent>::const_state_aware_iterator non_local_agents_iter  = context.begin(repast::SharedContext<RepastHPCDemoAgent>::NON_LOCAL);
	repast::SharedContext<RepastHPCDemoAgent>::const_state_aware_iterator non_local_agents_end   = context.end(repast::SharedContext<RepastHPCDemoAgent>::NON_LOCAL);
	while(non_local_agents_iter != non_local_agents_end){
		req.addCancellation((*non_local_agents_iter)->getId());
		non_local_agents_iter++;
	}
    repast::RepastProcess::instance()->requestAgents<RepastHPCDemoAgent, RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, req, *provider, *receiver, *receiver); // Remove Agents after their requests are cancelled (remove imported agents)	 
	std::vector<repast::AgentId> cancellations = req.cancellations();
	std::vector<repast::AgentId>::iterator idToRemove = cancellations.begin(); //AgentRequest is itself used as the agents list to be removed from the context 
	while(idToRemove != cancellations.end()){ // while-looping through the agent list using the iterator because it will become invalid when the an agent's removed
		context.importedAgentRemoved(*idToRemove);
		idToRemove++;
	}
}
/* removeLocalAgents */
void RepastHPCDemoModel::removeLocalAgents(){
	int rank = repast::RepastProcess::instance()->rank();
	if(rank == 0) std::cout << "REMOVING LOCAL AGENTS" << std::endl;
	for(int i = 0; i < 5; i++){
		repast::AgentId id(i, rank, 0);
		repast::RepastProcess::instance()->agentRemoved(id);
		context.removeAgent(id);
	}
  repast::RepastProcess::instance()->synchronizeAgentStatus<RepastHPCDemoAgent, RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, *provider, *receiver, *receiver);
// ^ when the synchronizeAgentStatus call's made, RepastProcess informs the other processes the agents they've borrowed are gone & the agents are auto-removed from other processes
}
/* moveAgents */
void RepastHPCDemoModel::moveAgents(){
	int rank = repast::RepastProcess::instance()->rank();
	if(rank == 0){
		repast::AgentId agent0(0, 0, 0); repast::AgentId agent1(1, 0, 0); repast::AgentId agent2(2, 0, 0);
		repast::AgentId agent3(3, 0, 0); repast::AgentId agent4(4, 0, 0);	
		repast::RepastProcess::instance()->moveAgent(agent0, 1); repast::RepastProcess::instance()->moveAgent(agent1, 2);
		repast::RepastProcess::instance()->moveAgent(agent2, 3);
		repast::RepastProcess::instance()->moveAgent(agent3, 3); repast::RepastProcess::instance()->moveAgent(agent4, 1);
	}
  repast::RepastProcess::instance()->synchronizeAgentStatus<RepastHPCDemoAgent, RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(context, *provider, *receiver, *receiver); // notify other contexts agents no longer exist
}
/* doSomething */
void RepastHPCDemoModel::doSomething(){ // provides scheduling ability; output shows current tick, which it retrieves via the RepastProcess instance
	int whichRank = 0; // want to look at the rank 0 output
	if(repast::RepastProcess::instance()->rank() == whichRank) std::cout << " TICK " << repast::RepastProcess::instance()->getScheduleRunner().currentTick() << std::endl; // function now shows the current tick, which it retrieves via the RepastProcess instance
	if(repast::RepastProcess::instance()->rank() == whichRank){
		std::cout << "LOCAL AGENTS:" << std::endl;
		for(int r = 0; r < 4; r++){
			for(int i = 0; i < 10; i++){
				repast::AgentId toDisplay(i, r, 0);
				RepastHPCDemoAgent* agent = context.getAgent(toDisplay);
				if((agent != 0) && (agent->getId().currentRank() == whichRank)) std::cout << agent->getId() << " " << agent->getC() << " " << agent->getTotal() << std::endl;
			}
		}
		std::cout << "NON LOCAL AGENTS:" << std::endl;
		for(int r = 0; r < 4; r++){
			for(int i = 0; i < 10; i++){
				repast::AgentId toDisplay(i, r, 0);
				RepastHPCDemoAgent* agent = context.getAgent(toDisplay);
				if((agent != 0) && (agent->getId().currentRank() != whichRank)) std::cout << agent->getId() << " " << agent->getC() << " " << agent->getTotal() << std::endl;
			}
		}
	}
	std::vector<RepastHPCDemoAgent*> agents;
	context.selectAgents(repast::SharedContext<RepastHPCDemoAgent>::LOCAL, countOfAgents, agents); // select local agents only (NON-LOCAL || LOCAL); here all agents selected (countOfAgents = total)
// ^ can be used to select any subset from the context & return those agents in arbitrary or mathemathically randomized order; order returned's random because the agent ptrs are put into a vector...
// ... which is an ordered list  
// In each time step, agent list's shuffled, each agent's asked to play the cooperation game against 3 other agents randomly selected from the population
	std::vector<RepastHPCDemoAgent*>::iterator it = agents.begin();
	while(it != agents.end()){ // have only the local agents act, ensure play order randomized each turn 
		(*it)->play(agentNetwork);
		it++;
    }
	repast::RepastProcess::instance()->synchronizeAgentStates<RepastHPCDemoAgentPackage, RepastHPCDemoAgentPackageProvider, RepastHPCDemoAgentPackageReceiver>(*provider, *receiver);
// ^ Ensure agent copies on each process are synchronized w/ the 'real' agents that are on the agents' home processes; sync agents after all local agents have played the game
}
/* Initialize Schedule */
void RepastHPCDemoModel::initSchedule(repast::ScheduleRunner& runner){
// ^ runner class used to schedule sim events; By placing this ptr on the event schedule, Repast's instructed to call that method of the model instance
// Class pointing to the "doSomething" method of the RepastHPCDemoMOdel obj instance
	runner.scheduleEvent(1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::requestAgents)));
    runner.scheduleEvent(1.1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::connectAgentNetwork))); // first (at the first time step) request agents, then connect them in a network
	runner.scheduleEvent(2, 1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::doSomething)));
// ^ All processes begin at tick 2 by doing "doSomething" and repeat every 1 ticks; Tutorial had nested events in 'if-else' statements with different... 
// ...initial ticks and steps; repetition arg can be omitted ( = no event repetition)
	runner.scheduleEvent(3, repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::moveAgents)));
	runner.scheduleEndEvent(repast::Schedule::FunctorPtr(new repast::MethodFunctor<RepastHPCDemoModel> (this, &RepastHPCDemoModel::recordResults)));
// ^ 'recordResults' function scheduled to be called as an end event, run after the sim comes at the stop event.
	runner.scheduleStop(stopAt);
// stops sim at timestep specified; can explicitly place the stop command at a step where it will not coincide with any repeating event, so it's made ...
// ... clear that all events scheduled to occur during tick 'N' will take place (e.g. saying 10.9 so 'stop' won't conincide w/ repeating events)
	// Data collection
	runner.scheduleEvent(1.5, 5, repast::Schedule::FunctorPtr(new repast::MethodFunctor<repast::DataSet>(agentValues, &repast::DataSet::record)));
// ^ (beginAtSpecificTick/Timestep, repeatEveryNTicks; starting tick can be a float (e.g. 4.2), tick repetition has to be a an int + startingTick ...
// ... (e.g. All processes start at tick 2.2 by doing "doSomething", then repeat every 1 tick: repeat on 3.2, 4.2, etc.)
	runner.scheduleEvent(10.6, 10, repast::Schedule::FunctorPtr(new repast::MethodFunctor<repast::DataSet>(agentValues, &repast::DataSet::write)));
	runner.scheduleEndEvent(repast::Schedule::FunctorPtr(new repast::MethodFunctor<repast::DataSet>(agentValues, &repast::DataSet::write)));
}
/* Record Results to 2 .csv files */
void RepastHPCDemoModel::recordResults(){ 
	if(repast::RepastProcess::instance()->rank() == 0){
		props->putProperty("Result","Passed");
// ^ adds the new 'Result' entry w/ a 'Passed' value into the properties file; in practice, the value could be derived from the sim results
		std::vector<std::string> keyOrder;
// ^ vector containing a properties list to be output; allows to specify which properties & their order are written to file (here all) 
		keyOrder.push_back("RunNumber"); keyOrder.push_back("stop.at"); keyOrder.push_back("Result");
		props->writeToSVFile("./output/results.csv", keyOrder); // record final output; 'RunNumber' & 'stop.at' are in both .csv files
    }
}
