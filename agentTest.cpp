/* agentTest.cpp */
#include "agentTest.h"
RepastHPCDemoAgent::RepastHPCDemoAgent(repast::AgentId id): id_(id), c(100), total(200){ } // constructors will use default c, total values if none are provided
RepastHPCDemoAgent::RepastHPCDemoAgent(repast::AgentId id, double newC, double newTotal): id_(id), c(newC), total(newTotal){ }
RepastHPCDemoAgent::~RepastHPCDemoAgent(){ }
void RepastHPCDemoAgent::set(int currentRank, double newC, double newTotal){
    id_.currentRank(currentRank);
    c     = newC; total = newTotal;
}
bool RepastHPCDemoAgent::cooperate(){ // uses an instance of Repast's 'random' class; it will be integrated with the RNG routines & hence fully reproducible
	return repast::Random::instance()->nextDouble() < c/total;
}
void RepastHPCDemoAgent::play(repast::SharedNetwork<RepastHPCDemoAgent,
                              repast::RepastEdge<RepastHPCDemoAgent>,
                              repast::RepastEdgeContent<RepastHPCDemoAgent>,
                              repast::RepastEdgeContentManager<RepastHPCDemoAgent> > *network){
    std::vector<RepastHPCDemoAgent*> agentsToPlay; 
    network->successors(this, agentsToPlay); // finds leadings (opposite of 'predecessors') connected by a directed vector from 'self' to 'other'; for an undirected net, both return the same connected ...
// ... nodes
    double cPayoff     = 0; double totalPayoff = 0;
    std::vector<RepastHPCDemoAgent*>::iterator agentToPlay = agentsToPlay.begin();
    while(agentToPlay != agentsToPlay.end()){
        bool iCooperated = cooperate(); // Do I cooperate?
        double payoff = (iCooperated ?
						 ((*agentToPlay)->cooperate() ?  7 : 1) : // If I cooperated, did my opponent?
						 ((*agentToPlay)->cooperate() ? 10 : 3)); // If I didn't cooperate, did my opponent?
        if(iCooperated) cPayoff += payoff;
        totalPayoff             += payoff;
		
        agentToPlay++;
    }
    c      += cPayoff; total  += totalPayoff;	
}
void RepastHPCDemoAgent::threshold() {
    	if(thresh < 7 || thresh = 7)       {state = 3} // agent chooses to use (drive/ride in) a car
    	else if(thresh = 8 || thresh < 11) {state = 2} // agent chooses to cycle
    	else                               {state = 1} // agent chooses to walk
  }
/* Serializable Agent Package Data */
RepastHPCDemoAgentPackage::RepastHPCDemoAgentPackage(){ }
RepastHPCDemoAgentPackage::RepastHPCDemoAgentPackage(int _id, int _rank, int _type, int _currentRank, double _c, double _total):
id(_id), rank(_rank), type(_type), currentRank(_currentRank), c(_c), total(_total){ }
