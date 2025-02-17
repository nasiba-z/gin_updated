#include "trigram_generator.h" 
#include <iostream>
#include <vector>
#include <tuple>
#include <algorithm>
#include "read_db.h"

// int main() {
// 	int32_t nentries = 0;
// 	// trgm2int() returns a unique_ptr to a vector of datum (trigram keys).
// 	auto trigram = trgm2int("yel", &nentries);
// 	std::cout << trigram->at(0) << std::endl;
// 	return 0;
// }

using namespace std;

int main() {
    // Read database rows from file "part.tbl"
    vector<Row> database = read_db("part.tbl");

    // Convert database rows to TableRow format.
    vector<TableRow> table;
    for (const auto& row : database) {
        int id = std::get<0>(row);       // p_partkey
        string data = std::get<1>(row);    // p_name
        table.push_back({ id, data });
    }

    // Use a set to aggregate unique trigram keys.
    set<int32_t> uniqueTrigrams;

    // For each TableRow, extract trigram keys.
    for (const auto& trow : table) {
        int32_t nentries = 0;
        // trgm2int returns a unique_ptr to a vector of datum (int32_t)
        auto trigramArray = trgm2int(trow.data, &nentries);
        if (!trigramArray || trigramArray->empty())
            continue;
        // Insert all trigram keys from this row into the set.
        for (int32_t trigram : *trigramArray) {
            uniqueTrigrams.insert(trigram);
        }
    }

    // Print the count of unique trigrams.
    cout << "Number of unique trigrams: " << uniqueTrigrams.size() << endl;

    return 0;
}
