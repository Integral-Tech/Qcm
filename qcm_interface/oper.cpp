#include "qcm_interface/oper/album_oper.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/oper/artist_oper.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/oper/song_oper.h"
#include "qcm_interface/model/song.h"

namespace qcm::oper
{

namespace detail
{
template<typename T>
auto create_list(usize num) -> OperList<T> {
    auto holder = typename OperList<T>::Holder(new std::vector<T>(num), [](voidp p) {
        delete static_cast<std::vector<T>*>(p);
    });
    auto vec    = static_cast<std::vector<T>*>(holder.get());
    auto span   = std::span { *vec };
    return { std::move(holder),
             [](voidp h) {
                 return static_cast<std::vector<T>*>(h)->data();
             },
             [](voidp h) {
                 return static_cast<std::vector<T>*>(h)->size();
             },
             [](T* d, usize s) -> T* {
                 return d + s;
             },
             [](voidp handle) -> T* {
                 return &(static_cast<std::vector<T>*>(handle)->emplace_back());
             } };
}
} // namespace detail

#define X(T)                                                                \
    template<>                                                              \
    QCM_INTERFACE_API auto Oper<T>::create_list(usize num) -> OperList<T> { \
        return detail::create_list<T>(num);                                 \
    }

X(model::Album)
X(model::Artist)
X(model::Song)

IMPL_OPER_PROPERTY(AlbumOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(AlbumOper, QString, name, name)
IMPL_OPER_PROPERTY(AlbumOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(AlbumOper, QDateTime, publishTime, publishTime)
IMPL_OPER_PROPERTY(AlbumOper, int, trackCount, trackCount)
IMPL_OPER_PROPERTY(AlbumOper, QString, description, description)
IMPL_OPER_PROPERTY(AlbumOper, QString, company, company)
IMPL_OPER_PROPERTY(AlbumOper, QString, type, type)

IMPL_OPER_PROPERTY(ArtistOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(ArtistOper, QString, name, name)
IMPL_OPER_PROPERTY(ArtistOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(ArtistOper, QString, description, description)
IMPL_OPER_PROPERTY(ArtistOper, qint32, albumCount, albumCount)
IMPL_OPER_PROPERTY(ArtistOper, qint32, musicCount, musicCount)
IMPL_OPER_PROPERTY(ArtistOper, std::vector<QString>, alias, alias)

IMPL_OPER_PROPERTY(SongOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(SongOper, QString, name, name)
IMPL_OPER_PROPERTY(SongOper, ItemId, albumId, albumId)
IMPL_OPER_PROPERTY(SongOper, qint32, trackNumber, trackNumber)
IMPL_OPER_PROPERTY(SongOper, QDateTime, duration, duration)
IMPL_OPER_PROPERTY(SongOper, bool, canPlay, canPlay)
IMPL_OPER_PROPERTY(SongOper, QString, coverUrl, coverUrl)
IMPL_OPER_PROPERTY(SongOper, QStringList, tags, tags)
IMPL_OPER_PROPERTY(SongOper, qreal, popularity, popularity)
IMPL_OPER_PROPERTY(SongOper, QVariant, source, source)
IMPL_OPER_PROPERTY(SongOper, ItemId, sourceId, sourceId)
} // namespace qcm::oper