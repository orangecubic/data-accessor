#pragma once

#include "../repository/IDataEntity.h"
#include <vector>

struct Battle : public IDataEntity {
    std::string setting;
    boost::mysql::datetime startedAt;
    boost::mysql::datetime endedAt;

    Battle() = default;
    Battle(
        EntityId id,
        boost::mysql::datetime createdAt,
        boost::mysql::datetime updatedAt,
        std::string_view setting,
        boost::mysql::datetime startedAt,
        boost::mysql::datetime endedAt
    ) : IDataEntity(id, createdAt, updatedAt), setting(setting), startedAt(startedAt), endedAt(endedAt) {}
};

struct BattleResult : public IDataEntity {
    EntityId userId;
    EntityId matchingRequestId;
    EntityId battleId;
    bool isWinner;

    BattleResult() = default;
    BattleResult(
        EntityId userId,
        EntityId matchingRequestId,
        EntityId battleId,
        bool isWinner,
        boost::mysql::datetime createdAt,
        boost::mysql::datetime updatedAt
    ) : IDataEntity(0, createdAt, updatedAt), userId(userId), matchingRequestId(matchingRequestId), battleId(battleId), isWinner(isWinner) {}
};

BattleResult toBattleResult(boost::mysql::row_view row, int startingIndex = 0);

enum class FinishBattleError : unsigned char {
    None = 1,
    BattleInfoNotFound = 2,
    AlreadyFinished = 3,
    UnknownWinnerId = 4,
};


DSResult<uint64_t> createBattle(DSContext* context, std::string_view settingJson);

DSResult<FinishBattleError> finishBattle(DSContext* context, EntityId battleId, EntityId winnerId);

DSVoidResult getBattleHistories(DSContext* context, EntityId userId, std::vector<BattleResult>& resultContainer);