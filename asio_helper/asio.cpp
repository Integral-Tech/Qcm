#include "core/core.h"

#ifdef __linux__
#    define ASIO_DECL C_DECL_EXPORT
#endif

#undef ASIO_DISABLE_VISIBILITY

#include <asio/impl/src.hpp>

#include "core/log.h"
#include "asio_helper/detached_log.h"

namespace helper
{

asio_detached_log_t asio_detached_log {};

void asio_detached_log_t::operator()(std::exception_ptr ptr, const std::source_location loc) {
    if (! ptr) return;
    try {
        std::rethrow_exception(ptr);
    } catch (const std::exception& e) {
        qcm::LogManager::instance()->log(qcm::LogLevel::ERROR, loc, "{}", e.what());
    }
}
} // namespace helper