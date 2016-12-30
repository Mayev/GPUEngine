#ifndef GE_INIT_AND_FINALIZE_H
#define GE_INIT_AND_FINALIZE_H

#include <geCore/Export.h>

namespace ge
{
   namespace core
   {

      class GECORE_EXPORT InitAndFinalize {
      public:
         InitAndFinalize(void (*initFce)(),void (*finalizeFce)());
         ~InitAndFinalize();
      };

   }
}

#endif /* GE_CORE_INIT_AND_FINALIZE_H */
