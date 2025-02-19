#pragma once

#include "core/core.h"
#include "qcm_interface/sql/item_sql.h"

namespace helper
{
class SqlConnect;
}
namespace qcm
{

class ItemSql : public db::ItemSqlBase {
public:
    ItemSql(rc<helper::SqlConnect> con);
    ~ItemSql();

    auto get_executor() -> QtExecutor& override;
    auto con() const -> rc<helper::SqlConnect> override;

    auto library_id(i64 provider_id, const QString& native_id) -> task<i64> override;
    auto library_id_list() -> task<std::vector<i64>> override;
    auto library_list() -> task<std::vector<model::Library>> override;
    auto create_library(model::Library) -> task<model::Library> override;
    auto delete_library(i64) -> task<bool> override;

    auto insert(std::span<const model::Album> items, ListParam columns, ListParam on_update = {})
        -> task<bool> override;
    auto insert(std::span<const model::Artist> items, ListParam columns, ListParam on_update = {})
        -> task<bool> override;
    auto insert(std::span<const model::Song> items, ListParam columns, ListParam on_update = {})
        -> task<bool> override;
    auto insert(std::span<const model::Mix> items, ListParam columns, ListParam on_update = {})
        -> task<bool> override;
    auto insert(std::span<const model::Radio> items, ListParam columns, ListParam on_update = {})
        -> task<bool> override;
    auto insert(std::span<const model::Program> items, ListParam columns, ListParam on_update = {})
        -> task<bool> override;
    auto insert(std::span<const model::Library> items, ListParam columns, ListParam on_update = {})
        -> task<bool> override;

    auto insert_album_artist(std::span<const RelationInsertId>) -> task<bool> override;
    auto insert_song_artist(std::span<const RelationInsertId>) -> task<bool> override;
    auto insert_radio_program(std::span<const RelationInsertId>) -> task<bool> override;

    auto insert_mix_song(i32 lib_id, i32 pos, model::ItemId mix_id,
                         std::span<const model::ItemId> song_ids) -> task<bool> override;
    auto remove_mix_song(i32 lib_id, model::ItemId mix_id, std::span<const model::ItemId> song_ids)
        -> task<bool> override;
    auto refresh_mix_song(i32 lib_id, i32 pos, model::ItemId mix_id,
                          std::span<const model::ItemId> song_ids) -> task<bool> override;
    auto select_mix(const model::ItemId& user_id, qint32 special_type)
        -> task<std::optional<model::Mix>> override;

    auto table_name(Table) const -> QStringView override;
    auto clean(const QDateTime& before, Table table) -> task<void>;
    auto missing(std::span<const model::ItemId> ids, Table table, std::optional<Table> join = {},
                 ListParam not_null = {}) -> task<std::vector<model::ItemId>>;

private:
    auto insert_mix_song(i32 lib_id, u32 last, u32 count, const model::ItemId& mix_id,
                         std::span<const model::ItemId> song_ids) -> bool;
    void create_library_table();
    void create_album_table();
    void create_artist_table();
    void create_song_table();
    void create_mix_table();
    void create_radio_table();
    void create_program_table();

    void create_song_artist_table();
    void create_album_artist_table();
    void create_mix_song_table();
    void create_radio_program_table();

    QString m_album_table;
    QString m_artist_table;
    QString m_song_table;
    QString m_album_artist_table;
    QString m_song_artist_table;
    QString m_mix_table;
    QString m_mix_song_table;
    QString m_radio_table;
    QString m_program_table;
    QString m_radio_program_table;
    QString m_library_table;

    rc<helper::SqlConnect> m_con;
};

} // namespace qcm
