#include "Error.h"

bool isForeignKeyConstraintError(DSError error) {
    return error.code().value() == (int)DSErrorCode::er_no_referenced_row_2 ||
            error.code().value() == (int)DSErrorCode::er_no_referenced_row;
}

bool isUniqueKeyError(DSError error) {
    return error.code().value() == (int)DSErrorCode::er_dup_entry;
}