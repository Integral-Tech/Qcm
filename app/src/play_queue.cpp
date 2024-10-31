#include "Qcm/play_queue.h"

#include <ranges>
#include <variant>
#include <unordered_set>

#include "core/random.h"
#include "core/log.h"
#include "core/optional_helper.h"
#include "asio_helper/basic.h"
#include "asio_qt/qt_sql.h"
#include "qcm_interface/global.h"
#include "qcm_interface/sync_api.h"
#include "Qcm/app.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/query/query_model.h"

namespace qcm
{

namespace
{
auto get_hash(const model::ItemId& id) -> usize { return std::hash<model::ItemId> {}(id); }
} // namespace

PlayIdQueue::PlayIdQueue(QObject* parent): model::IdQueue(parent) { setOptions(Options(~0)); }
PlayIdQueue::~PlayIdQueue() {}

PlayIdProxyQueue::PlayIdProxyQueue(QObject* parent)
    : QIdentityProxyModel(parent), m_support_shuffle(true), m_shuffle(true), m_current_index(-1) {}

PlayIdProxyQueue::~PlayIdProxyQueue() {}
void PlayIdProxyQueue::setSourceModel(QAbstractItemModel* source_model) {
    auto old = sourceModel();
    QIdentityProxyModel::setSourceModel(source_model);
    QBindable<qint32> source_idx(source_model, "currentIndex");
    m_current_index.setBinding([source_idx, this] {
        return mapFromSource(source_idx.value());
    });

    shuffleSync();

    auto options      = source_model->property("options").value<model::IdQueue::Options>();
    m_support_shuffle = options.testFlag(model::IdQueue::Option::SupportShuffle);
    if (old) {
        disconnect(
            old, &QAbstractItemModel::rowsInserted, this, &PlayIdProxyQueue::onSourceRowsInserted);
        disconnect(
            old, &QAbstractItemModel::rowsRemoved, this, &PlayIdProxyQueue::onSourceRowsRemoved);
        disconnect(old,
                   &QAbstractItemModel::rowsAboutToBeInserted,
                   this,
                   &PlayIdProxyQueue::onSourceRowsAboutToBeInserted);
    }
    connect(source_model,
            &QAbstractItemModel::rowsInserted,
            this,
            &PlayIdProxyQueue::onSourceRowsInserted);
    connect(source_model,
            &QAbstractItemModel::rowsAboutToBeInserted,
            this,
            &PlayIdProxyQueue::onSourceRowsAboutToBeInserted);
    connect(source_model,
            &QAbstractItemModel::rowsRemoved,
            this,
            &PlayIdProxyQueue::onSourceRowsRemoved);
}

auto PlayIdProxyQueue::shuffle() const -> bool { return m_shuffle; }
void PlayIdProxyQueue::setShuffle(bool v) {
    if (ycore::cmp_exchange(m_shuffle, v)) {
        shuffleChanged();
    };
}
auto PlayIdProxyQueue::useShuffle() const -> bool { return m_support_shuffle && shuffle(); }

auto PlayIdProxyQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void PlayIdProxyQueue::setCurrentIndex(qint32 idx) {
    sourceModel()->setProperty("currentIndex", mapToSource(idx));
}
auto PlayIdProxyQueue::bindableCurrentIndex() -> const QBindable<qint32> {
    return &m_current_index;
}

auto PlayIdProxyQueue::mapToSource(const QModelIndex& proxy_index) const -> QModelIndex {
    if (! proxy_index.isValid()) return QModelIndex();
    return sourceModel()->index(mapToSource(proxy_index.row()), proxy_index.column());
}

auto PlayIdProxyQueue::mapFromSource(const QModelIndex& sourc_index) const -> QModelIndex {
    if (! sourc_index.isValid()) return QModelIndex();
    return index(mapFromSource(sourc_index.row()), sourc_index.column());
}

auto PlayIdProxyQueue::mapToSource(int row) const -> int {
    if (row < 0 || row >= (int)m_shuffle_list.size()) return -1;
    return useShuffle() ? m_shuffle_list[row] : row;
}

auto PlayIdProxyQueue::mapFromSource(int row) const -> int {
    int proxy_row { -1 };
    if (useShuffle()) {
        proxy_row = helper::to_optional(m_source_to_proxy, row).value_or(-1);
    } else {
        proxy_row = row;
    }
    return proxy_row;
}

void PlayIdProxyQueue::shuffleSync() {
    auto count = sourceModel()->rowCount();
    auto old   = (int)m_shuffle_list.size();
    if (old < count) {
        while ((int)m_shuffle_list.size() < count) m_shuffle_list.push_back(m_shuffle_list.size());

        auto cur = m_current_index.value();
        Random::shuffle(m_shuffle_list.begin() + cur + 1, m_shuffle_list.end());
        refreshFromSource();

        // keep cur at queue first
        if (old == 0) {
            auto first = mapFromSource(0);
            std::swap(m_shuffle_list[first], m_shuffle_list[0]);
            m_current_index.value();
        }
    } else if (old > count) {
        for (int i = 0, k = 0; i < count; i++) {
            if (m_shuffle_list[i] >= count) {
                std::swap(m_shuffle_list[i], m_shuffle_list[old - k]);
                k++;
            }
        }
        m_shuffle_list.resize(count);
        refreshFromSource();

        // do not need check cur here
        // let upstream check later
        // if upstream no change, cur is ok
    }
}

void PlayIdProxyQueue::refreshFromSource() {
    int rowCount = sourceModel()->rowCount();
    m_source_to_proxy.clear();
    for (int proxyRow = 0; proxyRow < rowCount; ++proxyRow) {
        int sourceRow                = m_shuffle_list[proxyRow];
        m_source_to_proxy[sourceRow] = proxyRow;
    }
}

void PlayIdProxyQueue::onSourceRowsInserted(const QModelIndex&, int, int) { shuffleSync(); }
void PlayIdProxyQueue::onSourceRowsRemoved(const QModelIndex&, int, int) { shuffleSync(); }

void PlayIdProxyQueue::onSourceRowsAboutToBeInserted(const QModelIndex&, int, int) {}

PlayQueue::PlayQueue(QObject* parent)
    : meta_model::QMetaModelBase<QIdentityProxyModel>(parent),
      m_proxy(new PlayIdProxyQueue(parent)),
      m_loop_mode(LoopMode::NoneLoop),
      m_can_next(false),
      m_can_prev(false) {
    updateRoleNames(query::Song::staticMetaObject);
    connect(this, &PlayQueue::currentIndexChanged, this, [this](qint32 idx) {
        setCurrentSong(idx);
    });
    connect(m_proxy, &PlayIdProxyQueue::currentIndexChanged, this, &PlayQueue::checkCanMove);
    connect(this, &PlayQueue::loopModeChanged, m_proxy, [this] {
        m_proxy->setShuffle(loopMode() == LoopMode::ShuffleLoop);
    });
    loopModeChanged();
}
PlayQueue::~PlayQueue() {}

auto PlayQueue::data(const QModelIndex& index, int role) const -> QVariant {
    auto row = index.row();
    auto id  = getId(row);
    do {
        if (! id) break;
        auto hash  = std::hash<model::ItemId> {}(id.value());
        auto it    = m_songs.find(hash);
        bool it_ok = it != m_songs.end();

        if (auto prop = this->propertyOfRole(role); prop) {
            if (it_ok) {
                const query::Song& song = it->second;
                return prop.value().readOnGadget(&song);
            } else if (prop->name() == "itemId"sv) {
                return QVariant::fromValue(*id);
            } else {
                return prop.value().readOnGadget(&m_placeholder);
            }
        }
    } while (0);
    return {};
}

void PlayQueue::setSourceModel(QAbstractItemModel* source_model) {
    auto old = sourceModel();
    base_type::setSourceModel(source_model);
    QBindable<qint32> source_idx(source_model, "currentIndex");
    m_current_index.setBinding([source_idx] {
        return source_idx.value();
    });

    if (old) {
        disconnect(old, &QAbstractItemModel::rowsInserted, this, &PlayQueue::onSourceRowsInserted);
        disconnect(old, &QAbstractItemModel::rowsRemoved, this, &PlayQueue::onSourceRowsRemoved);
        disconnect(old,
                   &QAbstractItemModel::rowsAboutToBeRemoved,
                   this,
                   &PlayQueue::onSourceRowsAboutToBeRemoved);
        disconnect(this, SIGNAL(requestNext), old, SIGNAL(requestNext));
    }
    connect(this, SIGNAL(requestNext), source_model, SIGNAL(requestNext));
    connect(
        source_model, &QAbstractItemModel::rowsInserted, this, &PlayQueue::onSourceRowsInserted);
    connect(source_model, &QAbstractItemModel::rowsRemoved, this, &PlayQueue::onSourceRowsRemoved);
    connect(source_model,
            &QAbstractItemModel::rowsAboutToBeRemoved,
            this,
            &PlayQueue::onSourceRowsAboutToBeRemoved);

    m_options = source_model->property("options").value<model::IdQueue::Options>();
    m_proxy->setSourceModel(source_model);
    checkCanMove();
}

auto PlayQueue::currentSong() const -> query::Song {
    if (m_current_song) return *m_current_song;
    query::Song s {};
    if (auto id = currentId()) {
        s.id = *id;
    }
    return s;
}

void PlayQueue::setCurrentSong(const std::optional<query::Song>& s) {
    auto get_id = [](const auto& el) {
        return el.id;
    };
    if (m_current_song.transform(get_id) != s.transform(get_id)) {
        m_current_song = s;
        currentSongChanged();
    }
}
void PlayQueue::setCurrentSong(qint32 idx) {
    setCurrentSong(getId(idx).and_then([this](const auto& id) -> std::optional<query::Song> {
        auto hash = std::hash<model::ItemId>()(id);
        if (auto it = m_songs.find(hash); it != m_songs.end()) {
            return it->second;
        }
        return std::nullopt;
    }));
}

auto PlayQueue::currentId() const -> std::optional<model::ItemId> { return getId(currentIndex()); }

auto PlayQueue::getId(qint32 idx) const -> std::optional<model::ItemId> {
    if (auto source = sourceModel()) {
        auto val = source->data(source->index(idx, 0));
        if (auto pval = get_if<model::ItemId>(&val)) {
            return *pval;
        }
    }
    return std::nullopt;
}

auto PlayQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
auto PlayQueue::bindableCurrentIndex() -> const QBindable<qint32> { return &m_current_index; }

auto PlayQueue::loopMode() const -> enums::LoopMode { return m_loop_mode; }
void PlayQueue::setLoopMode(enums::LoopMode mode) {
    if (ycore::cmp_exchange(m_loop_mode, mode)) {
        loopModeChanged();
    }
}
void PlayQueue::iterLoopMode() {
    using M   = LoopMode;
    auto mode = loopMode();
    switch (mode) {
    case M::NoneLoop: mode = M::SingleLoop; break;
    case M::SingleLoop: mode = M::ListLoop; break;
    case M::ListLoop: mode = M::ShuffleLoop; break;
    case M::ShuffleLoop: mode = M::NoneLoop; break;
    }
    setLoopMode(mode);
}

auto PlayQueue::canNext() const -> bool { return m_can_next; }

auto PlayQueue::canPrev() const -> bool { return m_can_prev; }
void PlayQueue::setCanNext(bool v) {
    if (ycore::cmp_exchange(m_can_next, v)) {
        canNextChanged();
    }
}
void PlayQueue::setCanPrev(bool v) {
    if (ycore::cmp_exchange(m_can_prev, v)) {
        canPrevChanged();
    }
}

void PlayQueue::next() { next(loopMode()); }
void PlayQueue::prev() { prev(loopMode()); }
void PlayQueue::next(LoopMode mode) {
    bool support_loop = m_options.testFlag(Option::SupportLoop);
    auto count        = m_proxy->rowCount();
    auto cur          = m_proxy->currentIndex();
    switch (mode) {
    case LoopMode::NoneLoop: {
        if (cur + 1 < count) {
            m_proxy->setCurrentIndex(cur + 1);
        }
        break;
    }
    case LoopMode::ListLoop:
    case LoopMode::ShuffleLoop: {
        m_proxy->setCurrentIndex((cur + 1) % count);
        break;
    }
    case LoopMode::SingleLoop: {
    }
    }
    requestNext();
}
void PlayQueue::prev(LoopMode mode) {
    bool support_loop = m_options.testFlag(Option::SupportLoop);
    auto count        = m_proxy->rowCount();
    auto cur          = m_proxy->currentIndex();
    switch (mode) {
    case LoopMode::NoneLoop: {
        if (cur >= 1) {
            m_proxy->setCurrentIndex(cur - 1);
        }
        break;
    }
    case LoopMode::ListLoop:
    case LoopMode::ShuffleLoop: {
        m_proxy->setCurrentIndex(cur == 0 ? std::max(count, 1) - 1 : cur);
        break;
    }
    case LoopMode::SingleLoop: {
    }
    }
    requestNext();
}

void PlayQueue::clear() {}

auto PlayQueue::querySongsSql(std::span<const model::ItemId> ids)
    -> task<std::vector<query::Song>> {
    std::vector<query::Song> out;
    auto                     sql = App::instance()->album_sql();
    QStringList              placeholders;
    for (usize i = 0; i < ids.size(); ++i) {
        placeholders << u":id%1"_s.arg(i);
    }
    co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
    auto query = sql->con()->query();
    query.prepare(uR"(
SELECT 
    %1
FROM song
JOIN album ON song.albumId = album.itemId
JOIN song_artist ON song.itemId = song_artist.songId
JOIN artist ON song_artist.artistId = artist.itemId
WHERE song.itemId IN (%2)
GROUP BY song.itemId
ORDER BY song.trackNumber ASC;
)"_s.arg(query::Song::Select)
                      .arg(placeholders.join(",")));

    for (usize i = 0; i < ids.size(); ++i) {
        query.bindValue(placeholders[i], ids[i].toUrl());
    }
    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
    } else {
        while (query.next()) {
            auto& s = out.emplace_back();
            int   i = 0;
            query::load_query(s, query, i);
        }
    }

    co_return out;
}

auto PlayQueue::querySongs(std::span<const model::ItemId> ids) -> task<void> {
    auto sql     = App::instance()->album_sql();
    auto missing = co_await sql->missing(ItemSql::Table::SONG, ids);
    if (! missing.empty()) co_await query::SyncAPi::sync_items(missing);
    auto songs = co_await querySongsSql(ids);
    co_await asio::post(asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
    std::unordered_set<usize> hashes;
    for (auto& s : songs) {
        auto hash = get_hash(s.id);
        hashes.insert(hash);
        m_songs.insert_or_assign(hash, s);
    }

    for (int i = 0; i < rowCount(); i++) {
        auto hash = getId(i).transform(get_hash).value_or(0);
        if (hashes.contains(hash)) {
            auto idx = index(i, 0);
            dataChanged(idx, idx);
        }
    }

    if (currentIndex() != -1 && ! m_current_song) {
        setCurrentSong(currentIndex());
    }
    co_return;
}

void PlayQueue::onSourceRowsInserted(const QModelIndex&, int first, int last) {
    std::vector<model::ItemId> ids;
    for (int i = first; i <= last; i++) {
        if (auto id = getId(i)) {
            ids.emplace_back(id.value());
        }
    }

    auto ex = asio::make_strand(Global::instance()->pool_executor());
    asio::co_spawn(
        ex,
        [ids, this] -> task<void> {
            co_await querySongs(ids);
            co_return;
        },
        helper::asio_detached_log_t {});

    checkCanMove();
}

void PlayQueue::onSourceRowsAboutToBeRemoved(const QModelIndex&, int first, int last) {
    for (int i = first; i <= last; i++) {
        auto id = getId(i);
        if (id) {
            m_songs.erase(get_hash(*id));
        }
    }
}
void PlayQueue::onSourceRowsRemoved(const QModelIndex&, int, int) { checkCanMove(); }
void PlayQueue::checkCanMove() {
    bool support_prev       = m_options.testFlag(Option::SupportPrev);
    bool support_loop       = m_options.testFlag(Option::SupportLoop);
    auto check_on_none_loop = [this, support_prev] {
        setCanPrev(m_proxy->currentIndex() > 0 && support_prev);
        setCanNext(m_proxy->rowCount() > m_proxy->currentIndex() + 1);
    };
    switch (m_loop_mode) {
    case LoopMode::NoneLoop: {
        check_on_none_loop();
        break;
    }
    case LoopMode::ShuffleLoop:
    case LoopMode::ListLoop: {
        if (support_loop) {
            setCanPrev(support_prev);
            setCanNext(true);
        } else {
            check_on_none_loop();
        }
    }
    default: {
        setCanPrev(support_prev);
        setCanNext(true);
    }
    }
}

} // namespace qcm