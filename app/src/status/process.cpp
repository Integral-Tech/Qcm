#include "Qcm/status/process.hpp"
#include "asio_helper/basic.h"
#include "qcm_interface/ex.h"
#include "qcm_interface/global.h"
#include "Qcm/app.h"
#include "Qcm/status/provider_status.hpp"

void qcm::process_msg(msg::QcmMessage&& msg) {
    using M = msg::MessageTypeGadget::MessageType;
    switch (msg.type()) {
    case M::PROVIDER_META_STATUS_MSG: {
        asio::post(qcm::qexecutor(), [msg = std::move(msg)] {
            auto p = App::instance()->provider_meta_status();
            p->sync(msg.providerMetaStatusMsg().metas());
        });
        break;
    }
    case M::PROVIDER_STATUS_MSG: {
        asio::post(qcm::qexecutor(), [msg = std::move(msg)] {
            Global::instance()->app_state()->set_state(state::AppState::Session {});
            auto p = App::instance()->provider_status();
            p->sync(msg.providerStatusMsg().statuses());
        });
        break;
    }
    default: {
    }
    }
}