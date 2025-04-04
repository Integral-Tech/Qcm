#include "qcm_interface/global.h"
#include "qcm_interface/global_p.h"

#include <thread>
#include <QPluginLoader>
#include <QCoreApplication>
#include <QtQuick/QQuickItem>
#include <ctre.hpp>

#include <asio/bind_executor.hpp>

#include "core/log.h"
#include "qcm_interface/path.h"
#include "qcm_interface/ex.h"
#include "qcm_interface/global_static.h"
#include "qcm_interface/action.h"

import ncrequest;

namespace
{

auto static_global(qcm::Global* set = nullptr) -> qcm::Global* {
    static qcm::Global* theGlobal { set };
    _assert_rel_(theGlobal);
    return theGlobal;
}

} // namespace

auto qcm::qexecutor() -> QtExecutor& { return Global::instance()->qexecutor(); }
auto qcm::qexecutor_switch() -> task<void> {
    return asio::post(asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
}
auto qcm::pool_executor() -> asio::thread_pool::executor_type {
    return Global::instance()->pool_executor();
}
auto qcm::strand_executor() -> asio::strand<asio::thread_pool::executor_type> {
    return asio::make_strand(Global::instance()->pool_executor());
}

namespace qcm
{

auto get_pool_size() -> std::size_t {
    return std::clamp<u32>(std::thread::hardware_concurrency(), 4, 8);
}

namespace
{
template<typename T>
bool get_stop_signal(T&& t) {
    if constexpr (std::same_as<std::remove_cvref_t<T>, StopSignal>) {
        return t.val;
    } else {
        return false;
    }
}
template<typename T>
auto hook_stop_signal(T&& t) {
    if constexpr (std::same_as<std::remove_cvref_t<T>, StopSignal>) {
        return StopSignal { true };
    } else {
        return std::forward<T>(t);
    }
}

} // namespace
template<typename R, typename... ARGS>
void GlobalWrapper::connect_from_global(Global* g, R (Global::*g_func)(ARGS...),
                                        R (GlobalWrapper::*gw_func)(ARGS...)) {
    QObject::connect(g, g_func, this, [this, gw_func](ARGS... args) {
        auto stop = (get_stop_signal(args) || ...);
        if (! stop) {
            (this->*gw_func)(hook_stop_signal(args)...);
        }
    });
}
template<typename R, typename... ARGS>
void GlobalWrapper::connect_to_global(Global* g, R (Global::*g_func)(ARGS...),
                                      R (GlobalWrapper::*gw_func)(ARGS...)) {
    QObject::connect(this, gw_func, g, [g, g_func](ARGS... args) {
        auto stop = (get_stop_signal(args) || ...);
        if (! stop) {
            (g->*g_func)(hook_stop_signal(args)...);
        }
    });
}

Global::Private::Private(Global* p)
    : qt_ctx(make_arc<QtExecutionContext>(p, (QEvent::Type)QEvent::registerEventType())),
      pool(get_pool_size()),
      session(ncrequest::Session::make(pool.get_executor())),
      copy_action_comp(nullptr),
      app_state(new state::AppState(p)) {
}
Global::Private::~Private() {}

auto Global::instance() -> Global* { return static_global(); }

Global::Global(): d_ptr(make_up<Private>(this)) {
    C_D(Global);
    DEBUG_LOG("init Global");
    _assert_rel_(static_global(this) == this);

    {
        QCoreApplication::setApplicationName(APP_NAME);
        QCoreApplication::setOrganizationName(APP_NAME);
    }
}
Global::~Global() {
    // delete child before private pointer
    qDeleteAll(children());
    GlobalStatic::instance()->reset();
}

auto Global::info() const -> const model::AppInfo& {
    C_D(const Global);
    return d->info;
}

auto Global::app_state() const -> state::AppState* {
    C_D(const Global);
    return d->app_state;
}

auto Global::qexecutor() -> qt_executor_t& {
    C_D(Global);
    return d->qt_ctx->get_executor();
}
auto Global::pool_executor() -> pool_executor_t {
    C_D(Global);
    return d->pool.executor();
}
auto Global::session() -> rc<ncrequest::Session> {
    C_D(Global);
    return d->session;
}

auto Global::uuid() const -> const QUuid& {
    C_D(const Global);
    return d->uuid;
}

auto Global::get_cache_sql() const -> rc<media_cache::DataBase> {
    C_D(const Global);
    return d->cache_sql;
}

auto Global::get_metadata(const std::filesystem::path& path) const -> Metadata {
    C_D(const Global);
    if (d->metadata_impl) {
        return d->metadata_impl(path);
    }
    return {};
}
auto Global::user_agent() const -> std::string_view {
    return "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/605.1.15 (KHTML, "
           "like Gecko) Version/13.1.2 Safari/605.1.15"sv;
}


auto Global::copy_action_comp() const -> QQmlComponent* {
    C_D(const Global);
    return d->copy_action_comp;
}
void Global::set_copy_action_comp(QQmlComponent* val) {
    C_D(Global);
    if (std::exchange(d->copy_action_comp, val) != val) {
        copyActionCompChanged();
    }
}
void Global::set_uuid(const QUuid& val) {
    C_D(Global);
    if (ycore::cmp_exchange(d->uuid, val)) {
        uuidChanged();
    }
}

void Global::set_cache_sql(rc<media_cache::DataBase> val) {
    C_D(Global);
    d->cache_sql = val;
}

void Global::set_metadata_impl(const MetadataImpl& impl) {
    C_D(Global);
    d->metadata_impl = impl;
}

void Global::join() {
    C_D(Global);
    d->pool.join();
}


GlobalWrapper::GlobalWrapper(): m_g(Global::instance()) {
    connect_from_global(m_g, &Global::copyActionCompChanged, &GlobalWrapper::copyActionCompChanged);
    connect_from_global(m_g, &Global::errorOccurred, &GlobalWrapper::errorOccurred);
    connect_from_global(m_g, &Global::uuidChanged, &GlobalWrapper::uuidChanged);

    connect_to_global(m_g, &Global::errorOccurred, &GlobalWrapper::errorOccurred);

    connect(this, &GlobalWrapper::errorOccurred, this, [this](const QString& error) {
        if (! error.endsWith("Operation aborted.")) {
            QObject* act { nullptr };
            auto     comp = copy_action_comp();
            if (comp) {
                QVariantMap map;
                map.insert("error", error);
                act = comp->createWithInitialProperties(map);
            }
            Action::instance()->toast(error, 0, enums::ToastFlag::TFCloseable, act);
        }
    });
}
GlobalWrapper::~GlobalWrapper() {}
auto GlobalWrapper::datas() -> QQmlListProperty<QObject> { return { this, &m_datas }; }
auto GlobalWrapper::info() -> const model::AppInfo& { return m_g->info(); }
auto GlobalWrapper::copy_action_comp() const -> QQmlComponent* { return m_g->copy_action_comp(); }
auto GlobalWrapper::uuid() const -> QString { return m_g->uuid().toString(QUuid::WithoutBraces); }
auto GlobalWrapper::app_state() const -> state::AppState* { return m_g->app_state(); }
void GlobalWrapper::set_copy_action_comp(QQmlComponent* val) { m_g->set_copy_action_comp(val); }

} // namespace qcm

namespace qcm
{
auto image_provider_url(const QUrl& url, const QString& provider) -> QUrl {
    return QStringLiteral("image://qcm/%1/%2")
        .arg(provider)
        .arg(url.toString().toUtf8().toBase64());
}

auto parse_image_provider_url(const QUrl& url) -> std::tuple<QUrl, QString> {
    constexpr auto ImageProviderRe = ctll::fixed_string { "image://qcm/([^/]+?)/(.*)" };

    auto input = url.toString(QUrl::FullyEncoded).toStdString();
    if (auto match = ctre::match<ImageProviderRe>(input)) {
        return { QString::fromUtf8(
                     QByteArray::fromBase64(QByteArray::fromStdString(match.get<2>().to_string()))),
                 QString::fromStdString(match.get<1>().to_string()) };
    } else {
        return {};
    }
}

auto qml_dyn_count() -> std::atomic<i32>& {
    static std::atomic<i32> n { 0 };
    return n;
}

auto create_item(QQmlEngine* engine, const QJSValue& url_or_comp, const QVariantMap& props,
                 QObject* parent) -> QObject* {
    std::unique_ptr<QQmlComponent, void (*)(QQmlComponent*)> comp { nullptr, nullptr };
    if (auto p = qobject_cast<QQmlComponent*>(url_or_comp.toQObject())) {
        comp = decltype(comp)(p, [](QQmlComponent*) {
        });
    } else if (auto p = url_or_comp.toVariant(); ! p.isNull()) {
        QUrl url;
        if (p.canConvert<QUrl>()) {
            url = p.toUrl();
        } else if (p.canConvert<QString>()) {
            url = p.toString();
        }

        comp = decltype(comp)(new QQmlComponent(engine, url, nullptr), [](QQmlComponent* q) {
            delete q;
        });
    } else {
        log::error("url not valid");
        return nullptr;
    }

    switch (comp->status()) {
    case QQmlComponent::Status::Ready: {
        QObject* obj { nullptr };
        QMetaObject::invokeMethod(comp.get(),
                                  "createObject",
                                  Q_RETURN_ARG(QObject*, obj),
                                  Q_ARG(QObject*, parent),
                                  Q_ARG(const QVariantMap&, props));
        if (obj != nullptr) {
            qml_dyn_count()++;
            auto name = obj->metaObject()->className();
            QObject::connect(obj, &QObject::destroyed, [name](QObject*) {
                DEBUG_LOG("dyn_delete {}", name);
                qml_dyn_count()--;
            });
        } else {
            log::error("{}", comp->errorString());
        }
        return obj;
        break;
    }
    case QQmlComponent::Status::Error: {
        log::error("{}", comp->errorString());
        break;
    }
    case QQmlComponent::Status::Loading: {
        log::error("use before loading");
        break;
    }
    default: break;
    }
    return nullptr;
}

} // namespace qcm

#include <qcm_interface/moc_global.cpp>