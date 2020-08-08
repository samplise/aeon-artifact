#include "ThreadPool.h"
#include "ThreadStructure.h"
#include "ContextBaseClass.h"

using namespace mace;


void mace::releaseMemory() {
    mace::ContextBaseClass::releaseThreadSpecificMemory(); 
    mace::AgentLock::releaseThreadSpecificMemory();
    ThreadStructure::releaseThreadSpecificMemory();
}
