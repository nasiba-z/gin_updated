#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <map>
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
using namespace std;

// --- Helper Functions for Candidate Retrieval ---

// Pack a trigram string (assumed to be exactly 3 characters) into an int32_t,
// using the same method as in trgm2int().
int32_t packTrigram(const std::string &tri) {
    if (tri.size() < 3) return 0;
    uint32_t val = 0;
    val |= static_cast<unsigned char>(tri[0]);
    val <<= 8;
    val |= static_cast<unsigned char>(tri[1]);
    val <<= 8;
    val |= static_cast<unsigned char>(tri[2]);
    return static_cast<int32_t>(val);
}

// Maps row IDs to the full row text.
map<int, string> rowData;

// Returns the row text given a TID.
string getRowText(const TID &tid) {
    auto it = rowData.find(tid.rowId);
    if (it != rowData.end())
        return it->second;
    return "";
}

// Given an IndexTuple, retrieve its posting list as a vector of TID.
// This function checks if the posting list is inline; if not, it assumes that
// PostingTree provides a method getTIDs() that returns a vector<TID>.
vector<TID> getPostingList(IndexTuple* tup) {
    vector<TID> result;
    if (tup->postingList) {
        result = tup->postingList->tids;
    } else if (tup->postingTree) {
        // Assume postingTree has a method getTIDs() that returns a vector<TID>.
        result = tup->postingTree->getTIDs();
    }
    return result;
}

// Intersect multiple posting lists (each sorted by TID.rowId).
vector<TID> intersectPostingLists(const vector<vector<TID>>& lists) {
    if (lists.empty())
        return {};

    vector<TID> result = lists[0];
    for (size_t i = 1; i < lists.size(); i++) {
        vector<TID> temp;
        std::set_intersection(result.begin(), result.end(),
                              lists[i].begin(), lists[i].end(),
                              back_inserter(temp),
                              [](const TID &a, const TID &b) {
                                  return a.rowId < b.rowId;
                              });
        result = std::move(temp);
        if (result.empty())
            break;
    }
    return result;
}

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
// Time the entry tree building
void timeEntryTreeBuilding(const vector<IndexTuple*>& tuples) {
    auto start = std::chrono::high_resolution_clock::now();
    EntryTree tree;
    tree.bulkLoad(tuples);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Time taken to build EntryTree: " << elapsed.count() << " microseconds.\n";
}

// Helper function to compute the depth of leaves (should be same for all leaves)
void computeLeafDepth(EntryTreeNode* node, int currentDepth, vector<int>& depths) {
    if (!node) return;
    if (node->leaf) {
        depths.push_back(currentDepth);
    } else {
        for (auto child : node->children) {
            computeLeafDepth(child, currentDepth + 1, depths);
        }
    }
}
int main() {
     // Record the start time.
     auto start = std::chrono::high_resolution_clock::now();
    // 1. Read the database rows from file "part.tbl".
    vector<Row> database = read_db("part.tbl");

    // 2. Transform the database rows into TableRow format.
    vector<TableRow> table;
    for (const auto& row : database) {
        int id = std::get<0>(row);          // p_partkey
        string data = std::get<1>(row);       // p_name
        table.push_back({id, data});
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

    // Debug: Print the number of unique trigrams.
    // cout << "Number of unique trigrams: " << sortedPostingMap.size() << endl;
    // for (const auto& entry : sortedPostingMap) {
    //     cout << "Trigram: " << entry.first << " -> TIDs: ";
    //     for (const auto& tid : entry.second) {
    //         cout << tid.rowId << " ";
    //     }
    //     cout << endl;
    // }

    // 7. Form final IndexTuples from the GinIndex.
    vector<IndexTuple*> tuples;
    for (const auto& kv : sortedPostingMap) {
        datum key = kv.first;
        const vector<TID>& tids = kv.second;
        IndexTuple* tup = GinFormTuple(&state, key, tids, true);
        if (tup != nullptr)
            tuples.push_back(tup);

        // Print a counter each 1000th key
        static int counter = 0;
        counter++;
        if (counter % 1000 == 0) {
            cout << "Inserted key: " << key << " with " << tids.size() << " TIDs." << endl;
        }
    }

    // 8. Print the final IndexTuples.
    // for (const auto* tup : tuples) {
    //     cout << "IndexTuple key: " << tup->key << "\n";
    //     cout << "Posting Size (number of TIDs): " << tup->postingSize << "\n";
    //     if (tup->postingList) {
    //         // cout << "Inline Posting List TIDs: ";
    //         // for (const auto& tid : tup->postingList->tids) {
    //         //     cout << "(" << tid.rowId << ") ";
    //         // }
    //         // cout << "\n";
    //     } else if (tup->postingTree) {
    //         cout << "Posting tree present. Total tree size (simulated): " 
    //              << tup->postingTree->getTotalSize() << " bytes.\n";
    //     }
    //     cout << "-------------------------\n";
    // }
    cout << "Number of IndexTuples formed: " << tuples.size() << endl;
    // ---------------------------------------------------------------------
    // Additional Test: Build Posting Trees via Incremental Insertion.
    // For a few keys that have a substantial number of TIDs, we'll build a posting tree
    // by inserting TIDs one-by-one (standard building) and print its simulated size.
    // ---------------------------------------------------------------------
    // cout << "\nTesting incremental insertion for posting trees:\n";
    // int testCount = 0;
    // for (const auto& kv : sortedPostingMap) {
    //     const datum key = kv.first;
    //     const vector<TID>& tids = kv.second;
    //     // Only test keys with more than 7 TIDs (adjust threshold as desired)
    //     if (tids.size() > 7) {
    //         PostingTree incrementalTree;
    //         for (const TID& tid : tids) {
    //             incrementalTree.insert(tid);
    //         }
    //         cout << "Key " << key 
    //              << " (with " << tids.size() << " TIDs) incremental tree built. "
    //              << "Total tree size (simulated): " 
    //              << incrementalTree.getTotalSize() << " bytes.\n";
    //         testCount++;
    //     }
    //     if (testCount >= 5) // test only the first 5 keys for brevity
    //         break;
    // }


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
    std::cout << "Total execution time: " << elapsed.count() << " seconds." << std::endl;
    cout << "EntryTree built. Total keys in tree: " << entryTree.getTotalSize() << endl;
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
    // string trigramStr = "smo";
    // int32_t searchKey = packTrigram(trigramStr);  // Pack "smo" into an int32_t key.
    // IndexTuple* foundTuple = entryTree.search(searchKey);

    // if (foundTuple) {
    //     cout << "Found posting list for trigram \"" << trigramStr << "\" (key " << searchKey << "): ";
    //     vector<TID> postingList = getPostingList(foundTuple);
    //     for (const auto &tid : postingList)
    //         cout << tid.rowId << " ";
    //     cout << "\n";
    // } else {
    //     cout << "No entry found for trigram \"" << trigramStr << "\" (key " << searchKey << ").\n";
    // }
    // 10 Check that all leaves are at the same depth.
    // vector<int> leafDepths;
    // computeLeafDepth(entryTree.root, 0, leafDepths);
    // if (!leafDepths.empty()) {
    //     int expectedDepth = leafDepths.front();
    //     bool balanced = all_of(leafDepths.begin(), leafDepths.end(), [expectedDepth](int d) {
    //         return d == expectedDepth;
    //     });
    //     cout << "All leaves are at depth " << expectedDepth 
    //          << (balanced ? " (balanced)" : " (not balanced)") << endl;
    // }
    // /// --- Candidate Retrieval using the Gin Index (via EntryTree search) ---
    // Disabled for now. Uncomment the following code to enable candidate retrieval.
    // string pattern = "%smo%blu%";
    // // Extract required trigrams from the pattern.
    // set<string> requiredTrigrams = getRequiredTrigrams(pattern);
    // cout << "Required trigrams for pattern \"" << pattern << "\": ";
    // for (const auto &tri : requiredTrigrams) {
    //     cout << tri << " ";
    // }
    // cout << "\n";

    // vector<vector<TID>> postingLists;

    // for (const auto &tri : requiredTrigrams) {
    //     int32_t key = packTrigram(tri);
    //     // Use the entry tree search method to get the IndexTuple.
    //     IndexTuple* tup = entryTree.search(key);
    //     if (tup != nullptr) {
    //         vector<TID> plist = getPostingList(tup);
    //         postingLists.push_back(plist);
    //     } else {
    //         // If any required trigram is missing, no row can match.
    //         postingLists.clear();
    //         break;
    //     }
    // }

    // // Intersect all posting lists to get candidate TIDs.
    // vector<TID> candidateTIDs = intersectPostingLists(postingLists);
    // // --- Final Verification: Check Order ("smo" comes before "blu") ---
    // vector<TID> finalCandidates;
    // for (const auto &tid : candidateTIDs) {
    //     string text = getRowText(tid);
    //     size_t posSmo = text.find("smo");
    //     size_t posBlu = text.find("blu");
    //     if (posSmo != string::npos && posBlu != string::npos && posSmo < posBlu) {
    //         finalCandidates.push_back(tid);
    //     }
    // }
    
    // cout << "\nFinal candidate row IDs after order verification: ";
    // for (const auto &tid : finalCandidates) {
    //     cout << tid.rowId << " ";
    // }
    // cout << "\n";
    

    // 11. Cleanup: Delete all allocated IndexTuples and associated PostingTrees.
    for (auto* tup : tuples) {
        if (tup->postingTree)
            delete tup->postingTree;
        delete tup;
    }
    postingMap.clear();
    sortedPostingMap.clear();
    tuples.clear();
    // if (outFile.is_open()) {
    //     outFile.close();
    // }
    // std::vector<int>().swap(leafDepths); // Clears and releases memory for leafDepths
    std::cout.flush();
    return 0;
}