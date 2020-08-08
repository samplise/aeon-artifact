#include "mace.h"
#include "CommitWrapper.h"
#include "GlobalCommit.h"


std::set<mace::commit_executor*> mace::GlobalCommit::registered;
std::set<mace::CommitWrapper*> mace::GlobalCommit::registered_class;
/*
void GlobalCommit::executeCommit(uint64_t myTicketNum) 
{
    printf("execute commit registered : %u\n", (unsigned)registered_class.size());
    std::set<CommitWrapper*>::iterator i;
    for (i = registered_class.begin(); i != registered_class.end(); i++) {
        (*i)->commitCallBack(myTicketNum);
    }
    std::set<commit_executor*>::iterator regIter = registered.begin();
    while (regIter != registered.end()) {
        commit_executor* fnt = *regIter;
        printf("execute func on ticket %llu\n", myTicketNum);
        (*fnt)(myTicketNum);
        regIter++;
    }
}
*/

#include "mace.h"
#include "Accumulator.h"
