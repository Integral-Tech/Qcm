#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/sql/model_sql.h"

namespace qcm::model
{

struct ArtistRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(qcm::model::ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    std::strong_ordering operator<=>(const ArtistRefer&) const = default;

    QCM_INTERFACE_API static auto sql() -> const ModelSql&;
};

struct Artist : ArtistRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(qint32, albumCount, albumCount)
    GADGET_PROPERTY_DEF(qint32, musicCount, musicCount)
    GADGET_PROPERTY_DEF(std::vector<QString>, alias, alias)

    QCM_INTERFACE_API static auto sql() -> const ModelSql&;
};

} // namespace qcm::model