#pragma once

#include <filesystem>
#include <QtCore/QAbstractListModel>
#include <QtQml/QQmlEngine>

#include "qcm_interface/api.h"
#include "error/error.h"
#include "service_qml_ncm/api.h"

namespace ncm::qml
{

class CloudUploadModel : public QAbstractListModel {
    Q_OBJECT
public:
    CloudUploadModel(QObject* parent = nullptr);
    ~CloudUploadModel();

    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct Item {};
    std::vector<Item> m_items;
};

class CloudUploadApi : public qcm::QAsyncResultT<CloudUploadModel, NcmApiQueryBase> {
    Q_OBJECT
    QML_ELEMENT
public:
    CloudUploadApi(QObject* parent = nullptr);
    ~CloudUploadApi();

    void reload() override;

    Q_SLOT void upload(const QUrl&);

private:
    auto upload_impl(std::filesystem::path)
        -> asio::awaitable<nstd::expected<std::monostate, error::Error>>;
};
} // namespace ncm::qml