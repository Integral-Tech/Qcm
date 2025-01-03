#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/login.h"

#include "core/log.h"

namespace ncm::qml
{

namespace model
{

class Login : public QObject {
    Q_OBJECT
public:
    Login(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::Login;

    READ_PROPERTY(qint32, code, m_code, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_code, in.code);
        emit infoChanged();
    }

    Q_SIGNAL void infoChanged();
};

} // namespace model

using LoginQuerier_base = NcmApiQuery<ncm::api::Login, model::Login>;
class LoginQuerier : public LoginQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    LoginQuerier(QObject* parent = nullptr): LoginQuerier_base(parent) {}

    FORWARD_PROPERTY(QString, username, username)
    FORWARD_PROPERTY(QString, password, password_md5)
};
} // namespace ncm::qml
