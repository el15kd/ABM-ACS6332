/* Demo_02_Agent.cpp */
#include "Demo_02_Agent.h"
RepastHPCDemoAgent::RepastHPCDemoAgent(repast::AgentId id, int state): id_(id), state_(state){ } 
RepastHPCDemoAgent::~RepastHPCDemoAgent(){ }
void RepastHPCDemoAgent::set(int currentRank,int state){
	id_.currentRank(currentRank); state_=state;
}
//bool RepastHPCDemoAgent::cooperate(){return repast::Random::instance()->nextDouble() < c/total;}

int RepastHPCDemoAgent::getState(){return state_;}

void RepastHPCDemoAgent::play(repast::SharedContext<RepastHPCDemoAgent>* context){ //repast::SharedContext<RepastHPCDemoAgent>* context, std::vector<RepastHPCDemoAgent*> *agents
    thresh = 0, thresh2 = 0, threshSum = 0; // initialize the thresh sum variables to 0
    std::vector<RepastHPCDemoAgent*> agentsToPlay; // used to be std::set; 
    //context.successors(this,agentsToPlay);
    //agentsToPlay.insert(this); // Prohibit playing against self	
    //context->selectAgents(3, agentsToPlay, true); // (default false) if true, remove any elements originally in the set before the set is returned
    std::vector<RepastHPCDemoAgent*>::iterator agentToPlay = agentsToPlay.begin(); //std::advance(agentToPlay,1); // used to be std::set; start from the 2nd vector element
    while(agentToPlay != agentsToPlay.end()){ // agentToPlay equivalent to '*it'? (in the model)
        thresh = getState(); // basically thresh = state (for current agent)
        threshSum = (thresh+thresh2); thresh2 = thresh; thresh = threshSum; // Calculate the sum of the following 2 agents
        //std::cout << agentsToPlay.getId() << " Current threshSum is " << threshSum << std::endl; // TODO "Current threshSum for iter # ... is ..."
// For 5 iterations, it should work like this: Iter #; Agent(i); Agent(i+1); statesSum;
//                                                1        2         1           3   
//                                                2        2         1           6
//                                                3        1         3           10
//                                                4        2         2           14
//                                                5        3         3           20
        agentToPlay++; // go to the next agent
    } // minSum = 5, maxSum = 20
    if(threshSum <= 7)                       {state_ = 3;} // agent chooses to use (drive/ride in) a car
    else if(threshSum = 8 || threshSum < 11) {state_ = 2;} // agent chooses to cycle
    else                                     {state_ = 1;} // agent chooses to walk
}
/* Serializable Agent Package Data */
RepastHPCDemoAgentPackage::RepastHPCDemoAgentPackage(){ }
RepastHPCDemoAgentPackage::RepastHPCDemoAgentPackage(int _id,int _rank,int _type,int _currentRank,int _state):
id(_id),rank(_rank),type(_type),currentRank(_currentRank),state(_state){ }
/*
const char* treshold(int thresh) {
// 
    if(thresh <= 7) 			{state = 3; return "using a car or public transport";} // agent chooses to use (drive/ride in) a motorized vehicle
    else if (thresh = 8 || thresh < 11)	{state = 2; return "cycling";} // agent chooses to cycle
    else 				{state = 1; return "on-foot";} // agent chooses to walk
} // ^ lower state corresponds to transportation mode creating the least amount of traffic (1 - least, 3 - most); thresh range e [4 (4*1), 12 (4*3)] since an agent has 4 neighbours
int main(int argc, char** argv){
*/
