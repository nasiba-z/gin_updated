#ifndef READ_DB_H
#define READ_DB_H

#include <string>
#include <vector>
#include <tuple>

// Define a type alias for a database row
using Row = std::tuple<
    int,              // p_partkey
    std::string,      // p_name
    std::string,      // p_mfgr
    std::string,      // p_brand
    std::string,      // p_type
    int,              // p_size
    std::string,      // p_container
    double,           // p_retailprice
    std::string       // p_comment
>;

struct TableRow {
    int id;
    std::string data; // We'll use p_name as the data for GIN index building.
};

// Function to load data from the .tbl file into a vector of Row tuples
std::vector<Row> read_db(const std::string& filename);
#endif // READ_DB_H
