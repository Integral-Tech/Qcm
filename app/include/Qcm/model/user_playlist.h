#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/user_playlist.h"

#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
struct UserPlaylistItem {
    Q_GADGET
public:
    GATGET_PROPERTY(PlaylistId, itemId, itemId)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)
    GATGET_PROPERTY(qint32, playCount, playCount)
    GATGET_PROPERTY(qint32, trackCount, trackCount)
};
} // namespace model
} // namespace qcm

template<>
struct To<qcm::model::UserPlaylistItem> {
    static auto from(const ncm::model::UserPlaylistItem& in) {
        qcm::model::UserPlaylistItem o;
        CONVERT_PROPERTY(o.itemId, in.id);
        CONVERT_PROPERTY(o.name, in.name);
        CONVERT_PROPERTY(o.picUrl, in.coverImgUrl);
        CONVERT_PROPERTY(o.playCount, in.playCount);
        CONVERT_PROPERTY(o.trackCount, in.trackCount);
        return o;
    };
};

namespace qcm
{
namespace model
{

class UserPlaylist : public meta_model::QGadgetListModel<UserPlaylistItem> {
    Q_OBJECT
public:
    UserPlaylist(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<UserPlaylistItem>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::UserPlaylist;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset == 0) {
            auto& list       = re.playlist;
            bool  need_reset = list.size() > (usize)rowCount();
            if (! need_reset) {
                for (usize i = 0; i < list.size(); i++) {
                    auto id = To<PlaylistId>::from(list[i].id);
                    if (id != items()[i].itemId) {
                        need_reset = true;
                        break;
                    }
                }
            }
            if (need_reset)
                resetModel({});
            else
                return;
        } else if (input.offset != (int)rowCount()) {
            return;
        }

        for (auto& el : re.playlist) {
            insert(rowCount(), To<UserPlaylistItem>::from(el));
        }
        m_has_more = re.more;
    }

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }
signals:
    void fetchMoreReq(qint32);

private:
    bool m_has_more;
};
static_assert(modelable<UserPlaylist, ncm::api::UserPlaylist>);
} // namespace model

using UserPlaylistQuerier_base = ApiQuerier<ncm::api::UserPlaylist, model::UserPlaylist>;
class UserPlaylistQuerier : public UserPlaylistQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    UserPlaylistQuerier(QObject* parent = nullptr): UserPlaylistQuerier_base(parent) {}

    FORWARD_PROPERTY(model::UserId, uid, uid)
    FORWARD_PROPERTY(qint32, offset, offset)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    void fetch_more(qint32 cur_count) override { set_offset(cur_count); }
public slots:
    void reset() {
        api().input.offset = 0;
        reload();
    }
};

} // namespace qcm
