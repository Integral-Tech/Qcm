#pragma once

#include <QQmlEngine>
#include "core/core.h"
#include "qcm_interface/export.h"
#include "qcm_interface/async.h"
#include "meta_model/qobjectlistmodel.h"
#include "json_helper/helper.h"
#include "qcm_interface/model/user_account.h"

namespace qcm
{

namespace model
{
class UserAccount;
}

class QCM_INTERFACE_API UserModel
    : public meta_model::detail::QObjectListModel<model::UserAccount> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::UserAccount* activeUser READ active_user WRITE set_active_user NOTIFY
                   activeUserChanged)
    Q_PROPERTY(QAsyncResult* checkResult READ check_result NOTIFY checkResultChanged)
public:
    UserModel(QObject* parent = nullptr);
    ~UserModel();

    Q_INVOKABLE void check_user(model::UserAccount* = nullptr);

    auto find_by_url(const QUrl&) const -> model::UserAccount*;
    auto active_user() const -> model::UserAccount*;
    auto check_result() const -> QAsyncResult*;

public Q_SLOTS:
    void add_user(model::UserAccount*);
    void delete_user();
    void set_active_user(model::UserAccount*);

Q_SIGNALS:
    void activeUserChanged();
    void checkResultChanged();

private:
    class Private;
    C_DECLARE_PRIVATE(UserModel, d_ptr);
};
} // namespace qcm
DECLARE_JSON_SERIALIZER(qcm::UserModel);