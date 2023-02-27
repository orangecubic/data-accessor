#pragma once

#include "../repository/IDataEntity.h"
#include <vector>

struct UserAuthentication : public IDataEntity {
    EntityId userId;
    std::string password;

    UserAuthentication() = default;
    UserAuthentication(EntityId id, boost::mysql::datetime createdAt, boost::mysql::datetime updatedAt, EntityId userId, std::string_view password) : IDataEntity(id, createdAt, updatedAt), userId(userId), password(password) {}
};

struct User : public IDataEntity {
    std::string nickname;

    User() = default;
    User(EntityId id, boost::mysql::datetime createdAt, boost::mysql::datetime updatedAt, std::string_view nickname) : IDataEntity(id, createdAt, updatedAt), nickname(nickname) {}
};

User toUser(boost::mysql::row_view row, int startingIndex = 0);

enum class CreateUserError : unsigned char {
    None = 1,
    DuplicateNickname = 2,
};

struct CreateUserResult {
    EntityId id = 0;
    CreateUserError error;
};

DSResult<CreateUserResult> createUser(DSContext* context, std::string_view nickname, std::string_view password);

DSResult<uint64_t> deleteUser(DSContext* context, EntityId userId);

DSResult<std::vector<User>> getUsers(DSContext* context, uint32_t limit = 100, uint32_t offset = 0);

DSVoidResult getUsers(DSContext* context, std::vector<User>& resultContainer, uint32_t limit = 100, uint32_t offset = 0);

DSResult<bool, User> getUser(DSContext* context, std::string_view nickname);

DSResult<bool> authorizeUser(DSContext* context, EntityId userId, std::string_view password);

DSResult<bool> authorizeUser(DSContext* context, std::string_view nickname, std::string_view password);