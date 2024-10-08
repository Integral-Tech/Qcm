#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{

struct Comments {
    enum class Type
    {
        Album = 0,
        Playlist,
        Song,
        Program
    };
    Type             type { 0 };
    model::CommentId id;
    i32              offset { 0 };
    i32              limit { 20 };
    model::Time      before;
};

} // namespace params

namespace api_model
{

struct Comments {
    static Result<Comments> parse(std::span<const byte> bs, const params::Comments&) {
        return api_model::parse<Comments>(bs);
    }
    std::vector<model::Comment>                topComments;
    std::optional<std::vector<model::Comment>> hotComments;
    std::vector<model::Comment>                comments;
    i64                                        total;
    std::optional<bool>                        moreHot;
    bool                                       more;
};
JSON_DEFINE(Comments);
} // namespace api_model

namespace api
{

struct Comments {
    using in_type                      = params::Comments;
    using out_type                     = api_model::Comments;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;
    constexpr static std::array prefixs { "R_AL_3_", "A_PL_0_", "R_SO_4_", "A_DJ_1_" };

    std::string path() const {
        return fmt::format("/v1/resource/comments/{}{}",
                           prefixs[(int)input.type % prefixs.size()],
                           input.id.as_str());
    };
    UrlParams query() const { return {}; }
    Params    body() const {
        Params p;
        p["rid"] = input.id.as_str();
        convert(p["limit"], input.limit);
        convert(p["offset"], input.offset);
        convert(p["beforeTime"], input.before.milliseconds);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<Comments>);

} // namespace api

} // namespace ncm
