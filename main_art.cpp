// main_art.cpp
#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <map>
#include <set>
#include <algorithm>
#include "read_db.h"               // Contains read_db() function.
#include "gin_index_art.h"             // Contains GinIndex, GinFormTuple, IndexTuple, etc.
#include "gin_state.h"             // Contains GinState definition.
#include "tid.h"                   // Contains TID definition.
#include "trigram_generator.h"     // Contains trigram_generator() function.
#include "art.h"                   // Contains ART node classes.
#include <fstream>
#include <chrono>
#include "postinglist_utils.h"    // Contains intersectPostingLists() function.
#include "pattern_match.h"
using namespace std;
string vectorToString(const vector<unsigned char>& vec) {
    return string(vec.begin(), vec.end());
}
// Helper to print indentation.
void printIndent(int indent) {
    for (int i = 0; i < indent; i++) {
        cout << "  ";
    }
}

// Recursively prints the ART tree structure.
void printART(ARTNode* node, std::ostream& out, int indent = 0) {
    if (!node) return;

    // Print indentation for readability.
    for (int i = 0; i < indent; i++) {
        out << "  ";
    }

    // Print common node info: node type and prefix if available.
    string nodeType;
    switch (node->type) {
        case NodeType::LEAF:    nodeType = "Leaf"; break;
        case NodeType::NODE4:   nodeType = "Node4"; break;
        case NodeType::NODE16:  nodeType = "Node16"; break;
        case NodeType::NODE48:  nodeType = "Node48"; break;
        case NodeType::NODE256: nodeType = "Node256"; break;
        default:                nodeType = "Unknown"; break;
    }
    out << nodeType;
    if (!node->prefix.empty()) {
        out << ", prefix = \"" << vectorToString(node->prefix) << "\"";
    }
    out << std::endl;

    // If it's a leaf, print its full key.
    if (node->type == NodeType::LEAF) {
        ARTLeaf* leaf = static_cast<ARTLeaf*>(node);
        for (int i = 0; i < indent + 1; i++) {
            out << "  ";
        }
        out << "Full key: \"" << vectorToString(leaf->fullKey) << "\"" << std::endl;
        return;
    }

    // For inner nodes, print the keys stored in the node and traverse their children.
    if (node->type == NodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        for (int i = 0; i < n4->count; i++) {
            for (int j = 0; j < indent + 1; j++) {
                out << "  ";
            }
            out << "Key: " << static_cast<int>(n4->keys[i])
                << " ('" << static_cast<char>(n4->keys[i]) << "')" << std::endl;
            printART(n4->children[i], out, indent + 2);
        }
    } else if (node->type == NodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        for (int i = 0; i < n16->count; i++) {
            for (int j = 0; j < indent + 1; j++) {
                out << "  ";
            }
            out << "Key: " << static_cast<int>(n16->keys[i])
                << " ('" << static_cast<char>(n16->keys[i]) << "')" << std::endl;
            printART(n16->children[i], out, indent + 2);
        }
    } else if (node->type == NodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        // In Node48, keys are implicit: iterate over all 256 possible key bytes.
        for (int i = 0; i < 256; i++) {
            if (n48->childIndex[i] != 0xFF) { // Unused slot marked with 0xFF.
                for (int j = 0; j < indent + 1; j++) {
                    out << "  ";
                }
                out << "Key: " << i << " ('" << static_cast<char>(i) << "')" << std::endl;
                int pos = n48->childIndex[i];
                printART(n48->children[pos], out, indent + 2);
            }
        }
    } else if (node->type == NodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        // For Node256, iterate over all 256 slots.
        for (int i = 0; i < 256; i++) {
            if (n256->children[i] != nullptr) {
                for (int j = 0; j < indent + 1; j++) {
                    out << "  ";
                }
                out << "Key: " << i << " ('" << static_cast<char>(i) << "')" << std::endl;
                printART(n256->children[i], out, indent + 2);
            }
        }
    }
}


void printRootKeys(ARTNode* root) {
    if (!root) {
        cout << "ART is empty." << endl;
        return;
    }
    
    cout << "Root node type: ";
    switch (root->type) {
        case NodeType::NODE4:
            cout << "Node4" << endl;
            break;
        case NodeType::NODE16:
            cout << "Node16" << endl;
            break;
        case NodeType::NODE48:
            cout << "Node48" << endl;
            break;
        case NodeType::NODE256:
            cout << "Node256" << endl;
            break;
        case NodeType::LEAF:
            cout << "Leaf" << endl;
            break;
        default:
            cout << "Unknown" << endl;
            break;
    }
    
    // If the root is a leaf, print its key (if available) and exit.
    if (root->type == NodeType::LEAF) {
        ARTLeaf* leaf = static_cast<ARTLeaf*>(root);
        cout << "Root is a leaf. Full key: ";
        for (auto byte : leaf->fullKey)
            cout << byte;
        cout << endl;
        return;
    }
    
    // Now, print the keys for inner nodes.
    if (root->type == NodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(root);
        cout << "Keys in Node4: ";
        for (int i = 0; i < n4->count; i++) {
            cout << static_cast<int>(n4->keys[i]) << " ";
        }
        cout << endl;
    } else if (root->type == NodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(root);
        cout << "Keys in Node16: ";
        for (int i = 0; i < n16->count; i++) {
            cout << static_cast<int>(n16->keys[i]) << " ";
        }
        cout << endl;
    } else if (root->type == NodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(root);
        cout << "Keys in Node48 (using childIndex array): ";
        for (int i = 0; i < 256; i++) {
            // Assume unused entries are marked with 255.
            if (n48->childIndex[i] != 255) {
                cout << i << " ";
            }
        }
        cout << endl;
    } else if (root->type == NodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(root);
        cout << "Keys in Node256 (indices with non-null children): ";
        for (int i = 0; i < 256; i++) {
            if (n256->children[i] != nullptr) {
                cout << i << " ";
            }
        }
        cout << endl;
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
    warmUpCache("partsf10.tbl");
    auto start = std::chrono::high_resolution_clock::now();
    // 1. Read the database rows from file "part.tbl".
    vector<Row> database = read_db("partsf10.tbl");

    // 2. Transform the database rows into TableRow format.
    vector<TableRow> table;
    for (const auto& row : database) {
        int id = std::get<0>(row);          // p_partkey
        string data = std::get<1>(row);       // p_name
        table.push_back({id, data});
        rowData[id] = data; // Store the row text for later retrieval.
    }
    // 3. Aggregate posting data by trigram key.
    // We build a map from trigram (string) to a vector of TIDs.
    unordered_map<string, vector<TID>> postingMap;
    for (const auto& trow : table) {
        // Assume trigram_generator returns a set<string> of unique trigrams for the row.
        set<string> trigramSet = trigram_generator(trow.data);
        for (const auto& tri : trigramSet) {
            postingMap[tri].push_back(TID(trow.id));
        }
      
    }
    
    // // Sort trigrams by their counts in descending order and print them.
    // vector<pair<string, size_t>> trigramCounts;
    // for (const auto& entry : postingMap) {
    //     trigramCounts.emplace_back(entry.first, entry.second.size());
    // }
    // sort(trigramCounts.begin(), trigramCounts.end(), [](const auto& a, const auto& b) {
    //     return a.second > b.second;
    // });

    // for (const auto& entry : trigramCounts) {
    //     cout << "Trigram: " << entry.first << ", Count: " << entry.second << endl;
    // }

    // cout << "Number of unique trigrams: " << postingMap.size() << endl;
    GinState state(true, 256); // Reduced maxItemSize for testing posting trees.

    // 4. Create IndexTuples from each posting list.
    // Each IndexTuple holds the trigram key (as a string) and a pointer
    // to the posting data (either inline or as a posting tree).
    vector<IndexTuple*> tuples;
    // We assume GinFormTuple takes (GinState*, datum, postingData, errorTooBig)
    // where here we pass the trigram (string) as datum.
    // (For testing, GinState can be null or a dummy.)
    for (auto& entry : postingMap) {
        sort(entry.second.begin(), entry.second.end(), [](const TID& a, const TID& b) {
            return a.rowId < b.rowId;
        });
    }
    std::map<datum, vector<TID>> sortedPostingMap(postingMap.begin(), postingMap.end());

    for (const auto& kv : sortedPostingMap) {
        const string& trigram = kv.first;
        const vector<TID>& tids = kv.second;
        IndexTuple* tup = GinFormTuple_ART(&state, trigram, tids, true);
        if (tup != nullptr)
            tuples.push_back(tup);
    }
    vector<pair<vector<unsigned char>, IndexTuple*>> artItems;
    for (const auto* tup : tuples) {
        // Here we assume tup->key is a string.
        string trigram = tup->key;
        vector<unsigned char> keyVec(trigram.begin(), trigram.end());
        artItems.push_back({keyVec, const_cast<IndexTuple*>(tup)});
    }
    // Sort artItems lexicographically by key.
    sort(artItems.begin(), artItems.end(), [](const auto &p1, const auto &p2) {
        return p1.first < p2.first;
    });


    // 6. Bulk-load the ART tree using ART_bulkLoad.
    // The resulting ART tree maps each trigram (as a byte vector)
    // to the corresponding IndexTuple pointer (which holds the posting list/tree).
    ARTNode* artRoot = ART_bulkLoad(artItems, 0);
    // Record the end time.
    auto end = std::chrono::high_resolution_clock::now();
    // Compute the elapsed time in seconds.
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Bulk-loading execution time: " << elapsed.count() << " seconds." << std::endl;

     // --- Candidate Retrieval using the Gin Index (via EntryTree search) ---
    string pattern = "%mon%ros%";

    
    // Extract required trigrams from the pattern.
    std::vector<Trigram> requiredTrigrams = getRequiredTrigrams(pattern);
    vector<vector<TID>> postingLists;
    auto start_cr = std::chrono::high_resolution_clock::now();
    for (const auto &tri : requiredTrigrams) {
        // Use the entry tree search method to get the IndexTuple.
        std::vector<unsigned char> keyBytes(tri.begin(), tri.end());
        
        IndexTuple* tup = artRoot->search(keyBytes.data(),
        static_cast<int>(keyBytes.size()),
        /*depth*/ 0);
        if (tup != nullptr) {
            vector<TID> plist = getPostingList(tup);
            postingLists.push_back(plist);
        } else {
            // If any required trigram is missing, no row can match.
            postingLists.clear();
            break;
        }
    }
    // Record the end time for candidate retrieval.
    auto end_cr = std::chrono::high_resolution_clock::now();
    //print the elapsed time for candidate retrieval.
    std::chrono::duration<double> elapsed_cr = end_cr - start_cr;
    std::cout << "Candidate retrieval execution time for short predicate: " << elapsed_cr.count() << " seconds." << std::endl;
    // Intersect all posting lists to get candidate TIDs.
    vector<TID> candidateTIDs = intersectPostingLists(postingLists);
    // cout<< "Candidate TIDs: ";
    // for (const TID& tid : candidateTIDs) {
    //     cout << tid.rowId << " ";
    // }
    std::vector<TID> finalTIDs;
    for (const TID& tid : candidateTIDs)
    {
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
    std::cout << "Number of rows matching pattern \"" << pattern << "\": " << finalTIDs.size() << std::endl;

    // medium pattern:
    string pattern_med = "%chocolate%mon%";

    
    // Extract required trigrams from the pattern.
    std::vector<Trigram> requiredTrigrams_med = getRequiredTrigrams(pattern_med);
    vector<vector<TID>> postingLists_med;
    auto start_cr_med = std::chrono::high_resolution_clock::now();
    for (const auto &tri : requiredTrigrams_med) {
        // Use the entry tree search method to get the IndexTuple.
        std::vector<unsigned char> keyBytes(tri.begin(), tri.end());
        
        IndexTuple* tup = artRoot->search(keyBytes.data(),
        static_cast<int>(keyBytes.size()),
        /*depth*/ 0);
        if (tup != nullptr) {
            vector<TID> plist = getPostingList(tup);
            postingLists_med.push_back(plist);
        } else {
            // If any required trigram is missing, no row can match.
            postingLists_med.clear();
            break;
        }
    }
    // Record the end time for candidate retrieval.
    auto end_cr_med = std::chrono::high_resolution_clock::now();
    //print the elapsed time for candidate retrieval.
    std::chrono::duration<double> elapsed_cr_med = end_cr_med - start_cr_med;
    std::cout << "Candidate retrieval execution time for medium predicate: " << elapsed_cr_med.count() << " seconds." << std::endl;
    // Intersect all posting lists to get candidate TIDs.
    vector<TID> candidateTIDs_med = intersectPostingLists(postingLists_med);
    // cout<< "Candidate TIDs: ";
    // for (const TID& tid : candidateTIDs) {
    //     cout << tid.rowId << " ";
    // }
    std::vector<TID> finalTIDs_med;
    for (const TID& tid : candidateTIDs_med)
    {
        std::string text = getRowText(tid);   // fetch p_name, etc.
        // Check if the text matches the pattern and if the literals appear in order.
        // cout << "Checking text: " << text << "\n";

        if (literalsAppearInOrder(text, requiredTrigrams_med))
            finalTIDs_med.push_back(tid);
    }

    /* report -------------------------------------------------------- */
    // std::cout << "Rows matching pattern \"" << pattern << "\": ";
    // for (const TID& tid : finalTIDs)
    //     std::cout << tid.rowId << ' ';
    // std::cout << '\n';
    std::cout << "Number of rows matching pattern \"" << pattern_med << "\": " << finalTIDs_med.size() << std::endl;
    

    // medium pattern:
    string pattern_long = "%coral%chocolate%";

    
    // Extract required trigrams from the pattern.
    std::vector<Trigram> requiredTrigrams_long = getRequiredTrigrams(pattern_long);
    vector<vector<TID>> postingLists_long;
    auto start_cr_long = std::chrono::high_resolution_clock::now();
    for (const auto &tri : requiredTrigrams_long) {
        // Use the entry tree search method to get the IndexTuple.
        std::vector<unsigned char> keyBytes(tri.begin(), tri.end());
        
        IndexTuple* tup = artRoot->search(keyBytes.data(),
        static_cast<int>(keyBytes.size()),
        /*depth*/ 0);
        if (tup != nullptr) {
            vector<TID> plist = getPostingList(tup);
            postingLists_long.push_back(plist);
        } else {
            // If any required trigram is missing, no row can match.
            postingLists_long.clear();
            break;
        }
    }
    // Record the end time for candidate retrieval.
    auto end_cr_long= std::chrono::high_resolution_clock::now();
    //print the elapsed time for candidate retrieval.
    std::chrono::duration<double> elapsed_cr_long = end_cr_long - start_cr_long;
    std::cout << "Candidate retrieval execution time for long predicate: " << elapsed_cr_long.count() << " seconds." << std::endl;
    // Intersect all posting lists to get candidate TIDs.
    vector<TID> candidateTIDs_long = intersectPostingLists(postingLists_long);
    // cout<< "Candidate TIDs: ";
    // for (const TID& tid : candidateTIDs) {
    //     cout << tid.rowId << " ";
    // }
    std::vector<TID> finalTIDs_long;
    for (const TID& tid : candidateTIDs_long)
    {
        std::string text = getRowText(tid);   // fetch p_name, etc.
        // Check if the text matches the pattern and if the literals appear in order.
        // cout << "Checking text: " << text << "\n";

        if (literalsAppearInOrder(text, requiredTrigrams_long))
            finalTIDs_long.push_back(tid);
    }

    /* report -------------------------------------------------------- */
    // std::cout << "Rows matching pattern \"" << pattern << "\": ";
    // for (const TID& tid : finalTIDs)
    //     std::cout << tid.rowId << ' ';
    // std::cout << '\n';
    std::cout << "Number of rows matching pattern \"" << pattern_long << "\": " << finalTIDs_long.size() << std::endl;
    
    // 9. Cleanup.
    std::ofstream outFile("art_tree_output.txt");
    if (outFile.is_open()) {
        outFile << "ARTTree structure:\n";
        printART(artRoot, outFile);
        outFile << "-------------------------\n";
        outFile.close();
        cout << "EntryTree structure saved to art_tree_output.txt\n";
    } else {
        cerr << "Failed to open file for writing.\n";
    }

    for (auto* tup : tuples) {
        if (tup->postingTree)
            delete tup->postingTree;
        delete tup;
    }
    // postingMap.clear();
    // sortedPostingMap.clear();
    // tuples.clear();
    delete artRoot;
    return 0;
}
