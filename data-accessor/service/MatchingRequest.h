#pragma once

#include "../repository/IDataEntity.h"
#include <vector>

enum class MatchingRequestStatus : uint8_t {
    Wait = 1,
    Canceled = 2,
    Completed = 3,
};

struct MatchingRequest : public IDataEntity {
    EntityId userId;
    MatchingRequestStatus status;
    EntityId battleId;

    MatchingRequest() = default;
    MatchingRequest(
        EntityId id,
        boost::mysql::datetime createdAt,
        boost::mysql::datetime updatedAt,
        EntityId userId,
        MatchingRequestStatus status,
        EntityId battleId = 0
    ) : IDataEntity(id, createdAt, updatedAt), userId(userId), status(status), battleId(battleId) {}
};

MatchingRequest toMatchingRequest(boost::mysql::row_view row, int startingIndex = 0);

enum class CreateMatchingRequestError : unsigned char {
    None = 1,
    UserNotFound = 2,
    AlreadyInMatching = 3,
};

struct CreateMatchingRequestResult {
    EntityId id = 0;
    CreateMatchingRequestError error;
};

enum class MatchCompleteError : unsigned char {
    None = 1,
    RequestNotFound = 2,
};

struct MatchCompleteResult {
    EntityId battleId = 0;
    MatchCompleteError error;
};

DSResult<CreateMatchingRequestResult> createMatchingRequest(DSContext* context, EntityId userId);

DSResult<uint64_t> cancelMatchingRequest(DSContext* context, EntityId matchingRequestId);

DSVoidResult getAvailableRequest(DSContext* context, std::chrono::seconds afterSeconds, std::vector<MatchingRequest>& resultContainer);

DSResult<MatchCompleteResult> matchComplete(DSContext* context, const std::vector<EntityId>& requests, std::string_view settingJson);

DSResult<bool, MatchingRequest> getMatchingRequest(DSContext* context, EntityId matchingRequestId);
