#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct Login {
    std::string username;
    std::string password_md5;
};
} // namespace params

namespace api_model
{
struct Login {
    static Result<Login> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse_no_apierr<Login>(bs);
    }

    i64 code;
};
JSON_DEFINE(Login);

} // namespace api_model

namespace api
{

struct Login {
    using in_type                      = params::Login;
    using out_type                     = api_model::Login;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/login"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["username"]      = input.username;
        p["password"]      = input.password_md5;
        p["rememberLogin"] = "true";
        return p;
    }
    in_type input;
};
static_assert(ApiCP<Login>);

} // namespace api

} // namespace ncm
