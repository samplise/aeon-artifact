#ifndef _MACE_COMMIT_WRAPPER_H
#define _MACE_COMMIT_WRAPPER_H
#include <set>
#include <unistd.h>
#include <stdint.h>

namespace mace{
  typedef void (commit_executor)(uint64_t); ///< non-class function for commit
  class CommitWrapper {
    public:
      virtual void commitCallBack(uint64_t)=0;
    protected:
      virtual ~CommitWrapper() {};
  };

  template <class Class> class SpecificCommitWrapper : public CommitWrapper
  {
    private:
      void (Class::*fpt)(uint64_t);
      Class* pt2Object;
    public:
      SpecificCommitWrapper(Class* _pt2Object, void(Class::*_fpt)(uint64_t))
      {
        pt2Object = _pt2Object; fpt = _fpt;
      };
      virtual void commitCallBack(uint64_t ticket)
      {
        (*pt2Object.*fpt)(ticket);
      }
  };

}
#endif
