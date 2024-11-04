#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

// Function to parse a line from the .tbl file into a vector of strings (representing a row)
std::vector<std::string> parseRow(const std::string& line) {
    std::vector<std::string> row;
    std::istringstream ss(line);
    std::string value;

    while (std::getline(ss, value, '|')) {
        row.push_back(value);
    }

    return row;
}

// Function to load data from the .tbl file into a vector of rows (each row is a vector of strings)
std::vector<std::vector<std::string>> loadDatabase(const std::string& filename) {
    std::vector<std::vector<std::string>> database;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return database;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            std::vector<std::string> row = parseRow(line);  // Parse line into a row vector
            database.push_back(row);                        // Add the row to the database
        }
    }

    file.close();
    return database;
}

// Function to display the contents of the database
// void displayDatabase(const std::vector<std::vector<std::string>>& database) {
//     for (const auto& row : database) {
//         for (const auto& value : row) {
//             std::cout << value << " ";
//         }
//         std::cout << std::endl;
//     }
// }

std::set<std::string> generateTrigrams(const std::string& text) {
    std::set<std::string> trigrams;
    std::string padded_text = "  " + text + "  "; // Pad with spaces at both ends

    for (size_t i = 0; i <= padded_text.size() - 3; ++i) {
        trigrams.insert(padded_text.substr(i, 3)); // Extract each trigram
    }
    
    return trigrams;
}
// Function to generate trigrams for each row and store them as key-value pairs
std::unordered_map<int, std::set<std::string>> generateDatabaseTrigrams(const std::vector<std::vector<std::string>>& database) {
    std::unordered_map<int, std::set<std::string>> rowTrigrams;
    
    for (size_t rowIndex = 0; rowIndex < database.size(); ++rowIndex) {
        const auto& row = database[rowIndex];
        std::set<std::string> trigrams;

        // Generate trigrams for each field in the row and merge them into a single set
        for (const auto& field : row) {
            auto fieldTrigrams = generateTrigrams(field);
            trigrams.insert(fieldTrigrams.begin(), fieldTrigrams.end());
        }

        // Store trigrams for the current row, with row number as key
        rowTrigrams[rowIndex + 1] = trigrams; // Adding 1 to rowIndex to start rows at 1
    }

    return rowTrigrams;
}

// Function to display the trigrams for each row
void displayRowTrigrams(const std::unordered_map<int, std::set<std::string>>& rowTrigrams) {
    for (const auto& row : rowTrigrams) {
        int rowNumber = row.first;
        const std::set<std::string>& trigrams = row.second;

        std::cout << "Row " << rowNumber << ": ";
        for (const auto& trigram : trigrams) {
            std::cout << "\"" << trigram << "\" ";
        }
        std::cout << std::endl;
    }
}

int main() {
    // Load the data from the .tbl file into the database
    std::string filename = "part.tbl";
    std::vector<std::vector<std::string>> database = loadDatabase(filename);

    // Display the database contents
    //std::cout << "Database contents:" << std::endl;
    //displayDatabase(database);
     // Generate trigrams for each row in the database
    auto rowTrigrams = generateDatabaseTrigrams(database);

    // Display the trigrams for each row
    std::cout << "Row Trigrams:" << std::endl;
    displayRowTrigrams(rowTrigrams);
    return 0;
}
