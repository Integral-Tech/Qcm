#include "qt_execution_context.h"

QtExecutionContext::QtExecutionContext(QThread* thread, QEvent::Type t)
    : m_target(new QtExecutionEventRunner(t)) {
    // move to target thread
    if (thread != m_target->thread()) {
        m_target->moveToThread(thread);
    }
}
QtExecutionContext::QtExecutionContext(QObject* target, QEvent::Type t)
    : QtExecutionContext(target->thread(), t) {}

QtExecutionContext::~QtExecutionContext() { m_target->deleteLater(); }

auto QtExecutionContext::event_type() const -> QEvent::Type { return m_target->event_type; }

auto QtExecutionEventRunner::event(QEvent* event) -> bool {
    if (event->type() == event_type) {
        auto p = static_cast<QtExecEvent*>(event);
        p->accept();
        p->invoke();
        return true;
    } else {
        return QObject::event(event);
    }
}