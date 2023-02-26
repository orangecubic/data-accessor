#include "DataSource.h"

DSContext::DSContext(ConnectionStream* stream) : mConnection(stream) {}

DSVoidResult DSContext::transaction(std::function<DSVoidResult()> body) {
    std::exception_ptr eptr;
    auto [error] = co_await startTransaction();

    DS_RETURN_ON_ERROR(error);

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

DSResult<boost::mysql::statement> DSContext::preparedStatement(std::string_view query) {

    DEFINE_REQUIRED_DS_VARIABLES(error, diagnostics, results);

    boost::mysql::statement statement;

    std::tie(error, statement) = co_await mConnection->async_prepare_statement(query, diagnostics, tuple_awaitable);

    co_return std::make_tuple(DSError(error, diagnostics), statement);
}

ConnectionStream* DSContext::getConnection() {
    return mConnection;
}

DSVoidResult DSContext::startTransaction() {
    DEFINE_REQUIRED_DS_VARIABLES(error, diagnostics, results);

    int txNumber = ++mNestedTxCount;

    if (txNumber == 1) {
        std::tie(error) = co_await mConnection->async_query("start transaction", results, diagnostics, tuple_awaitable);
    }

    co_return std::make_tuple(DSError(error, diagnostics));
}

DSVoidResult DSContext::commitTransaction() {
    int txNumber = --mNestedTxCount;

    assert(txNumber >= 0);

    DEFINE_REQUIRED_DS_VARIABLES(error, diagnostics, results);

    if (txNumber == 0) {
        bool flag = mRollbackFlag;
        mRollbackFlag = false;

        if (flag) {
            co_return co_await rollbackTransaction();
        }

        std::tie(error) = co_await mConnection->async_query("commit", results, diagnostics, tuple_awaitable);
    }
    
    co_return std::make_tuple(DSError(error, diagnostics));
}

DSVoidResult DSContext::rollbackTransaction() {
    int txNumber = --mNestedTxCount;

    assert(txNumber >= 0);

    DEFINE_REQUIRED_DS_VARIABLES(error, diagnostics, results);

    if (txNumber == 0) {
        mRollbackFlag = false;
        std::tie(error) = co_await mConnection->async_query("rollback", results, diagnostics, tuple_awaitable);    
    } else {
        mRollbackFlag = true;
    }

    co_return std::make_tuple(DSError(error, diagnostics));
}

std::string formatInQuery(const std::vector<EntityId>& idList, std::stringstream& sstream) {
    sstream<<"";

    int index = 0;
    for (auto id : idList) {
        sstream<<id;
        if (++index != idList.size()) {
            sstream<<",";
        }
    }
    sstream<<"";

    return sstream.str();
};