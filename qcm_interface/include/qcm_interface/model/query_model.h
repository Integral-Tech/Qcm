#pragma once

#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/model/song.h"
#include "qcm_interface/model/mix.h"
#include "qcm_interface/model/radio.h"
#include "qcm_interface/model/program.h"
#include "core/qstr_helper.h"

namespace qcm::query
{

struct Artist : model::Artist {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
};
struct Album : model::Album {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(std::vector<model::ArtistRefer>, artists, artists)
    QCM_INTERFACE_API static auto sql() -> const model::ModelSql&;
};

struct Song : model::Song {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_VALUE_TYPE(t_song)
public:
    GADGET_PROPERTY_DEF(model::AlbumRefer, album, album)
    GADGET_PROPERTY_DEF(std::vector<model::ArtistRefer>, artists, artists)

    QCM_INTERFACE_API static auto sql() -> const model::ModelSql&;
};

struct Radio : model::Radio {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
};

struct Program : model::Program {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
};
} // namespace qcm::query