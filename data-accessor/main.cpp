#include "service/User.h"
#include "service/MatchingRequest.h"
#include "service/Battle.h"
#include "boost/asio/as_tuple.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ssl/context.hpp"
#include "boost/asio/awaitable.hpp"
#include "boost/asio/co_spawn.hpp"
#include "boost/asio/spawn.hpp"
#include "boost/asio/use_awaitable.hpp"
#include "gflags/gflags.h"
#include <thread>

DEFINE_string(user, "root", "db user name");
DEFINE_string(password, "root", "db user password");
DEFINE_string(database, "mydb", "database name");

DEFINE_string(host, "127.0.0.1", "database host");
DEFINE_string(port, "3306", "port for database");

using namespace std;

boost::asio::awaitable<void> fetch(boost::mysql::tcp_ssl_connection& conn, boost::asio::ip::tcp::resolver& resolver, const boost::mysql::handshake_params& params, const char* hostname) {

    boost::mysql::error_code ec;
    boost::mysql::diagnostics diag;

    auto endpoints = co_await resolver.async_resolve(
        hostname,
        FLAGS_port,
        boost::asio::use_awaitable
    );

    // Connect to server
    std::tie(ec) = co_await conn.async_connect(*endpoints.begin(), params, diag, tuple_awaitable);
    boost::mysql::throw_on_error(ec, diag);

    DSContext context(&conn);


    auto [error] = co_await context.transaction([&]() -> DSVoidResult {

        auto [error1, createUserResult1] = co_await createUser(&context, "user1", "password");

        DS_RETURN_ON_ERROR(error1);

        if (createUserResult1.error != CreateUserError::None) {
            cout<<"failed to create user1, code: "<<(int)createUserResult1.error<<endl;
            throw std::exception();
        }

        auto [error2, createUserResult2] = co_await createUser(&context, "user2", "password");

        DS_RETURN_ON_ERROR(error2);

        if (createUserResult2.error != CreateUserError::None) {
            cout<<"failed to create user2, code: "<<(int)createUserResult2.error<<endl;
            throw std::exception();
        }
        
        cout<<"complete to create users: "<<createUserResult1.id<<", "<<createUserResult2.id<<endl;

        CreateMatchingRequestResult result1, result2;
        std::tie(error1, result1) = co_await createMatchingRequest(&context, createUserResult1.id);
        std::tie(error2, result2) = co_await createMatchingRequest(&context, createUserResult2.id);

        DS_RETURN_ON_ERROR(error1);
        DS_RETURN_ON_ERROR(error2);

        if(result1.error != CreateMatchingRequestError::None) {
            cout<<"failed to create user1 matching request, code: "<<(int)result1.error<<endl;
            throw std::exception();
        }

        if(result2.error != CreateMatchingRequestError::None) {
            cout<<"failed to create user2 matching request, code: "<<(int)result2.error<<endl;
            throw std::exception();
        }

        cout<<"complete to create match requests: "<<result1.id<<", "<<result2.id<<endl;

        MatchCompleteResult matchResult;
        std::tie(error1, matchResult) = co_await matchComplete(&context, {result1.id, result2.id}, "{}");

        DS_RETURN_ON_ERROR(error1);

        if (matchResult.error != MatchCompleteError::None) {
            cout<<"failed to match users, code: "<<endl;
            throw std::exception();
        }

        cout<<"complete match with battle: "<<matchResult.battleId<<endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));

        FinishBattleError finishBattleError;
        std::tie(error1, finishBattleError) = co_await finishBattle(&context, matchResult.battleId, createUserResult1.id);

        DS_RETURN_ON_ERROR(error1);

        if (finishBattleError != FinishBattleError::None) {
            cout<<"failed to finish battle, code: "<<(int)finishBattleError<<endl;
            throw std::exception();
        }

        cout<<"battle finished"<<endl;

        std::vector<BattleResult> results;

        std::tie(error1) = co_await getBattleHistories(&context, createUserResult1.id, results);

        DS_RETURN_ON_ERROR(error1);

        for(auto result : results) {
            cout<<"user: "<<result.userId << " "<<(result.isWinner ? "is win" : "is lose")<<" in battle "<<result.battleId<<endl;
        }

    });

    boost::mysql::throw_on_error(error.code(), error.get_diagnostics());

    co_return;
}

int main() {

    boost::asio::io_context ctx;
    boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tls_client);
    boost::mysql::tcp_ssl_connection conn(ctx, ssl_ctx);

    // Connection parameters
    boost::mysql::handshake_params params(
        FLAGS_user,                // username
        FLAGS_password,                // password
        FLAGS_database  // database to use; leave empty or omit the parameter for no
                                // database
    );

    // Resolver for hostname resolution
    boost::asio::ip::tcp::resolver resolver(ctx.get_executor());

    // The entry point. We pass in a function returning
    // boost::asio::awaitable<void>, as required.
    boost::asio::co_spawn(
        ctx.get_executor(),
        [&conn, &resolver, params] {
            return fetch(conn, resolver, params, FLAGS_host.c_str());
        },
        // If any exception is thrown in the coroutine body, rethrow it.
        [](std::exception_ptr ptr) {
            if (ptr)
            {
                std::rethrow_exception(ptr);
            }
        }
    );

    ctx.run();
}