#pragma once

#include <QObject>
#include "Qcm/macro.hpp"

namespace qcm
{
namespace model
{

class AppInfo {
    Q_GADGET
public:
    AppInfo();
    ~AppInfo();

    Q_PROPERTY(QString id MEMBER id CONSTANT FINAL)
    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(QString version MEMBER version CONSTANT FINAL)
    Q_PROPERTY(QString author MEMBER author CONSTANT FINAL)
    Q_PROPERTY(QString summary MEMBER summary CONSTANT FINAL)
private:
    QString id;
    QString name;
    QString version;
    QString author;
    QString summary;
};

} // namespace model
} // namespace qcm
