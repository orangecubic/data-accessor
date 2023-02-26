#pragma once

#include "boost/mysql.hpp"

using DSError = boost::mysql::error_with_diagnostics;

using DSErrorCode = boost::mysql::common_server_errc;

const DSError DSNoError(boost::mysql::error_code{}, boost::mysql::diagnostics{});

bool isForeignKeyConstraintError(DSError error);

bool isUniqueKeyError(DSError error);