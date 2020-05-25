#include "../common/MGDFComponents.hpp"

#ifdef USE_JSONCPP_STORAGE
#include "jsoncpp/JsonCppStorageFactoryComponent.hpp"
#endif

namespace MGDF {
namespace core {
namespace storage {

// provide a generic name for the rest of the  so that
// there is no explicit instansiation of a particular xml
// implementation
#ifdef USE_JSONCPP_STORAGE
#define CreateStorageFactoryComponentImpl \
  jsoncppImpl::CreateJsonCppStorageFactoryComponent
#endif

}  // namespace storage
}  // namespace core
}  // namespace MGDF