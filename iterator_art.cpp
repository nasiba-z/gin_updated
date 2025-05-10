// Call main.exe 50 times and save three columns of output to a file:
// // 1. Iteration number
// // 2. Time for bulk-loading the EntryTree
// // 3. Time for candidate retrieval
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <string>

int main() {
    // Output file to save the results
    std::ofstream outFile("iteration_results_art2_sf10.csv");
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open output file!" << std::endl;
        return 1;
    }

    // Write the header for the CSV file
    outFile << "Iteration,Time_Bulk_Loading,Time_Candidate_Retrieval\n";

    // Loop for 50 iterations
    for (int i = 1; i <= 50; ++i) {
        std::cout << "Running iteration " << i << "..." << std::endl;

        // Start timing the execution of main.exe
        auto start = std::chrono::high_resolution_clock::now();

        // Run the compiled main.exe and redirect its output to a temporary file
        system("./main_art > temp_output.txt");
        // End timing
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        // Read the output from temp_output.txt
        std::ifstream tempFile("temp_output.txt");
        if (!tempFile.is_open()) {
            std::cerr << "Error: Could not open temporary output file!" << std::endl;
            return 1;
        }

        std::string line;
        double bulkLoadingTime = 0.0;
        double candidateRetrievalTime = 0.0;

        while (std::getline(tempFile, line)) {
            if (line.find("Bulk-loading execution time:") != std::string::npos) {
                bulkLoadingTime = std::stod(line.substr(line.find(":") + 1));
            } else if (line.find("Candidate retrieval execution time:") != std::string::npos) {
                candidateRetrievalTime = std::stod(line.substr(line.find(":") + 1));
            }
        }
        tempFile.close();

        // Write the results to the CSV file
        outFile << i << "," << bulkLoadingTime << "," << candidateRetrievalTime << "\n";
    }

    // Close the output file
    outFile.close();

    // Clean up the temporary file
    std::remove("temp_output.txt");

    return 0;
}