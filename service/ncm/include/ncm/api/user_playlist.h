#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct UserPlaylist {
    model::UserId uid;
    i64           offset { 0 };
    i64           limit { 25 };
    bool          includeVideo { true };
};
} // namespace params

namespace model
{
struct UserPlaylistItem {
    // subscribers	[]
    bool subscribed;
    // creator	{…}
    // artists	null
    // tracks	null
    // updateFrequency	null
    // backgroundCoverId	0
    // backgroundCoverUrl	null
    // titleImage	0
    // titleImageUrl	null
    // englishTitle	null
    // opRecommend	false
    // recommendInfo	null
    i64 subscribedCount;
    // cloudTrackCount	1
    model::UserId userId;
    // totalDuration	0
    // coverImgId	109951167805071570
    // privacy	0
    // trackUpdateTime	1678022963095
    i64         trackCount;
    Time        updateTime;
    std::string commentThreadId;
    std::string coverImgUrl;
    i64         specialType;
    // anonimous	false
    Time createTime;
    // highQuality	false
    // newImported	false
    // trackNumberUpdateTime	1678018138230
    i64 playCount;
    // adType	0
    std::optional<std::string> description;
    std::vector<std::string>   tags;
    // ordered	true
    // status	0
    std::string       name;
    model::PlaylistId id;
    // coverImgId_str	"109951167805071571"
    // sharedUsers	null
    // shareStatus	null
    // copied	false
};
// special type
// 5 -> like playlist
// 100 -> offical playlist
// 300 -> ?
} // namespace model

namespace api_model
{

struct UserPlaylist {
    static Result<UserPlaylist> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<UserPlaylist>(bs);
    }

    bool                                 more;
    std::vector<model::UserPlaylistItem> playlist;
};
JSON_DEFINE(UserPlaylist);

} // namespace api_model

namespace api
{

struct UserPlaylist {
    using in_type                      = params::UserPlaylist;
    using out_type                     = api_model::UserPlaylist;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/user/playlist"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        convert(p["uid"], input.uid.as_str());
        convert(p["offset"], input.offset);
        convert(p["limit"], input.limit);
        convert(p["includeVideo"], input.includeVideo);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<UserPlaylist>);

} // namespace api

} // namespace ncm
/*
subscribers	[]
subscribed	false
creator	{…}
artists	null
tracks	null
updateFrequency	null
backgroundCoverId	0
backgroundCoverUrl	null
titleImage	0
titleImageUrl	null
englishTitle	null
opRecommend	false
recommendInfo	null
subscribedCount	4
cloudTrackCount	1
userId	32953014
totalDuration	0
coverImgId	109951167805071570
privacy	0
trackUpdateTime	1678022963095
trackCount	1032
updateTime	1678018138230
commentThreadId	"A_PL_0_24381616"
coverImgUrl	"https://p1.music.126.net/a1rL4eeEnJO0F-B26zxVMw==/109951167805071571.jpg"
specialType	5
anonimous	false
createTime	1407747901072
highQuality	false
newImported	false
trackNumberUpdateTime	1678018138230
playCount	18151
adType	0
description	"描述"
tags
0	"学习"
ordered	true
status	0
name	"binaryify喜欢的音乐"
id	24381616
coverImgId_str	"109951167805071571"
sharedUsers	null
shareStatus	null
copied	false
*/
