#pragma once

#include <QPointer>

#include <asio/post.hpp>

#include "core/core.h"
#include "asio_qt/qt_executor.h"

namespace helper
{
template<typename T>
class QWatcher {
public:
    QWatcher(): m_ptr(nullptr) {}
    QWatcher(T* t): QWatcher() {
        if (t != nullptr) {
            m_ptr = rc<helper>(new helper(t), [](helper* h) {
                h->deleteLater();
            });
            m_ptr->moveToThread(t->thread());
        }
    }
    // QWatcher(QPointer<T> t): m_ptr(make_rc<QPointer<T>>(t)) {}

    ~QWatcher() {}

    QWatcher(const QWatcher&)            = default;
    QWatcher& operator=(const QWatcher&) = default;
    QWatcher(QWatcher&&)                 = default;
    QWatcher& operator=(QWatcher&&)      = default;

    T* operator->() const { return get(); }
    operator bool() const { return m_ptr && m_ptr->pointer; }
    auto get() const -> T* {
        if (m_ptr) {
            return m_ptr->pointer.load();
        }
        return nullptr;
    }

    void take_owner() const {
        if (*this) {
            this->get()->setParent(this->m_ptr.get());
        }
    }

    operator T*() const { return get(); }

    auto thread() const -> QThread* {
        if (*this) {
            return m_ptr->thread();
        };
        return nullptr;
    }

private:
    struct helper : QObject {
        std::atomic<T*> pointer;
        helper(T* p): pointer(p) {
            connect(
                p,
                &QObject::destroyed,
                this,
                [this] {
                    pointer = nullptr;
                },
                Qt::DirectConnection);
        }
    };

    rc<helper> m_ptr;
};
} // namespace helper