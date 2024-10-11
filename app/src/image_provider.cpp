#include "Qcm/image_provider.h"

#include <filesystem>
#include <cstdio>
#include <fstream>

#include <asio/bind_allocator.hpp>
#include <asio/recycling_allocator.hpp>
#include <asio/detached.hpp>

#include <ctre.hpp>
#include <QtCore/QPointer>

#include "qcm_interface/path.h"
#include "qcm_interface/cache_sql.h"
#include "qcm_interface/global.h"

#include "core/expected_helper.h"
#include "request/response.h"
#include "request/session.h"
#include "asio_helper/sync_file.h"
#include "crypto/crypto.h"
#include "Qcm/app.h"

#include "platform/platform.h"

using namespace qcm;

namespace
{
void header_record_db(const request::HttpHeader& h, media_cache::DataBase::Item& db_it) {
    static constexpr auto DigitPattern = ctll::fixed_string { "\\d*" };
    for (auto& f : h.fields) {
        if (helper::case_insensitive_compare(f.name, "content-type") == 0)
            db_it.content_type = f.value;
        else if (helper::case_insensitive_compare(f.name, "content-length") == 0) {
            if (auto whole = ctre::starts_with<DigitPattern>(f.value); whole) {
                db_it.content_length = whole.to_number();
            }
        }
    }
}

} // namespace

namespace qcm
{

QcmAsyncImageResponse::QcmAsyncImageResponse() {}
QcmAsyncImageResponse::~QcmAsyncImageResponse() { plt::malloc_trim_count(0, 10); }

QQuickTextureFactory* QcmAsyncImageResponse::textureFactory() const {
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

class QcmImageProviderInner : std::enable_shared_from_this<QcmImageProviderInner>, NoCopy {
public:
    using executor_type = asio::thread_pool::executor_type;

    executor_type& get_executor() { return m_ex; }

    QcmImageProviderInner()
        : m_ex(Global::instance()->pool_executor()),
          m_session(Global::instance()->session()),
          m_cache_sql(Global::instance()->get_cache_sql()) {}

    asio::awaitable<request::HttpHeader> dl_image(const request::Request& req,
                                                  std::filesystem::path   p) {
        helper::SyncFile file { std::fstream(p, std::ios::out | std::ios::binary) };
        file.handle().exceptions(std::ios_base::failbit | std::ios_base::badbit);

        auto rsp_http = (co_await m_session->get(req)).value();

        co_await rsp_http->read_to_stream(file);

        file.handle().close();
        co_return rsp_http->header();
    }

    asio::awaitable<void> cache_new_image(const request::Request& req, std::string_view key,
                                          std::filesystem::path cache_file, QSize req_size) {
        media_cache::DataBase::Item db_it;
        db_it.key = key;

        auto file_dl = cache_file;
        file_dl.replace_extension(fmt::format("dl{}x{}", req_size.width(), req_size.height()));

        auto header = co_await dl_image(req, file_dl);
        std::filesystem::rename(file_dl, cache_file);
        header_record_db(header, db_it);
        co_await m_cache_sql->insert(db_it);
    }

    asio::awaitable<QImage> request_image(const request::Request& req,
                                          std::filesystem::path cache_path, QSize req_size) {
        std::string key = cache_path.filename().native();
        asio::co_spawn(m_cache_sql->get_executor(), m_cache_sql->get(key), asio::detached);
        if (! std::filesystem::exists(cache_path)) {
            co_await cache_new_image(req, key, cache_path, req_size);
        }
        auto img = QImage(cache_path.c_str());
        if (req_size.isValid() && ! img.isNull()) {
            img = img.scaled(req_size,
                             Qt::AspectRatioMode::KeepAspectRatioByExpanding,
                             Qt::TransformationMode::SmoothTransformation);
        }
        co_return img;
    }

    asio::awaitable<void> handle_request(helper::QWatcher<QcmAsyncImageResponse> rsp,
                                         request::Request req, std::filesystem::path cache_path,
                                         QSize req_size) {
        auto img = co_await request_image(req, cache_path, req_size);
        QcmImageProviderInner::handle_res(rsp, img);
        co_return;
    }

    static void handle_res(QcmAsyncImageResponse* rsp, nstd::expected<QImage, QString> res) {
        if (rsp == nullptr) return;

        if (res.has_value()) {
            QMetaObject::invokeMethod(
                rsp, "handle", Qt::QueuedConnection, Q_ARG(QImage, res.value()));
        } else {
            QMetaObject::invokeMethod(
                rsp, "handle_error", Qt::QueuedConnection, Q_ARG(QString, res.error()));
        }
    };

private:
    executor_type             m_ex;
    rc<request::Session>      m_session;
    rc<media_cache::DataBase> m_cache_sql;
};
} // namespace qcm

QcmImageProvider::QcmImageProvider()
    : QQuickAsyncImageProvider(), m_inner(std::make_shared<QcmImageProviderInner>()) {}
QcmImageProvider::~QcmImageProvider() {}

QQuickImageResponse* QcmImageProvider::requestImageResponse(const QString& id,
                                                            const QSize&   requestedSize) {
    QcmAsyncImageResponse* rsp = new QcmAsyncImageResponse();

    do {
        if (id.isEmpty()) break;

        auto [url, provider] = parse_image_provider_url(QString("image://qcm/%1").arg(id));
        if (url.isEmpty()) break;

        auto             rsp_guard = helper::QWatcher(rsp);
        request::Request req;
        if (auto c = Global::instance()->qsession()->client();
            c && c->api->provider == provider.toStdString()) {
            if (! c->api->make_request(
                    *(c->instance), req, url, Client::ReqInfoImg { requestedSize })) {
                ERROR_LOG("make image req failed");
                break;
            }
        } else {
            ERROR_LOG("client not found");
            break;
        }
        std::filesystem::path file_path;
        if (auto opt = gen_image_cache_entry(provider, url, requestedSize)) {
            file_path = opt.value();
        } else {
            ERROR_LOG("gen cache entry failed");
            break;
        }

        auto alloc = asio::recycling_allocator<void>();
        auto ex    = asio::make_strand(m_inner->get_executor());
        asio::co_spawn(
            ex,
            rsp->wdog().watch(
                ex,
                [rsp_guard, requestedSize, req, file_path, inner = m_inner]()
                    -> asio::awaitable<void> {
                    co_await inner->handle_request(rsp_guard, req, file_path, requestedSize);
                    co_return;
                },
                alloc),
            asio::bind_allocator(alloc, [rsp_guard, file_path, id, url](std::exception_ptr p) {
                if (p) {
                    try {
                        if (std::filesystem::exists(file_path)) {
                            std::filesystem::remove(file_path);
                        }
                        std::rethrow_exception(p);
                    } catch (const std::exception& e) {
                        QcmImageProviderInner::handle_res(
                            rsp_guard,
                            nstd::unexpected(convert_from<QString>(fmt::format(R"(
QcmImageProvider
    id: {}
    url: {}
    error: {})",
                                                                               id,
                                                                               url.toString(),
                                                                               e.what()))));
                    }
                }
            }));
        return rsp;
    } while (false);
    rsp->finished();
    return rsp;
}