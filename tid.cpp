#include "tid.h"
#include <string>

std::string TID::toString() const {
    return std::to_string(rowId);
}

//for comparison later
bool TID::operator>(const TID& other) const {
    return rowId > other.rowId;
}

bool TID::operator<(const TID& other) const {
    return rowId < other.rowId;
}