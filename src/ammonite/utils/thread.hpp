#ifndef INTERNALTHREAD
#define INTERNALTHREAD

#include "../visibility.hpp"

//Include public interface
#include "../../include/ammonite/utils/thread.hpp" // IWYU pragma: export

namespace AMMONITE_INTERNAL ammonite {
  namespace utils {
    namespace thread {
      namespace internal {
        unsigned int getHardwareThreadCount();
        unsigned int getThreadPoolSize();

        bool createThreadPool(unsigned int threadCount);
        void destroyThreadPool();

        void submitWork(AmmoniteWork work, void* userPtr, AmmoniteGroup* group);
        void submitMultiple(AmmoniteWork work, void* userBuffer, int stride,
                            AmmoniteGroup* group, unsigned int newJobs,
                            AmmoniteGroup* submitGroup);
        void submitMultipleSync(AmmoniteWork work, void* userBuffer, int stride,
                                AmmoniteGroup* group, unsigned int newJobs);

        void waitGroupComplete(AmmoniteGroup* group, unsigned int jobCount);
        bool isSingleWorkComplete(AmmoniteGroup* group);
        unsigned int getRemainingWork(AmmoniteGroup* group, unsigned int jobCount);

        void blockThreads();
        void unblockThreads();
        void finishWork();
      }
    }
  }
}

#endif

