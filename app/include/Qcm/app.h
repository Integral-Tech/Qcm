#pragma once

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <iostream>
#include <vector>

#include <asio/thread_pool.hpp>
#include <asio/steady_timer.hpp>

#include "core/core.h"
#include "core/str_helper.h"
#include "core/log.h"
#include "asio_qt/qt_executor.h"

#include "ncm/client.h"

#include "Qcm/model/user_account.h"
#include "mpris/mpris.h"
#include "mpris/mediaplayer2.h"

namespace qcm
{
class App : public QObject {
    Q_OBJECT

    Q_PROPERTY(mpris::MediaPlayer2* mpris READ mpris)
public:
    App();
    virtual ~App();

    void init(QQmlApplicationEngine*);

    static App* instance();

    ncm::Client                      ncm_client() const;
    asio::any_io_executor            get_executor() { return m_qt_ex; }
    asio::thread_pool::executor_type get_pool_executor() { return m_pool.get_executor(); }

    mpris::MediaPlayer2* mpris() const { return m_mpris->mediaplayer2(); };

    Q_INVOKABLE QString md5(QString) const;
    Q_INVOKABLE model::ArtistId artistId(QString id) const;
    Q_INVOKABLE model::AlbumId albumId(QString id) const;
    Q_INVOKABLE QUrl getImageCache(QString url, QSize reqSize) const;

public slots:
    void loginPost(model::UserAccount*);

private:
    void load_session();
    void save_session();
    // up<QQmlApplicationEngine> m_qml_engine;
    QtExecutor        m_qt_ex;
    asio::thread_pool m_pool;

    rc<request::Session> m_session;
    mutable ncm::Client  m_client;
    up<mpris::Mpris>     m_mpris;
};
} // namespace qcm
