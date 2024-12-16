#include "read_db.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <stdexcept>

std::vector<Row> read_db(const std::string& filename) {
    std::vector<Row> database;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return database;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            std::istringstream ss(line);
            std::string p_partkey_str, p_name, p_mfgr, p_brand, p_type, p_size_str, p_container, p_retailprice_str, p_comment;

            // Parse the line using '|' as the delimiter
            std::getline(ss, p_partkey_str, '|');
            std::getline(ss, p_name, '|');
            std::getline(ss, p_mfgr, '|');
            std::getline(ss, p_brand, '|');
            std::getline(ss, p_type, '|');
            std::getline(ss, p_size_str, '|');
            std::getline(ss, p_container, '|');
            std::getline(ss, p_retailprice_str, '|');
            std::getline(ss, p_comment, '|');

            // Convert fields to appropriate data types
            int p_partkey = std::stoi(p_partkey_str);
            int p_size = std::stoi(p_size_str);
            double p_retailprice = std::stod(p_retailprice_str);

            // Add the parsed row to the database
            database.emplace_back(p_partkey, p_name, p_mfgr, p_brand, p_type, p_size, p_container, p_retailprice, p_comment);
        }
    }

    file.close();
    return database;
}
