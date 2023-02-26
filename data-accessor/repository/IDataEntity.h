#pragma once

#include "DataSource.h"

struct IDataEntity {
    EntityId id;
    boost::mysql::datetime createdAt;
    boost::mysql::datetime updatedAt;

    IDataEntity() = default;
    IDataEntity(EntityId id, boost::mysql::datetime createdAt, boost::mysql::datetime updatedAt) : id(id), createdAt(createdAt), updatedAt(updatedAt) {}
};