#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <algorithm>
#include "read_db.h"               // Contains read_db() function.
#include "gin_index.h"             // Contains GinIndex, GinFormTuple, IndexTuple, etc.
#include "gin_state.h"             // Contains GinState definition.
#include "tid.h"                   // Contains TID definition.
#include "trigram_generator.h"     // Contains trgm2int() function.
#include "entry_tree.h"            // Contains EntryTree definition.
#include "posting_tree.h"          // Contains PostingTree definition.
#include <fstream>
#include <chrono>
#include "postinglist_utils.h"
#include "pattern_match.h"
using namespace std;

// --- Helper Functions for Candidate Retrieval ---

// Helper function to recursively print the entry tree.
void printEntryTree(EntryTreeNode* node, std::ostream& out, int depth = 0, bool isRoot = true) {
    if (!node) return;

    // Indentation for readability.
    for (int i = 0; i < depth; i++) out << "  ";

    // Print whether the node is the root, internal, or leaf.
    if (isRoot) {
        out << "Root: ";
    } else if (node->leaf) {
        out << "Leaf: ";
    } else {
        out << "Internal: ";
    }

    // Print the keys in the node.
    for (const auto& k : node->keys) {
        out << k << " ";
    }
    out << "\n";

    // If it's a leaf node, print the associated values.
    if (node->leaf) {
        for (size_t i = 0; i < node->keys.size(); i++) {
            for (int j = 0; j < depth + 1; j++) out << "  "; // Indentation
            out << "Key " << node->keys[i] << " -> ";
            if (i < node->values.size() && node->values[i]) {
                out << "IndexTuple (key: " << node->values[i]->key << ")";
            } else {
                out << "No associated value";
            }
            out << "\n";
        }
    }

    // Recursively print the children for internal nodes.
    if (!node->leaf) {
        for (auto child : node->children) {
            printEntryTree(child, out, depth + 1, false);
        }
    }
}


// For testing purposes: warm up the cache by reading a file into memory.
void warmUpCache(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open " << filename << " for warmup." << std::endl;
        return;
    }
    // Read the entire file into a temporary buffer.
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    std::cout << "Warmup: Read " << buffer.size() << " bytes from " << filename << std::endl;
    // The buffer goes out of scope here, leaving the file data cached in RAM.
}
int main() {
     // Record the start time.
    warmUpCache("part.tbl");
    auto start = std::chrono::high_resolution_clock::now();
    // 1. Read the database rows from file "part.tbl".
    vector<Row> database = read_db("part.tbl");

    // 2. Transform the database rows into TableRow format.
    vector<TableRow> table;
    for (const auto& row : database) {
        int id = std::get<0>(row);          // p_partkey
        string data = std::get<1>(row);       // p_name
        table.push_back({id, data});
        rowData[id] = data; // Store the row text for later retrieval.
    }

    // 3. Create a GinState with desired parameters.
    GinState state(true, 384); // Include inline posting list, 384 bytes for inline size. as part of ginstate to allow dynamic max size for inline posting.

    

    // 5. Aggregate posting data by trigram key.
    unordered_map<datum, vector<TID>> postingMap;
    int processedRows = 0;
    for (const auto& trow : table) {
        int32_t nentries = 0;
        auto trigramArray = trgm2int(trow.data, &nentries);
        if (!trigramArray || trigramArray->empty()) {
            continue;
        }
        // Process every trigram for this row.
        for (datum trigramKey : *trigramArray) {
            // Append the row's TID (constructed from trow.id) to the vector for this trigram.
            postingMap[trigramKey].push_back(TID(trow.id));
        }
        // processedRows++;
        // if (processedRows % 10000 == 0) {
        //     cout << "Processed " << processedRows << " rows." << endl;
        // }
    }
    // Now sort each posting list once.
    for (auto& entry : postingMap) {
        sort(entry.second.begin(), entry.second.end(), [](const TID& a, const TID& b) {
            return a.rowId < b.rowId;
        });
    }

    // 6. Convert postingMap to a sorted map for debugging.
    std::map<datum, vector<TID>> sortedPostingMap(postingMap.begin(), postingMap.end());
    std::cout << "Number of keys in sortedPostingMap: " << sortedPostingMap.size() << std::endl;
    // 7. Form final IndexTuples from the GinIndex.
    vector<IndexTuple*> tuples;
    for (const auto& kv : sortedPostingMap) {
        datum key = kv.first;
        const vector<TID>& tids = kv.second;
        IndexTuple* tup = GinFormTuple(&state, key, tids, true);
        if (tup != nullptr)
            tuples.push_back(tup);

        // Print a counter each 1000th key
        // static int counter = 0;
        // counter++;
        // if (counter % 50 == 0) {
        //     cout << "Inserted key: " << key << " with " << tids.size() << " TIDs." << endl;
        // }
    }


    // 9. Build and Test EntryTree
    // For the EntryTree, we assume that each IndexTuple->key is an int32_t.
    // First, sort the tuples by key.
    vector<IndexTuple*> sortedTuples = tuples;
    sort(sortedTuples.begin(), sortedTuples.end(), [](const IndexTuple* a, const IndexTuple* b) {
        return a->key < b->key;
    });

    // Build the entry tree using bulk-loading.
    // Initial Build of Entry Tree:
    // GIN uses a bulk-loading method—extracting, de-duplicating, and sorting the keys, 
    // then building the tree bottom-up.

    // Subsequent Updates:
    // For online operations, incremental insertion is used if new keys need to be added. 
    // However, the initial creation of the entry tree is entirely based on bulk loading.

    // References in the PostgreSQL Source Code:

    // In ginbuild() and ginHeapTupleBulkInsert() in src/backend/access/gin/gininsert.c, 
    //  the system accumulates entries and later processes them in bulk.
    // The routines that build the entry tree are relatively straightforward compared to the 
    // more complex handling of posting trees (see ginpostingtree.c), because the entry tree only needs 
    // to maintain a sorted, static list of keys with pointers to posting data.
    // This separation of concerns (bulk-loading for static keys in the entry tree and a hybrid approach 
    // for large posting lists) is one of the key design decisions in the GIN index that allows it to be 
    // both efficient and scalable.
    
   
    EntryTree entryTree;
    entryTree.bulkLoad(sortedTuples);
    // Record the end time.
    auto end = std::chrono::high_resolution_clock::now();
    // Compute the elapsed time in seconds.
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Bulk-loading execution time: " << elapsed.count() << " seconds." << std::endl;
    // Save the EntryTree structure to a .txt file.
    std::ofstream outFile("entry_tree_output.txt");
    if (outFile.is_open()) {
        outFile << "EntryTree structure:\n";
        printEntryTree(entryTree.root, outFile);
        outFile << "-------------------------\n";
        outFile.close();
        cout << "EntryTree structure saved to entry_tree_output.txt\n";
    } else {
        cerr << "Failed to open file for writing.\n";
    }
    
    // --- Candidate Retrieval using the Gin Index (via EntryTree search) ---
    // Disabled for now. Uncomment the following code to enable candidate retrieval.
    // Calculate time for this section only
    auto start_cr = std::chrono::high_resolution_clock::now();
    string pattern = "%chocolate%mon%";
    // Extract required trigrams from the pattern.
    std::vector<Trigram> requiredTrigrams = getRequiredTrigrams(pattern);

    vector<vector<TID>> postingLists;

    for (const auto &tri : requiredTrigrams) {
        std::cout << tri << " ";
        int32_t key = packTrigram(tri);
        // Use the entry tree search method to get the IndexTuple.
        IndexTuple* tup = entryTree.search(key);
        if (tup != nullptr) {
            vector<TID> plist = getPostingList(tup);
            postingLists.push_back(plist);
        } else {
            // If any required trigram is missing, no row can match.
            postingLists.clear();
            break;
        }
    }

    // Intersect all posting lists to get candidate TIDs.
    vector<TID> candidateTIDs = intersectPostingLists(postingLists);
       std::vector<TID> finalTIDs;
    for (const TID& tid : candidateTIDs)
    {   
        // std::cout << tid.rowId << " ";
        std::string text = getRowText(tid);   // fetch p_name, etc.
        // Check if the text matches the pattern and if the literals appear in order.
        // cout << "Checking text: " << text << "\n";

        if (literalsAppearInOrder(text, requiredTrigrams))
            finalTIDs.push_back(tid);
    }


    /* report -------------------------------------------------------- */
    // std::cout << "Rows matching pattern \"" << pattern << "\": ";
    // for (const TID& tid : finalTIDs)
    //     std::cout << tid.rowId << ' ';
    // std::cout << '\n';
    // count the number of matching rows
    std::cout << "Number of rows matching pattern \"" << pattern << "\": " << finalTIDs.size() << std::endl;
    // std::cout << "Number of rows matching pattern \"" << pattern << "\": " << survivors.size() << std::endl;
    // Record the end time for candidate retrieval.
    auto end_cr = std::chrono::high_resolution_clock::now();
    //print the elapsed time for candidate retrieval.
    std::chrono::duration<double> elapsed_cr = end_cr - start_cr;
    std::cout << "Candidate retrieval execution time: " << elapsed_cr.count() << " seconds." << std::endl;

    // 11. Cleanup: Delete all allocated IndexTuples and associated PostingTrees.
    for (auto* tup : tuples) {
        if (tup->postingTree)
            delete tup->postingTree;
        delete tup;
    }
    postingMap.clear();
    sortedPostingMap.clear();

    // if (outFile.is_open()) {
    //     outFile.close();
    // }
    // std::vector<int>().swap(leafDepths); // Clears and releases memory for leafDepths
    // std::cout.flush();
    return 0;
}