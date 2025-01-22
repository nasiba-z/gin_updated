#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <memory>
#include "trigram_generator.h"
#include "read_db.h"
#include "gin_extract_value_trgm.h"
#include "ginbuild_updated.h"
#include "page_management.h"

// using DatabaseRow = std::tuple<int, std::string, std::string, std::string, std::string, int, std::string, double, std::string>;

void processDatabase(const std::vector<Row>& database) {
    for (const auto& row : database) {
        std::string name = std::get<1>(row);

        if (name.empty()) {
            std::cout << "Name field is empty. Skipping row.\n";
            continue;
        }

        int32_t nentries = 0;
        auto entries = gin_extract_value_trgm(name, &nentries);

        if (!entries) {
            std::cout << "No trigrams generated for Name: " << name << "\n";
            continue;
        }

        std::cout << "Name: " << name << "\nNumber of Trigrams: " << nentries << "\nTrigrams (as integers): ";
        for (const auto& entry : *entries) {
            std::cout << entry << " ";
        }
        std::cout << "\n";
    }
}

// int main() {
//     std::vector<Row> database = read_db("part.tbl");

//     GinState ginstate{true, 1024};

//     try {
//         std::vector<IndexTuple> ginIndex = ginBuild(database, ginstate);

//         std::cout << "GIN Index built successfully. Output:\n";
//         for (const auto& tuple : ginIndex) {
//             std::cout << "Key: " << tuple.datums[0] << "\n";
//             std::cout << "Posting List: ";
//             for (size_t i = tuple.postingOffset; i < tuple.postingOffset + tuple.postingSize; ++i) {
//                 std::cout << tuple.datums[i] << " ";
//             }
//             std::cout << "\n";
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "Error building GIN index: " << e.what() << "\n";
//         return 1;
//     }

//     return 0;
// }

int main() {
    // Load database rows from file
    std::vector<Row> database = read_db("part2.tbl");

    // Transform database into TableRow format for ginBuild
    std::vector<TableRow> table;
    for (const auto& row : database) {
        int id = std::get<0>(row);          // Extract p_partkey
        std::string data = std::get<1>(row); // Extract p_name
        table.push_back({id, data});
    }

 // explore other search methods just'abc' without any continuation, how does it fit with original like 
    //in some way it's like %choc%. ' ch'? 'b'? what if no space before choc?
    // check if number of tuples match, do some exception
    std::string searchPattern = "choc";
    int32_t nentries = 0; // Number of trigrams generated
    auto entries = gin_extract_value_trgm(searchPattern, &nentries);

    if (!entries || nentries == 0) {
        std::cerr << "No trigrams generated for the search pattern: " << searchPattern << "\n";
        return 1;
    }
    

    



    // Initialize GIN index state
    GinState ginstate{true, 8192};

    
    try {
        // Build GIN index
        std::vector<GinPage> ginIndex = ginBuild_updated(table, ginstate);

        // Process and output GIN index
        std::cout << "GIN Index built successfully. Output:\n";

        for (const auto& entry : *entries) {
        bool found = false;
        for (const auto& page : ginIndex) {
            for (const auto& tuple : page.tuples) { // Iterate over the tuples in the page
                // Convert key in tuple to an integer for comparison
                int tupleKey = std::stoi(tuple.datums[0]);

                if (tupleKey == entry) {
                    found = true;
                    std::cout << tupleKey<< "\n";
                    std::cout << "Matched Key: " << tupleKey << "\n";
                    std::cout << "Posting List: ";

                    // Process and print the posting list
                    for (size_t i = tuple.postingOffset; i < tuple.postingOffset + tuple.postingSize; ++i) {
                        std::cout << tuple.datums[i] << " ";
                    }
                    std::cout << "\n";
                }
            }
        }

        if (!found) {
            
            std::cout << "No match found for entry: " << entry << "\n";
        }
    }
    } catch (const std::exception& e) {
        std::cerr << "Error building GIN index: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
