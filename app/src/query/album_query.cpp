#include "Qcm/query/album_query.hpp"

#include "Qcm/backend.h"
#include "Qcm/app.h"

#include "qcm_interface/async.inl"

namespace qcm
{

AlbumListModel::AlbumListModel(QObject* parent): base_type(parent) {}

AlbumsQuery::AlbumsQuery(QObject* parent): query::QueryList<AlbumListModel>(parent) {
    // set_use_queue(true);
}
void AlbumsQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetAlbumsReq {};
    req.setPage(offset());
    req.setPageSize(limit());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetAlbumsRsp& el) {
            auto& al = el.albums();
            log::warn("alsize: {}", al.size());
            self->tdata()->resetModel(al);
        });
        co_return;
    });
}

void AlbumsQuery::fetchMore(qint32) {
}

} // namespace qcm

#include <Qcm/query/moc_album_query.cpp>