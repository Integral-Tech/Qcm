#pragma once
#include <deque>
#include <QTimer>
#include "qcm_interface/sync_api.h"
#include "qcm_interface/async.h"
#include "qcm_interface/action.h"
#include "qcm_interface/notifier.h"

namespace qcm::query
{

class QCM_INTERFACE_API QueryBase : public QAsyncResult {
    Q_OBJECT

    Q_PROPERTY(bool delay READ delay WRITE setDelay NOTIFY delayChanged FINAL)
public:
    QueryBase(QObject* parent = nullptr);
    ~QueryBase();

    static auto create(std::string_view) -> QueryBase*;

    Q_SLOT void   request_reload();
    auto          delay() const -> bool;
    void          setDelay(bool v);
    Q_SIGNAL void delayChanged();

protected:
    template<typename T, typename R, typename... ARGS>
    void connect_requet_reload(R (T::*f)(ARGS...), T* obj = nullptr) {
        connect(obj == nullptr ? static_cast<T*>(this) : obj, f, this, &QueryBase::request_reload);
    }

private:
    QTimer m_timer;
    bool   m_delay;

    std::deque<std::function<task<void>()>> m_queue;
};

class QCM_INTERFACE_API QueryListBase : public QueryBase {
    Q_OBJECT
    Q_PROPERTY(qint32 offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(qint32 limit READ limit WRITE setLimit NOTIFY limitChanged)
public:
    QueryListBase(QObject* parent = nullptr);
    ~QueryListBase();
    auto          offset() const -> qint32;
    auto          limit() const -> qint32;
    Q_SLOT void   setOffset(qint32 v);
    Q_SLOT void   setLimit(qint32 v);
    Q_SIGNAL void offsetChanged();
    Q_SIGNAL void limitChanged();

private:
    qint32 m_offset;
    qint32 m_limit;
};

template<typename T, typename Base = QueryBase>
class Query : public QAsyncResultT<T, Base> {
public:
    Query(QObject* parent = nullptr)
        : QAsyncResultT<T, Base>(parent), m_last(QDateTime::fromSecsSinceEpoch(0)) {}
    ~Query() {}

    auto last() const -> const QDateTime& { return m_last; }
    void setLast(const QDateTime& t, const QDateTime& last = QDateTime::currentDateTime()) {
        m_last = std::min<QDateTime>(t, last);
    }
    void resetLast() { m_last = QDateTime::fromSecsSinceEpoch(0); }
    auto record() {
        auto old = m_last;
        m_last   = QDateTime::currentDateTimeUtc();
        return old;
    }

    QDateTime m_last;
};

template<typename T>
using QueryList = Query<T, QueryListBase>;

} // namespace qcm::query