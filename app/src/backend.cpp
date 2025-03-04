#include "Qcm/backend.h"

#include <filesystem>
#include <QJsonDocument>
#include <QJsonObject>

#include <asio/posix/stream_descriptor.hpp>

#include "core/log.h"
#include "core/qstr_helper.h"
#include "Qcm/message/message.qpb.h"

import ncrequest.event;
import rstd.rc;

namespace qcm
{
Backend::Backend()
    : m_thread(make_box<QThread>(new QThread())),
      m_context(
          make_box<QtExecutionContext>(m_thread.get(), (QEvent::Type)QEvent::registerEventType())),
      m_process(new QProcess()),
      m_client(make_box<ncrequest::WebSocketClient>(
          ncrequest::event::create<asio::posix::basic_stream_descriptor>(
              m_context->get_executor()))) {
    m_process->setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedErrorChannel);
    m_client->set_on_error_callback([](std::string_view err) {
        ERROR_LOG("{}", err);
    });
    m_client->set_on_message_callback([](std::span<const std::byte> bytes, bool last) {
    });
    // start thread
    {
        bool ok = m_process->moveToThread(m_thread.get());
        _assert_(ok);
        m_thread->start();
    }
    // connect signal
    {
        struct State {
            bool port_readed { false };
        };
        auto state = rstd::rc::make_rc<State>();
        connect(m_process, &QProcess::readyReadStandardOutput, m_process, [this, state] mutable {
            if (state->port_readed) return;

            m_process->setReadChannel(QProcess::ProcessChannel::StandardOutput);
            if (m_process->canReadLine()) {
                state->port_readed = true;
                auto line          = m_process->readLine();
                auto doc           = QJsonDocument::fromJson(line);
                if (auto jport = doc.object().value("port"); ! jport.isUndefined()) {
                    auto port = jport.toVariant().value<i32>();
                    INFO_LOG("backend port: {}", port);
                    Q_EMIT this->started(port);
                } else {
                    ERROR_LOG("read port from backend failed");
                }
            }
        });
    }
}

Backend::~Backend() {
    m_thread->quit();
    m_thread->wait();
}

auto Backend::start(QStringView exe_, QStringView data_dir_) -> bool {
    auto exe      = exe_.toString();
    auto data_dir = data_dir_.toString();
    {
        std::error_code ec;
        auto            path = std::filesystem::path(exe.toStdString());
        if (! std::filesystem::exists(path, ec)) {
            ERROR_LOG("{}", ec.message());
            return false;
        }
    }

    m_context->post([this, exe, data_dir] {
        INFO_LOG("starting backend: {}", exe);
        m_process->start(exe, { u"--data"_s, data_dir });
    });
    return true;
}

void Backend::on_started(i32 port) { m_client->connect(std::format("127.0.0.1:{}", port)); }
} // namespace qcm

#include <Qcm/moc_backend.cpp>