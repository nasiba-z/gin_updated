#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <stdexcept>

// Define a tuple type for a row
using Row = std::tuple<int, std::string, std::string, std::string, std::string, int, std::string, double, std::string>;  

// Function to parse a line from the .tbl file into a Row tuple
Row parseRow(const std::string& line) {
    std::istringstream ss(line);
    std::string p_partkey_str, p_name, p_mfgr, p_brand, p_type, p_size_str, p_container, p_retailprice_str, p_comment;
    
    // Parse each field from the line using '|' as a delimiter
    std::getline(ss, p_partkey_str, '|');
    std::getline(ss, p_name, '|');
    std::getline(ss, p_mfgr, '|');
    std::getline(ss, p_brand, '|');
    std::getline(ss, p_type, '|');
    std::getline(ss, p_size_str, '|');
    std::getline(ss, p_container, '|');
    std::getline(ss, p_retailprice_str, '|');
    std::getline(ss, p_comment, '|');

    // Convert strings to appropriate data types
    int p_partkey = std::stoi(p_partkey_str);
    int p_size = std::stoi(p_size_str);
    double p_retailprice = std::stod(p_retailprice_str);

    // Create and return the tuple
    return std::make_tuple(p_partkey, p_name, p_mfgr, p_brand, p_type, p_size, p_container, p_retailprice, p_comment);
}

// Function to load data from the .tbl file into a vector of Row tuples
std::vector<Row> loadDatabase(const std::string& filename) {
    std::vector<Row> database;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return database;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            Row row = parseRow(line);  // Parse line into a Row tuple
            database.push_back(row);   // Add the row to the database
        }
    }

    file.close();
    return database;
}

// Function to display the contents of the database
void displayDatabase(const std::vector<Row>& database) {
    for (const auto& row : database) {
        std::cout << "Part Key: " << std::get<0>(row)
                  << ", Name: " << std::get<1>(row)
                  << ", Manufacturer: " << std::get<2>(row)
                  << ", Brand: " << std::get<3>(row)
                  << ", Type: " << std::get<4>(row)
                  << ", Size: " << std::get<5>(row)
                  << ", Container: " << std::get<6>(row)
                  << ", Retail Price: " << std::get<7>(row)
                  << ", Comment: " << std::get<8>(row) << std::endl;
    }
}

int main() {
    // Load the data from the .tbl file into the database
    std::string filename = "part.tbl";
    std::vector<Row> database = loadDatabase(filename);

    // Display the database contents
    std::cout << "Database contents:" << std::endl;
    displayDatabase(database);

    return 0;
}
