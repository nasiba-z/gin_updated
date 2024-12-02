#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <memory>
#include "trigram_generator.h"
#include "read_db.h"
#include "gin_extract_value_trgm.h"



using Row = std::tuple<int, std::string, std::string, std::string, std::string, int, std::string, double, std::string>;

void processDatabase(const std::vector<Row>& database) {
    for (const auto& row : database) {
        // Extract the "Name" field
        std::string name = std::get<1>(row);

        // Variable to store the number of trigram entries
        int32_t nentries = 0;

        // Generate trigram integers using generateTrgmEntries
        auto entries = gin_extract_value_trgm(name, &nentries);

        // Display the "Name" and its trigram integers
        std::cout << "Name: " << name << "\nNumber of Trigrams: " << nentries << "\nTrigrams (as integers): ";
        for (const auto& entry : *entries) {
            std::cout << entry << " ";
        }
        std::cout << "\n";
    }
}

int main() {
    // Load the database using read_db
    std::vector<Row> database = read_db("part.tbl");

    // Process the database to extract and process "Name" fields
    processDatabase(database);

    return 0;
}
