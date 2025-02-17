#include "Qcm/sql/item_sql.h"

#include <unordered_set>
#include <asio/bind_executor.hpp>
#include <asio/use_awaitable.hpp>
#include "asio_qt/qt_sql.h"
#include "core/str_helper.h"
#include "core/strv_helper.h"
#include "core/qstr_helper.h"
#include "qcm_interface/sql/meta_sql.h"
#include "Qcm/query/query_load.h"

namespace qcm
{
namespace
{
auto get_song_ignore() -> std::set<std::string_view> { return { "source"sv, "sourceId"sv }; }
auto song_ignore(const std::set<std::string>& in) {
    auto out = in;
    out.insert("^source");
    out.insert("^sourceId");
    return out;
}
auto item_id_converter(const QVariant& v) -> QVariant {
    auto id = v.value<model::ItemId>();
    return id.toUrl();
}
} // namespace

ItemSql::ItemSql(rc<helper::SqlConnect> con)
    : m_album_table("album"),
      m_artist_table("artist"),
      m_song_table("song"),
      m_album_artist_table("album_artist"),
      m_song_artist_table("song_artist"),
      m_mix_table("playlist"),
      m_mix_song_table("playlist_song"),
      m_radio_table("radio"),
      m_program_table("program"),
      m_radio_program_table("radio_program"),
      m_con(con) {
    asio::dispatch(m_con->get_executor(), [this] {
        create_album_table();
        create_artist_table();
        create_song_table();
        create_album_artist_table();
        create_song_artist_table();
        create_mix_table();
        create_mix_song_table();
        create_radio_table();
        create_program_table();
        create_radio_program_table();
    });
}
ItemSql::~ItemSql() {}

void ItemSql::create_album_table() {
    m_con->transaction();

    auto migs = m_con->generate_meta_migration(
        m_album_table, u"", model::Album::staticMetaObject, { { "itemId"sv } }, { "name"s });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}

void ItemSql::create_artist_table() {
    m_con->transaction();

    auto migs = m_con->generate_meta_migration(
        m_artist_table, u"", model::Artist::staticMetaObject, { { "itemId"sv } });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}

void ItemSql::create_song_table() {
    m_con->transaction();

    auto migs = m_con->generate_meta_migration(m_song_table,
                                               u"",
                                               model::Song::staticMetaObject,
                                               { { "itemId"sv } },
                                               { "name"s },
                                               get_song_ignore());

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}

void ItemSql::create_mix_table() {
    m_con->transaction();

    auto migs = m_con->generate_meta_migration(
        m_mix_table, u"", model::Mix::staticMetaObject, { { "itemId"sv } });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}

void ItemSql::create_radio_table() {
    m_con->transaction();

    auto migs = m_con->generate_meta_migration(
        m_radio_table, u"", model::Radio::staticMetaObject, { { "itemId"sv } });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}

void ItemSql::create_program_table() {
    m_con->transaction();

    auto migs = m_con->generate_meta_migration(
        m_program_table, u"", model::Program::staticMetaObject, { { "itemId"sv } });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}

void ItemSql::create_album_artist_table() {
    m_con->transaction();

    auto migs = m_con->generate_column_migration(
        m_album_artist_table,
        std::array {
            helper::SqlColumn { .name = "albumId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "artistId", .type = "TEXT", .notnull = 1 },
            m_con->EditTimeColumn,
        },
        { helper::SqlUnique { "albumId"sv, "artistId"sv } });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}

void ItemSql::create_song_artist_table() {
    m_con->transaction();
    auto migs = m_con->generate_column_migration(
        m_song_artist_table,
        std::array {
            helper::SqlColumn { .name = "songId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "artistId", .type = "TEXT", .notnull = 1 },
            m_con->EditTimeColumn,
        },
        { { "songId"sv, "artistId"sv } });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}

void ItemSql::create_mix_song_table() {
    m_con->transaction();

    auto migs = m_con->generate_column_migration(
        m_mix_song_table,
        std::array {
            helper::SqlColumn { .name = "songId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "playlistId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "orderIdx", .type = "INTEGER" },
            helper::SqlColumn { .name = "removed", .type = "INTEGER", .dflt_value = "0"s },
            m_con->EditTimeColumn,
        },
        { { "songId"sv, "playlistId"sv } });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}
void ItemSql::create_radio_program_table() {
    m_con->transaction();

    auto migs = m_con->generate_column_migration(
        m_radio_program_table,
        std::array {
            helper::SqlColumn { .name = "radioId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "programId", .type = "TEXT", .notnull = 1 },
            m_con->EditTimeColumn,
        },
        { { "radioId"sv, "programId"sv } });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->rollback();
            return;
        }
    }
    m_con->commit();
}

auto ItemSql::get_executor() -> QtExecutor& { return m_con->get_executor(); }
auto ItemSql::con() const -> rc<helper::SqlConnect> { return m_con; }

auto ItemSql::insert(std::span<const model::Album> items, ListParam columns, ListParam on_update)
    -> task<bool> {
    DEBUG_LOG("start insert album, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(
        m_album_table, { "itemId"s }, model::Album::staticMetaObject, items, columns, on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    DEBUG_LOG("end insert");
    co_return true;
}

auto ItemSql::insert(std::span<const model::Artist> items, ListParam columns,
                     const std::set<std::string>& on_update) -> task<bool> {
    DEBUG_LOG("start insert artist, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(
        m_artist_table, { "itemId"s }, model::Artist::staticMetaObject, items, columns, on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    DEBUG_LOG("end insert");
    co_return true;
}
auto ItemSql::insert(std::span<const model::Song> items, ListParam columns, ListParam on_update)
    -> task<bool> {
    DEBUG_LOG("start insert song, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_song_table,
                                                       { "itemId"s },
                                                       model::Song::staticMetaObject,
                                                       items,
                                                       song_ignore(columns),
                                                       on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    DEBUG_LOG("end insert");
    co_return true;
}
auto ItemSql::insert(std::span<const model::Mix> items, ListParam columns,
                     const std::set<std::string>& on_update) -> task<bool> {
    DEBUG_LOG("start insert playlist, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(
        m_mix_table, { "itemId"s }, model::Mix::staticMetaObject, items, columns, on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    DEBUG_LOG("end insert");
    co_return true;
}
auto ItemSql::insert(std::span<const model::Radio> items, ListParam columns,
                     const std::set<std::string>& on_update) -> task<bool> {
    DEBUG_LOG("start insert radio, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(
        m_radio_table, { "itemId"s }, model::Radio::staticMetaObject, items, columns, on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    DEBUG_LOG("end insert");
    co_return true;
}
auto ItemSql::insert(std::span<const model::Program> items, ListParam columns,
                     const std::set<std::string>& on_update) -> task<bool> {
    DEBUG_LOG("start insert program, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_program_table,
                                                       { "itemId"s },
                                                       model::Program::staticMetaObject,
                                                       items,
                                                       columns,
                                                       on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    DEBUG_LOG("end insert");
    co_return true;
}

auto ItemSql::insert_album_artist(std::span<const IdPair> ids) -> task<bool> {
    QVariantList albumIds, artistIds;
    for (auto& el : ids) {
        albumIds << std::get<0>(el).toUrl();
        artistIds << std::get<1>(el).toUrl();
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare(uR"(
INSERT OR IGNORE INTO %1 (albumId, artistId) VALUES (:albumId, :artistId);
)"_s.arg(m_album_artist_table));
    query.bindValue(":albumId", albumIds);
    query.bindValue(":artistId", artistIds);
    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    co_return true;
}

auto ItemSql::insert_song_artist(std::span<const IdPair> ids) -> task<bool> {
    QVariantList songIds, artistIds;
    for (auto& el : ids) {
        songIds << std::get<0>(el).toUrl();
        artistIds << std::get<1>(el).toUrl();
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare(uR"(
INSERT OR IGNORE INTO %1 (songId, artistId) VALUES (:songId, :artistId);
)"_s.arg(m_song_artist_table));
    query.bindValue(":songId", songIds);
    query.bindValue(":artistId", artistIds);
    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    co_return true;
}

auto ItemSql::insert_radio_program(std::span<const IdPair> ids) -> task<bool> {
    QVariantList radioIds, programIds;
    for (auto& el : ids) {
        radioIds << std::get<0>(el).toUrl();
        programIds << std::get<1>(el).toUrl();
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare(uR"(
INSERT OR IGNORE INTO %1 (radioId, programId) VALUES (:radioId, :programId);
)"_s.arg(m_song_artist_table));
    query.bindValue(":radioId", radioIds);
    query.bindValue(":programId", programIds);
    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    co_return true;
}

auto ItemSql::insert_mix_song(u32 last, u32 count, const model::ItemId& playlist_id,
                              std::span<const model::ItemId> song_ids) -> bool {
    Q_UNUSED(count);
    u32 limit = 100;

    auto query         = m_con->query();
    auto rerange_query = m_con->query();
    query.prepare(uR"(
INSERT OR REPLACE INTO %1 (playlistId, songId, orderIdx)
VALUES (:playlistId, :songId, :orderIdx);
)"_s.arg(m_mix_song_table));
    rerange_query.prepare(uR"(
WITH OrderedSongs AS (
    SELECT songId,
           ROW_NUMBER() OVER (ORDER BY orderIdx) * :limit AS newOrderIdx
    FROM playlist_song
    WHERE playlistId = :playlistId
)
UPDATE playlist_song
SET orderIdx = (SELECT newOrderIdx FROM OrderedSongs WHERE playlist_song.songId = OrderedSongs.songId)
WHERE playlistId = :playlistId;
)"_s);
    u32 offset { last };
    for (u32 i = 0; i < song_ids.size(); i += limit - 1, offset += (limit - 1) * limit) {
        QVariantList playlist_id_list;
        QVariantList song_id_list;
        QVariantList order_idx;
        auto         n = std::min<usize>(i + limit, song_ids.size());
        for (u32 j = i, k = 1; j < n; j++, k++) {
            playlist_id_list << playlist_id.toUrl();
            song_id_list << song_ids[j].toUrl();
            order_idx << offset + k;
        }

        query.bindValue(":playlistId", playlist_id_list);
        query.bindValue(":songId", song_id_list);
        query.bindValue(":orderIdx", order_idx);
        if (! query.execBatch()) {
            ERROR_LOG("{}", query.lastError().text());
            return false;
        }

        rerange_query.bindValue(":limit", limit);
        rerange_query.bindValue(":playlistId", playlist_id.toUrl());
        if (! rerange_query.exec()) {
            ERROR_LOG("{}", rerange_query.lastError().text());
            return false;
        }
    }
    return true;
}

auto ItemSql::insert_mix_song(i32 pos, model::ItemId playlist_id,
                              std::span<const model::ItemId> song_ids) -> task<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();
    query.prepare(uR"(
SELECT 
    COALESCE(
        (SELECT orderIdx
         FROM playlist_song
         WHERE playlistId = :playlistId
         ORDER BY orderIdx
         LIMIT 1 OFFSET :pos - 1),
        (SELECT orderIdx
         FROM playlist_song
         WHERE playlistId = :playlistId
         ORDER BY orderIdx DESC
         LIMIT 1)
    ) AS orderIdx,
    COUNT(*) AS totalCount
FROM playlist_song
WHERE playlistId = :playlistId;
)"_s);

    query.bindValue(":pos", pos);
    query.bindValue(":playlistId", playlist_id.toUrl());

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        co_return false;
    } else if (query.next()) {
        u32 last  = query.value(0).toUInt();
        u32 count = query.value(1).toUInt();
        if (pos == 0) last = 0;
        m_con->transaction();
        if (insert_mix_song(last, count, playlist_id, song_ids)) {
            m_con->commit();
        } else {
            m_con->rollback();
            co_return false;
        }
    } else {
    }

    co_return true;
}
auto ItemSql::remove_mix_song(model::ItemId mix_id, std::span<const model::ItemId> song_ids)
    -> task<bool> {
    QStringList placeholders;
    for (usize i = 0; i < song_ids.size(); ++i) {
        placeholders << u":id%1"_s.arg(i);
    }
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = con()->query();
    query.prepare_sv(fmt::format(R"(
DELETE FROM {0}
WHERE playlistId = :playlistId AND songId IN ({1});
)",
                                 m_mix_song_table,
                                 placeholders.join(",")));

    query.bindValue(":playlistId", mix_id.toUrl());
    for (usize i = 0; i < song_ids.size(); ++i) {
        query.bindValue(placeholders[i], song_ids[i].toUrl());
    }

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        co_return false;
    }
    co_return true;
}

auto ItemSql::refresh_mix_song(i32 pos, model::ItemId mix_id,
                               std::span<const model::ItemId> song_ids) -> task<bool> {
    Q_UNUSED(pos);
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    m_con->transaction();

    auto query = con()->query();

    query.prepare_sv(fmt::format(R"(
DELETE FROM {0}
WHERE playlistId = :playlistId;
)",
                                 m_mix_song_table));
    query.bindValue(":playlistId", mix_id.toUrl());

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }

    if (! insert_mix_song(0, 0, mix_id, song_ids)) {
        m_con->rollback();
        co_return false;
    }

    if (! m_con->commit()) {
        co_return false;
    }

    co_return true;
}
auto ItemSql::select_mix(const model::ItemId& user_id, qint32 special_type)
    -> task<std::optional<model::Mix>> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = con()->query();

    query.prepare_sv(fmt::format(R"(
SELECT 
    {1}
FROM {0}
WHERE userId = :userId AND specialType = :specialType
LIMIT 1;
)",
                                 m_mix_table,
                                 model::Mix::sql().select));
    query.bindValue(":userId", user_id.toUrl());
    query.bindValue(":specialType", special_type);
    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
    }

    if (query.next()) {
        model::Mix m;
        int        i = 0;
        query::load_query(query, m, i);
        co_return m;
    }
    co_return std::nullopt;
}

auto ItemSql::table_name(Table t) const -> QStringView {
    switch (t) {
    case Table::ALBUM: return m_album_table;
    case Table::SONG: return m_song_table;
    case Table::ALBUM_ARTIST: return m_album_artist_table;
    case Table::PLAYLIST_SONG: return m_mix_song_table;
    case Table::PLAYLIST: return m_mix_table;
    case Table::ARTIST: return m_artist_table;
    case Table::SONG_ARTIST: return m_song_artist_table;
    case Table::RADIO: return m_radio_table;
    }
    _assert_rel_(false);
    return {};
}
auto ItemSql::clean(const QDateTime& before, Table table) -> task<void> {
    auto cur = QDateTime::currentDateTimeUtc();

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare(uR"(
DELETE FROM %1
WHERE _editTime < :before OR _editTime > :after;
)"_s.arg(table_name(table)));
    query.bindValue(":before", before);
    query.bindValue(":after", cur);
    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
    }
}

auto ItemSql::missing(std::span<const model::ItemId> ids, Table table, std::optional<Table> join,
                      ListParam not_null) -> task<std::vector<model::ItemId>> {
    std::unordered_set<model::ItemId> include;
    std::vector<model::ItemId>        out;
    QStringList                       placeholders;
    for (usize i = 0; i < ids.size(); ++i) {
        placeholders << u":id%1"_s.arg(i);
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    query.prepare_sv(fmt::format(
        R"(
SELECT 
    {0}.itemId
FROM {0}
{1}
WHERE {0}.itemId IN ({2}) {3};
)",
        table_name(table),
        join.transform([this, table](const auto& in) {
                return fmt::format(
                    "LEFT JOIN {1} ON {1}.itemId = {0}.itemId", table_name(table), table_name(in));
            })
            .value_or(""s),
        placeholders.join(", "),
        not_null.size() ? fmt::format("AND ({})", db::null<db::Logical::AND, db::Eq::NOT>(not_null))
                        : ""s));
    for (usize i = 0; i < ids.size(); ++i) {
        query.bindValue(placeholders[i], ids[i].toUrl());
    }
    if (query.exec()) {
        while (query.next()) {
            include.insert(model::ItemId { query.value(0).toUrl() });
        }
    } else {
        ERROR_LOG("{}", query.lastError().text());
    }
    for (auto& el : ids) {
        if (! include.contains(el)) {
            out.emplace_back(el);
        }
    }
    co_return out;
}

} // namespace qcm