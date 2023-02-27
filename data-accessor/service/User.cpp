#include "User.h"
#include "boost/mysql.hpp"

User toUser(boost::mysql::row_view row, int startingIndex) {
    return User(row[startingIndex].as_uint64(), row[startingIndex + 2].as_datetime(), row[startingIndex + 3].as_datetime(), row[startingIndex + 1].as_string());
}

DSResult<CreateUserResult> createUser(DSContext* context, std::string_view nickname, std::string_view password) {
    co_return co_await context->transaction<CreateUserResult>([&]() -> DSResult<CreateUserResult> {
        auto [error, id] = co_await context->executeInsert("insert into users(nickname, created_at, updated_at) values (?, now(3), now(3))", std::make_tuple(nickname));

        if (error.code()) {
            if (error.code().value() == (int)DSErrorCode::er_dup_entry) {
                co_return std::make_tuple(DSNoError, CreateUserResult{0, CreateUserError::DuplicateNickname});
            }
            DS_RETURN_ON_ERROR(error, CreateUserResult{});
        }
        
        auto [err, _id] = co_await context->executeInsert("insert into user_authentications values (?, ?, now(3), now(3))", std::make_tuple(id, password));

        DS_RETURN_ON_ERROR(err, CreateUserResult{});

        co_return std::make_tuple(DSNoError, CreateUserResult{id, CreateUserError::None});
    });
}

DSResult<uint64_t> deleteUser(DSContext* context, EntityId userId) {

    auto [error, count] = co_await context->executeUpdate("update users set deleted_at = now(3) where userId = ?", std::make_tuple(userId));

    DS_RETURN_ON_ERROR(error, 0);

    co_return std::make_tuple(DSNoError, count);
}

DSResult<std::vector<User>> getUsers(DSContext* context, uint32_t limit, uint32_t offset) {
    std::vector<User> users;

    auto [error] = co_await getUsers(context, users, limit, offset);

    co_return std::make_tuple(error, users);
}

DSVoidResult getUsers(DSContext* context, std::vector<User>& resultContainer, uint32_t limit, uint32_t offset) {;

    auto [error, result] = co_await context->executeSelect("select * from users limit ?, ?", std::make_tuple(offset, limit));

    for (auto row : result.rows()) {
        
        resultContainer.push_back(toUser(row));
    }

    co_return DSNoError;
}

DSResult<bool, User> getUser(DSContext* context, std::string_view nickname) {
    auto [error, result] = co_await context->executeSelect("select * from users where nickname = ?", std::make_tuple(nickname));

    DS_RETURN_ON_ERROR(error, false, User{});

    bool hasValue = result.has_value();
    co_return std::make_tuple(DSNoError, hasValue, hasValue ? toUser(result.rows()[0]) : User{});
}

DSResult<bool> authorizeUser(DSContext* context, EntityId userId, std::string_view password) {
    auto [error, result] = co_await context->executeSelect("select 1 from user_authentications where user_id = ? and `password` = ?", std::make_tuple(userId, password));
    
    DS_RETURN_ON_ERROR(error, false);

    co_return std::make_tuple(DSNoError, result.rows().size() == 1);
}

DSResult<bool> authorizeUser(DSContext* context, std::string_view nickname, std::string_view password) {
    auto [error, result] = co_await context->executeSelect("select 1 from users u inner join user_authentications auth on u.nickname = ? and u.id = auth.user_id and auth.password = ?", std::make_tuple(nickname, password));
    
    DS_RETURN_ON_ERROR(error, false);

    co_return std::make_tuple(DSNoError, result.rows().size() == 1);
}