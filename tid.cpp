#include "tid.h"
#include <string>
#include <unordered_map>
#include "tid.h"
std::unordered_map<int, std::string> rowData;
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
bool TID::operator==(const TID& other) const {
    return rowId == other.rowId;
}

// Returns the row text given a TID.
std::string getRowText(const TID &tid) {
    auto it = rowData.find(tid.rowId);
    if (it != rowData.end())
        return it->second;
    return "";
}