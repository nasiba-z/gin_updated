#ifndef READ_DB_H
#define READ_DB_H

#include <string>
#include <vector>
#include <tuple>

// Define a type alias for a database row
using Row = std::tuple<int, std::string, std::string, std::string, std::string, int, std::string, double, std::string>;

// Function to parse a line from a .tbl file into a Row tuple
Row parseRow(const std::string& line);

// Function to load data from the .tbl file into a vector of Row tuples
std::vector<Row> read_db(const std::string& filename);

// Function to display the contents of the database
void displayDatabase(const std::vector<Row>& database);

#endif // READ_DB_H
