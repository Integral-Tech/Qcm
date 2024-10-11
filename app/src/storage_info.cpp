#include "Qcm/storage_info.h"
#include "qcm_interface/async.inl"
#include "qcm_interface/global.h"
#include "Qcm/app.h"
#include "Qcm/cache_sql.h"

#include <asio/strand.hpp>

namespace qcm::qml
{

StorageInfo::StorageInfo(QObject* parent): QObject(parent) {}
auto StorageInfo::total() const -> double { return m_total; }
void StorageInfo::setTotal(double v) {
    if (ycore::cmp_exchange(m_total, v)) {
        totalChanged();
    }
}
StorageInfoQuerier::StorageInfoQuerier(QObject* parent): QAsyncResult(parent) {
    set_data(new StorageInfo(this));
}
void StorageInfoQuerier::reload() {
    set_status(Status::Querying);
    auto ex              = asio::make_strand(Global::instance()->pool_executor());
    auto media_cache_sql = App::instance()->media_cache_sql();
    auto cache_sql       = App::instance()->cache_sql();
    this->spawn(ex, [media_cache_sql, cache_sql, this]() -> asio::awaitable<void> {
        auto media_size  = co_await media_cache_sql->total_size();
        auto normal_size = co_await cache_sql->total_size();

        co_await asio::post(
            asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));

        auto d = static_cast<StorageInfo*>(data());
        d->setTotal(media_size + normal_size);
        set_status(Status::Finished);
    });
}
} // namespace qcm::qml