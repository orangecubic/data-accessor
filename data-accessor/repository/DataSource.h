#pragma once

#include "boost/mysql.hpp"
#undef BOOST_ASIO_HEADER_ONLY
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_generators.hpp"
#include "Error.h"
#include <atomic>
#include <array>
#include <cstring>
#include <sstream>
#include <iostream>

#define BOOST_ASIO_HAS_CO_AWAIT
#include "boost/asio/awaitable.hpp"
#include "boost/asio/use_awaitable.hpp"
#include "boost/asio/as_tuple.hpp"

constexpr auto tuple_awaitable = boost::asio::as_tuple(boost::asio::use_awaitable);
using ConnectionStream = boost::mysql::tcp_ssl_connection;

template <typename ...T>
using DSResult = boost::asio::awaitable<std::tuple<DSError, T...>>;

using DSVoidResult = boost::asio::awaitable<std::tuple<DSError>>;

using EntityId = uint64_t;

#define DEFINE_REQUIRED_DS_VARIABLES(error, diagnostics, results) \
    boost::mysql::error_code error; \
    boost::mysql::diagnostics diagnostics; \
    boost::mysql::results results;

#define DS_RETURN_ON_ERROR(error, ...) \
    if (error.code().failed()) { \
        co_return std::make_tuple(error, ##__VA_ARGS__); \   
    }

std::string formatInQuery(const std::vector<EntityId>& idList, std::stringstream& sstream);

constexpr int MAX_TX_COUNT  = 5;

class DSContext {
private:
    ConnectionStream* mConnection;

    std::atomic_int mNestedTxCount = 0;

    bool mRollbackFlag = false;

    boost::uuids::random_generator mUUIDGenerator;

    DSResult<boost::mysql::statement> preparedStatement(std::string_view query);

    DSVoidResult startTransaction();

    DSVoidResult commitTransaction();

    DSVoidResult rollbackTransaction();

public:
    DSContext(ConnectionStream* stream);
    ConnectionStream* getConnection();

    template <typename T>
    DSResult<T> transaction(std::function<DSResult<T>()> body) {
        std::exception_ptr eptr;
        auto [error] = co_await startTransaction();
        
        DS_RETURN_ON_ERROR(error, T());

        try {
            auto bodyResult = co_await body();
            auto error = std::get<0>(bodyResult);

            if (error.code()) {
                co_await rollbackTransaction();
                co_return bodyResult;
            }

            std::tie(error) = co_await commitTransaction();
            
            if (error.code()) {
                co_await rollbackTransaction();

                std::get<0>(bodyResult) = error;
                co_return bodyResult;
            }

            co_return bodyResult;
        } catch (std::exception_ptr ptr) {
            eptr =  ptr;
        } catch (std::exception& ex) {
            eptr = std::make_exception_ptr(ex);
        } catch (...) {
            std::cout<<"must throw std::exception instance in transaction block"<<std::endl;
        }

        co_await rollbackTransaction();
        std::rethrow_exception(eptr);
    }

    DSVoidResult transaction(std::function<DSVoidResult()> body);

    template <BOOST_MYSQL_FIELD_LIKE_TUPLE Tuple>
    DSResult<boost::mysql::results> executeSelect(std::string_view query, Tuple statementData) {
        
        DEFINE_REQUIRED_DS_VARIABLES(error, diagnostics, results);

        auto [statementError, statement] = co_await preparedStatement(query);

        DS_RETURN_ON_ERROR(statementError, results);

        std::tie(error) = co_await mConnection->async_execute_statement(statement, statementData, results, diagnostics, tuple_awaitable);

        co_return std::make_tuple(DSError(error, diagnostics), results);
    }

    template <BOOST_MYSQL_FIELD_LIKE_TUPLE Tuple>
    DSResult<uint64_t> executeInsert(std::string_view query, Tuple statementData) {
        
        DEFINE_REQUIRED_DS_VARIABLES(error, diagnostics, results);

        auto [statementError, statement] = co_await preparedStatement(query);

        DS_RETURN_ON_ERROR(statementError, 0);

        std::tie(error) = co_await mConnection->async_execute_statement(statement, statementData, results, diagnostics, tuple_awaitable);

        DS_RETURN_ON_ERROR(DSError(error, diagnostics), 0);

        co_return std::make_tuple(DSError(error, diagnostics), results.last_insert_id());
    }

    template <BOOST_MYSQL_FIELD_LIKE_TUPLE Tuple>
    DSResult<uint64_t> executeUpdate(std::string_view query, Tuple statementData) {
        
        DEFINE_REQUIRED_DS_VARIABLES(error, diagnostics, results);

        auto [statementError, statement] = co_await preparedStatement(query);

        DS_RETURN_ON_ERROR(statementError, 0);

        std::tie(error) = co_await mConnection->async_execute_statement(statement, statementData, results, diagnostics, tuple_awaitable);
        
        DS_RETURN_ON_ERROR(DSError(error, diagnostics), 0);

        co_return std::make_tuple(DSError(error, diagnostics), results.affected_rows());
    }

    template <BOOST_MYSQL_FIELD_LIKE_TUPLE Tuple>
    DSResult<uint64_t> executeDelete(std::string_view query, Tuple statementData) {
        return executeUpdate(query, statementData);
    }
};