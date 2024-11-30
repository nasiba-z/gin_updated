#include "trigram_generator.h"
#include "read_db.h"
int main() {
    // Load the data from the .tbl file into the database
    std::string filename = "part.tbl";
    std::vector<Row> database = loadDatabase(filename);

    // Display the database contents
    // std::cout << "Database contents:" << std::endl;
    // displayDatabase(database);
    std::string testInput = "aquamarine cornsilk honeydew beige azure"; 
    std::cout << "Input: " << testInput << std::endl;

    // Generate trigrams
    std::set<Trigram> trigrams = trigram_generator(testInput);

    // Display trigrams
    displayTrigrams(trigrams);

    return 0;
}
