#include "MatchingRequest.h"
#include "Battle.h"
#include <format>

MatchingRequest toMatchingRequest(boost::mysql::row_view row, int startingIndex) {
    return MatchingRequest(row[startingIndex].as_uint64(), row[startingIndex + 5].as_datetime(), row[startingIndex + 6].as_datetime(), row[startingIndex + 1].as_uint64(), (MatchingRequestStatus)row[startingIndex + 4].as_int64(), row[startingIndex + 3].is_null() ? 0 : row[startingIndex + 3].as_uint64());
}

DSResult<CreateMatchingRequestResult> createMatchingRequest(DSContext* context, EntityId userId) {

    auto [error, id] = co_await context->executeInsert("insert into matching_requests(dup_flag, user_id, battle_id, status, created_at, updated_at) values (?, ?, NULL, ?, now(3), now(3))", std::make_tuple(
        1, // dup_flag value
        userId,
        (int)MatchingRequestStatus::Wait
    ));

    if (error.code()) {
        if (isUniqueKeyError(error)) {
            co_return std::make_tuple(DSNoError, CreateMatchingRequestResult{0, CreateMatchingRequestError::AlreadyInMatching});
        }

        if (isForeignKeyConstraintError(error)) {
            co_return std::make_tuple(DSNoError, CreateMatchingRequestResult{0, CreateMatchingRequestError::UserNotFound});
        }

        co_return std::make_tuple(error, CreateMatchingRequestResult{});
    }

    co_return std::make_tuple(DSNoError, CreateMatchingRequestResult{id, CreateMatchingRequestError::None});
}

DSResult<uint64_t> cancelMatchingRequest(DSContext* context, EntityId matchingRequestId) {
    auto [error, count] = co_await context->executeUpdate("update matching_requests set dup_flag = NULL, status = ?, updated_at = now(3) where id = ? and dup_flag = 1", std::make_tuple(
        (int)MatchingRequestStatus::Canceled,
        matchingRequestId
    ));

    DS_RETURN_ON_ERROR(error, 0);

    co_return std::make_tuple(DSNoError, count);

}

DSVoidResult getAvailableRequest(DSContext* context, std::chrono::seconds afterSeconds, std::vector<MatchingRequest>& resultContainer) {
    auto [error, result] = co_await context->executeSelect("select * from matching_requests where status = ? and created_at < now(3) - interval ? second", std::make_tuple(
        (int)MatchingRequestStatus::Wait,
        afterSeconds.count()
    ));
    
    DS_RETURN_ON_ERROR(error);

    for (auto row : result.rows()) {
        resultContainer.push_back(toMatchingRequest(row));
    }

    co_return std::make_tuple(DSNoError);
}

DSResult<MatchCompleteResult> matchComplete(DSContext* context, const std::vector<EntityId>& requests, std::string_view settingJson) {
    co_return co_await context->transaction<MatchCompleteResult>([&]() -> DSResult<MatchCompleteResult> {
        std::stringstream sstream;
        auto inQuery = formatInQuery(requests, sstream);

        sstream.str("");
        sstream<<"select * from matching_requests where id in("<<inQuery<<") and status = ? FOR UPDATE";

        uint64_t insertResult = 0, updateResult = 0;

        auto [error, result] = co_await context->executeSelect(sstream.str(), std::make_tuple(
            (int)MatchingRequestStatus::Wait
        ));
        
        DS_RETURN_ON_ERROR(error, MatchCompleteResult{0, MatchCompleteError::None});
        
        if (result.rows().size() != requests.size()) {
            co_return std::make_tuple(DSNoError, MatchCompleteResult{0, MatchCompleteError::RequestNotFound});
        }

        std::tie(error, insertResult) = co_await createBattle(context, settingJson);

        DS_RETURN_ON_ERROR(error, MatchCompleteResult{});

        sstream.str("");
        sstream<<"update matching_requests set dup_flag = NULL, status = ?, battle_id = ?, updated_at = now(3) where id in("<<inQuery<<")";

        std::tie(error, updateResult) = co_await context->executeUpdate(sstream.str(), std::make_tuple(
            (int)MatchingRequestStatus::Completed,
            insertResult
        ));

        co_return std::make_tuple(error, MatchCompleteResult{insertResult, MatchCompleteError::None});
    });
}

DSResult<bool, MatchingRequest> getMatchingRequest(DSContext* context, EntityId matchingRequestId) {
    auto [error, result] = co_await context->executeSelect("select * from matching_requests where id = ?", std::make_tuple(
        matchingRequestId
    ));

    DS_RETURN_ON_ERROR(error, false, MatchingRequest{});

    bool hasValue = result.has_value();
    co_return std::make_tuple(DSNoError, hasValue, hasValue ? toMatchingRequest(result.rows()[0]) : MatchingRequest{});
}
