#include "Battle.h"
#include "MatchingRequest.h"

BattleResult toBattleResult(boost::mysql::row_view row, int startingIndex) {
    return BattleResult(row[startingIndex].as_uint64(), row[startingIndex + 1].as_uint64(), row[startingIndex + 2].as_uint64(), row[startingIndex + 3].as_int64() == 1, row[startingIndex + 4].as_datetime(), row[startingIndex + 5].as_datetime());
}

DSResult<uint64_t> createBattle(DSContext* context, std::string_view settingJson) {
    auto [error, id] = co_await context->executeInsert("insert into battles(setting, started_at, ended_at, created_at, updated_at) values (?, NULL, NULL, now(3), now(3))", std::make_tuple(
        settingJson
    ));

    co_return std::make_tuple(error, id);
}

DSResult<FinishBattleError> finishBattle(DSContext* context, EntityId battleId, EntityId winnerId) {
    co_return co_await context->transaction<FinishBattleError>([&]() -> DSResult<FinishBattleError> {
        uint64_t affectedRows, lastId;

        auto [error, results] = co_await context->executeSelect("select b.ended_at, mr.* from battles b inner join matching_requests mr on b.id = ? and b.id = mr.battle_id FOR UPDATE", std::make_tuple(battleId));

        DS_RETURN_ON_ERROR(error, FinishBattleError::None);

        if (results.rows().empty()) {
            co_return std::make_tuple(DSNoError, FinishBattleError::BattleInfoNotFound);
        }

        if (!results.rows()[0][0].is_null()) {
            co_return std::make_tuple(DSNoError, FinishBattleError::AlreadyFinished);
        }

        std::stringstream sstream;
        sstream<<"insert into battle_results values ";

        bool findWinner = false;
        int index = 0;
        for (auto row : results.rows()) {
            
            auto request = toMatchingRequest(row, 1);
            sstream<<'('<<request.userId<<','<<request.id<<','<<request.battleId<<','<<(winnerId == request.userId)<<",now(3), now(3))";
            if (++index != results.rows().size()) {
                sstream<<',';
            }

            if (request.userId == winnerId) {
                findWinner = true;
            }
        }

        if (!findWinner) {
            co_return std::make_tuple(DSNoError, FinishBattleError::UnknownWinnerId);
        }

        std::tie(error, lastId) = co_await context->executeInsert(sstream.str(), std::make_tuple());
        DS_RETURN_ON_ERROR(error, FinishBattleError::None);

        std::tie(error, affectedRows) = co_await context->executeUpdate("update battles set ended_at = now(3), updated_at = now(3) where id = ?", std::make_tuple(battleId));
        co_return std::make_tuple(error, FinishBattleError::None);
    });
}

DSVoidResult getBattleHistories(DSContext* context, EntityId userId, std::vector<BattleResult>& resultContainer, uint32_t limit, uint32_t offset) {
    auto [error, results] = co_await context->executeSelect("select * from battle_results where user_id = ? limit ?, ?", std::make_tuple(userId, offset, limit));

    DS_RETURN_ON_ERROR(error);

    for (auto row : results.rows()) {
        resultContainer.push_back(toBattleResult(row));
    }
    
    co_return DSNoError;
}

