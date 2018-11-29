/* agentTest.h */
#ifndef agentTest
#define agentTest
#include "repast_hpc/AgentId.h"
#include "repast_hpc/SharedContext.h" // collection in which the agent will exist during the sim runs
#include "repast_hpc/SharedNetwork.h"
/* Agents */
class RepastHPCDemoAgent{ 
private:
    repast::AgentId   id_; // Repast uses a unique identifier to specify each agent; [agentNumOnProcess, createdOnProcess, type, processOn] < first 3 provide the unique ID
// ^ 5th such agent (4th argument) created on 'that' process (2nd argument), e.g. [4, 12, 3, 5]
    int thresh, state; // threshold & agent state variables: thresh e [1: on-foot, 2: cycling, 3: car use]
    double c, total; // in the demo (Prisonner's dilemma game), even though 2 agents play the game, the result's only recorded for the agent initiating the game ('ego'), other agent - unaffected
public:
    RepastHPCDemoAgent(repast::AgentId id);
    RepastHPCDemoAgent(){} ~RepastHPCDemoAgent();
    RepastHPCDemoAgent(repast::AgentId id, int newState);
    /* Required Agent Getters = DONE */
    virtual repast::AgentId& getId(){return id_;} // All agents must implement the two getters for agentID; generally this is implemented by providing the agent ...
    virtual const repast::AgentId& getId() const {return id_;} // ... with an ID in its constructor, this being recorded as an instance variable (like here)
    /* Getters specific to this kind of Agent = DONE */
    double getC(){return c;} double getTotal(){return total;}
    /* Setter */
    void set(int currentRank, double newC, double newTotal);
    /* Actions */
    //bool cooperate(); // Will indicate whether the agent cooperates or not; probability determined by = c / total
    void play(repast::SharedNetwork<RepastHPCDemoAgent,
              repast::RepastEdge<RepastHPCDemoAgent>,
              repast::RepastEdgeContent<RepastHPCDemoAgent>,
              repast::RepastEdgeContentManager<RepastHPCDemoAgent> > *network);
    void threshold();
};
/* Serializable Agent Package = DONE */
struct RepastHPCDemoAgentPackage {
public:
    int id, rank, type, currentRank; // Agent Identification variables
    /* Constructors */
    RepastHPCDemoAgentPackage(); // For serialization
    RepastHPCDemoAgentPackage(int _id, int _rank, int _type, int _currentRank, double _c, double _total);
    /* For archive packaging */
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & id; ar & rank; ar & type; ar & currentRank; ar & c; ar & total;
    }	
};
#endif
