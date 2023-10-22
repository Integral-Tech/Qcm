#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/qrcode_unikey.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class QrcodeUnikey : public QObject {
    Q_OBJECT
public:
    QrcodeUnikey(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::QrcodeUnikey;

    READ_PROPERTY(QString, key, m_key, infoChanged)
    READ_PROPERTY(QString, qrurl, m_qrurl, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_key, in.unikey);
        convert(o.m_qrurl, in.qrurl);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<QrcodeUnikey, ncm::api::QrcodeUnikey>);

} // namespace model

using QrcodeUnikeyQuerier_base = ApiQuerier<ncm::api::QrcodeUnikey, model::QrcodeUnikey>;
class QrcodeUnikeyQuerier : public QrcodeUnikeyQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    QrcodeUnikeyQuerier(QObject* parent = nullptr): QrcodeUnikeyQuerier_base(parent) {}
};
} // namespace qcm
