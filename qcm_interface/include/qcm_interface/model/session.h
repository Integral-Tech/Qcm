#pragma once
#include <QObject>
#include "qcm_interface/model.h"
#include "qcm_interface/model/user_model.h"
#include "qcm_interface/model/page.h"

namespace qcm::model
{

class QCM_INTERFACE_API Session : public Model<Session, QObject> {
    Q_OBJECT
    DECLARE_MODEL()
public:
    Session(QObject* parent = nullptr);
    ~Session();

    DECLARE_PROPERTY(UserAccount*, user, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(std::vector<Page>, pages, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(bool, valid, NOTIFY_NAME(infoChanged))

Q_SIGNALS:
    void infoChanged();
};

} // namespace qcm::model