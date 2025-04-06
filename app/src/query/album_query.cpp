#include "Qcm/query/album_query.hpp"

#include "Qcm/backend.hpp"
#include "Qcm/app.hpp"
#include "Qcm/store.hpp"

#include "Qcm/util/async.inl"

namespace qcm
{

AlbumListModel::AlbumListModel(QObject* parent): base_type(parent) {}
QQmlPropertyMap* AlbumListModel::extra(i32 idx) const {
    if (auto extend = App::instance()->store()->albums.query_extend(this->key_at(idx)); extend) {
        return extend->extra.get();
    }
    return nullptr;
}

AlbumsQuery::AlbumsQuery(QObject* parent): query::QueryList<AlbumListModel>(parent) {
    // set_use_queue(true);
    this->tdata()->set_store(this->tdata(), App::instance()->store()->albums);
}

static const std::set<QStringView> album_json_fields { u"artists" };
static const std::set<QStringView> song_json_fields { u"artists", u"album" };

void AlbumsQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetAlbumsReq {};
    req.setPage(0);
    req.setPageSize((offset() + 1) * limit());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetAlbumsRsp& el) {
            self->tdata()->resetModel(el.items());
            auto store = AppStore::instance();
            for (qsizetype i = 0; i < el.extras().size(); i++) {
                auto id = el.items().at(i).id_proto();
                merge_store_extra(store->albums, id, el.extras().at(i));
            }
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

void AlbumsQuery::fetchMore(qint32) {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetAlbumsReq {};
    req.setPage(offset() + 1);
    req.setPageSize(limit());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto offset = req.page();
        auto rsp    = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self, offset](msg::GetAlbumsRsp& el) {
            auto view = std::views::transform(el.items(), [](auto&& el) {
                return model::Album(el);
            });

            self->tdata()->extend(view);
            auto store = AppStore::instance();

            for (qsizetype i = 0; i < el.extras().size(); i++) {
                auto id = el.items().at(i).id_proto();
                merge_store_extra(store->albums, id, el.extras().at(i));
            }
            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

AlbumSongListModel::AlbumSongListModel(QObject* parent): base_type(parent), m_key(0) {}
AlbumSongListModel::~AlbumSongListModel() {
    auto store = App::instance()->store();
    store->albums.store_remove(m_key);
}
auto AlbumSongListModel::album() const -> album_type {
    if (auto item = App::instance()->store()->albums.store_query(m_key); item) {
        return *item;
    }
    return {};
}
void AlbumSongListModel::setAlbum(const album_type& album) {
    auto store = App::instance()->store();
    auto key   = album.id_proto();

    auto old = m_key;
    if (ycore::cmp_exchange(m_key, key)) {
        store->albums.store_remove(old);
        store->albums.store_insert(album, true, 0);
        albumChanged();
    } else {
        store->albums.store_insert(album, false, 0);
    }
}
auto AlbumSongListModel::extra() const -> QQmlPropertyMap* {
    if (auto extend = App::instance()->store()->albums.query_extend(m_key); extend) {
        return extend->extra.get();
    }
    return nullptr;
}

AlbumQuery::AlbumQuery(QObject* parent): query::QueryList<AlbumSongListModel>(parent) {
    this->tdata()->set_store(this->tdata(), App::instance()->store()->songs);
}
auto AlbumQuery::itemId() const -> model::ItemId { return m_item_id; }
void AlbumQuery::setItemId(model::ItemId in) {
    if (ycore::cmp_exchange(m_item_id, in)) {
        itemIdChanged();
    }
}

void AlbumQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetAlbumReq {};
    req.setId_proto(m_item_id.id());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetAlbumRsp& el) {
            auto store = App::instance()->store();
            self->tdata()->setAlbum(el.item());
            self->tdata()->resetModel(el.songs());

            merge_store_extra(store->albums, el.item().id_proto(), el.extra());

            for (qsizetype i = 0; i < el.songExtras().size(); i++) {
                auto id = el.songs().at(i).id_proto();
                merge_store_extra(store->songs, id, el.songExtras().at(i));
            }
        });
        co_return;
    });
}
} // namespace qcm

#include <Qcm/query/moc_album_query.cpp>