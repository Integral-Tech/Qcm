#include "qcm_interface/global.h"
#include "qcm_interface/global_p.h"

#include <thread>
#include <QPluginLoader>
#include <QCoreApplication>
#include <QtQuick/QQuickItem>

#include "core/log.h"
#include "request/session.h"
#include "qcm_interface/path.h"

namespace
{

auto static_global(qcm::Global* set = nullptr) -> qcm::Global* {
    static qcm::Global* theGlobal { set };
    _assert_rel_(theGlobal);
    return theGlobal;
}
} // namespace

namespace qcm
{

auto get_pool_size() -> std::size_t {
    return std::clamp<u32>(std::thread::hardware_concurrency(), 4, 12);
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
    : qt_ex(std::make_shared<QtExecutionContext>(p, (QEvent::Type)QEvent::registerEventType())),
      pool(get_pool_size()),
      session(std::make_shared<request::Session>(pool.get_executor())),
      qsession_empty(new model::Session(p)),
      action(new Action(p)),
      user_model(nullptr),
      copy_action_comp(nullptr) {
    // add empty user to empty session
    qsession_empty->set_user(new model::UserAccount(qsession_empty));
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

    // init after set static
    d->user_model = new UserModel(this);

    connect(user_model(), &UserModel::activeUserChanged, this, [this] {
        auto auser  = user_model()->active_user();
        auto cur_ss = qsession();
        if (auser == nullptr) {
            set_session(nullptr);
        } else if (cur_ss == nullptr || auser != cur_ss->user()) {
            auto ss = new model::Session(this);
            ss->set_user(auser);
            ss->set_valid(true);
            plugin(auser->userId().provider())
                .transform([ss](std::reference_wrapper<QcmPluginInterface> ref) {
                    ss->set_pages(ref.get().main_pages());
                    return nullptr;
                });
            set_session(ss);
        }
    });
}
Global::~Global() { save_user(); }

auto Global::client(std::string_view                       name_view,
                    std::optional<std::function<Client()>> init) -> Client {
    C_D(Global);
    std::unique_lock l { d->mutex };
    auto             name = std::string(name_view);
    if (! d->clients.contains(name_view)) {
        if (init) {
            d->clients.insert({ name, init.value()() });
            return d->clients.at(name);
        } else {
            return {};
        }
    } else {
        return d->clients.at(name);
    }
}
auto Global::info() const -> const model::AppInfo& {
    C_D(const Global);
    return d->info;
}

auto Global::action() const -> Action* {
    C_D(const Global);
    return d->action;
}

auto Global::qexecutor() -> qt_executor_t& {
    C_D(Global);
    return d->qt_ex;
}
auto Global::pool_executor() -> pool_executor_t {
    C_D(Global);
    return d->pool.executor();
}
auto Global::session() -> rc<request::Session> {
    C_D(Global);
    return d->session;
}

auto Global::qsession() const -> model::Session* {
    C_D(const Global);
    if (d->qsession != nullptr) {
        return d->qsession;
    } else {
        return d->qsession_empty;
    }
}
auto Global::uuid() const -> const QUuid& {
    C_D(const Global);
    return d->uuid;
}
auto Global::plugin(QStringView name) const
    -> std::optional<std::reference_wrapper<QcmPluginInterface>> {
    C_D(const Global);
    if (auto it = d->plugins.find(name); it != d->plugins.end()) {
        return *(it->second);
    }
    return std::nullopt;
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

auto Global::user_model() const -> UserModel* {
    C_D(const Global);
    return d->user_model;
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

void Global::load_user() {
    C_D(Global);
    auto user_path = config_path() / "user.json";
    json::parse(user_path).transform([d](const auto& j) {
        json::get_to(*j, *(d->user_model));
    });
}

void Global::save_user() {
    C_D(Global);
    auto user_path = config_path() / "user.json";
    auto j         = json::create();
    json::assign(*j, *(d->user_model));
    auto j_str = convert_from<std::string>(*j);

    auto file = std::fopen(user_path.c_str(), "w+");
    std::fwrite(j_str.data(), j_str.size(), 1, file);
    std::fclose(file);
}

void Global::set_uuid(const QUuid& val) {
    C_D(Global);
    if (ycore::cmp_exchange(d->uuid, val)) {
        uuidChanged();
    }
}
void Global::set_session(model::Session* val) {
    C_D(Global);
    auto old = d->qsession;
    if (ycore::cmp_exchange(d->qsession, val)) {
        sessionChanged();
        if (d->qsession != nullptr) {
            d->qsession->setParent(this);
        }
        if (old != nullptr) {
            old->deleteLater();
        }
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

auto Global::get_client(std::string_view name) -> Client* {
    C_D(Global);
    auto it = d->clients.find(name);
    if (it != d->clients.end()) {
        return &(it->second);
    }
    return nullptr;
}

auto Global::load_plugin(const std::filesystem::path& path) -> bool {
    C_D(Global);
    auto loader = new QPluginLoader(path.c_str(), this);
    if (loader->load()) {
        auto p = qobject_cast<QcmPluginInterface*>(loader->instance());
        if (p) {
            d->plugins.insert_or_assign(p->info().name(), p);
        } else {
            return false;
        }
    } else {
        DEBUG_LOG("{}", loader->errorString());
    }
    return loader->isLoaded();
}

void Global::join() {
    C_D(Global);
    d->pool.join();
}

auto Global::server_url(const model::ItemId& id) -> QVariant {
    C_D(Global);
    const auto& p      = id.provider().toStdString();
    auto        client = get_client(p);
    if (client) {
        return convert_from<QString>(client->api->server_url(*(client->instance), id));
    }
    return {};
}

GlobalWrapper::GlobalWrapper(): m_g(Global::instance()) {
    connect_from_global(m_g, &Global::copyActionCompChanged, &GlobalWrapper::copyActionCompChanged);
    connect_from_global(m_g, &Global::errorOccurred, &GlobalWrapper::errorOccurred);
    connect_from_global(m_g, &Global::uuidChanged, &GlobalWrapper::uuidChanged);
    connect_from_global(m_g, &Global::sessionChanged, &GlobalWrapper::sessionChanged);

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
            m_g->action()->toast(error, 0, enums::ToastFlag::TFCloseable, act);
        }
    });
}
GlobalWrapper::~GlobalWrapper() {}
auto GlobalWrapper::datas() -> QQmlListProperty<QObject> { return { this, &m_datas }; }
auto GlobalWrapper::info() -> const model::AppInfo& { return m_g->info(); }
auto GlobalWrapper::server_url(const model::ItemId& id) -> QVariant { return m_g->server_url(id); }
auto GlobalWrapper::user_model() const -> UserModel* { return m_g->user_model(); }
auto GlobalWrapper::copy_action_comp() const -> QQmlComponent* { return m_g->copy_action_comp(); }
auto GlobalWrapper::uuid() const -> QString { return m_g->uuid().toString(QUuid::WithoutBraces); }
auto GlobalWrapper::qsession() const -> model::Session* { return m_g->qsession(); }
void GlobalWrapper::set_copy_action_comp(QQmlComponent* val) { m_g->set_copy_action_comp(val); }

} // namespace qcm

namespace qcm
{

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
        ERROR_LOG("url not valid");
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
            ERROR_LOG("{}", comp->errorString());
        }
        return obj;
        break;
    }
    case QQmlComponent::Status::Error: {
        ERROR_LOG("{}", comp->errorString());
        break;
    }
    case QQmlComponent::Status::Loading: {
        ERROR_LOG("use before loading");
        break;
    }
    default: break;
    }
    return nullptr;
}
} // namespace qcm