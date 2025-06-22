#include "postinglist_utils.h"
#include <algorithm>
#include <iterator>
#include <set>
#include <vector>
#include <iostream>
#include <cassert>
#include "tid.h"
// Intersect multiple posting lists (each sorted by TID.rowId).
std::vector<TID> intersectPostingLists(const std::vector<std::vector<TID>>& lists) {
    if (lists.empty())
        return {};

    std::vector<TID> result = lists[0];
    for (size_t i = 1; i < lists.size(); i++) {
        std::vector<TID> temp;
        std::set_intersection(result.begin(), result.end(),
                              lists[i].begin(), lists[i].end(),
                              back_inserter(temp),
                              [](const TID &a, const TID &b) {
                                  return a.rowId < b.rowId;
                              });
        result = std::move(temp);
        if (result.empty())
            break;
    }
    return result;
}