#pragma once

#include <asio/ip/tcp.hpp>
#include <asio/awaitable.hpp>

#include "request/session.h"
#include "core/core.h"

#include "get_request.h"
#include "media_cache/database.h"

namespace media_cache
{
class Server;
class Connection {
public:
    Connection(asio::ip::tcp::socket, rc<DataBase>);
    ~Connection();

    auto run(rc<request::Session>, std::filesystem::path cache_dir) -> asio::awaitable<void>;

    void stop();
    auto get_req() -> const std::optional<GetRequest>&;

private:
    auto http_source(std::filesystem::path, rc<request::Session>) -> asio::awaitable<void>;
    auto file_source(std::filesystem::path, std::pmr::polymorphic_allocator<byte>)
        -> asio::awaitable<void>;

    auto send_http_header(DataBase::Item& db_item, const request::Header& header,
                          const request::Request& proxy_req) -> asio::awaitable<void>;
    auto send_file_header(std::optional<DataBase::Item>, i64 offset, usize size)
        -> asio::awaitable<void>;

    auto check_cache(std::string key, std::filesystem::path) -> asio::awaitable<bool>;

    asio::ip::tcp::socket     m_s;
    rc<DataBase>              m_db;
    std::optional<GetRequest> m_req;
};

} // namespace media_cache
