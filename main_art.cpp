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
// Recursively calculates the maximum depth of the ART.
// Depth is defined as the number of nodes from the root to the deepest leaf.
int calculateDepth(ARTNode* node) {
    if (!node)
        return 0;
    
    // If this is a leaf, depth is 1.
    if (node->type == NodeType::LEAF)
        return 1;
    
    int maxChildDepth = 0;
    
    switch (node->type) {
        case NodeType::NODE4: {
            ARTNode4* n4 = static_cast<ARTNode4*>(node);
            for (int i = 0; i < n4->count; i++) {
                int childDepth = calculateDepth(n4->children[i]);
                if (childDepth > maxChildDepth)
                    maxChildDepth = childDepth;
            }
            break;
        }
        case NodeType::NODE16: {
            ARTNode16* n16 = static_cast<ARTNode16*>(node);
            for (int i = 0; i < n16->count; i++) {
                int childDepth = calculateDepth(n16->children[i]);
                if (childDepth > maxChildDepth)
                    maxChildDepth = childDepth;
            }
            break;
        }
        case NodeType::NODE48: {
            ARTNode48* n48 = static_cast<ARTNode48*>(node);
            // Assuming an unused slot in childIndex is marked with 255.
            for (int i = 0; i < 256; i++) {
                if (n48->childIndex[i] != 255) {
                    int childPos = n48->childIndex[i];
                    int childDepth = calculateDepth(n48->children[childPos]);
                    if (childDepth > maxChildDepth)
                        maxChildDepth = childDepth;
                }
            }
            break;
        }
        case NodeType::NODE256: {
            ARTNode256* n256 = static_cast<ARTNode256*>(node);
            for (int i = 0; i < 256; i++) {
                if (n256->children[i] != nullptr) {
                    int childDepth = calculateDepth(n256->children[i]);
                    if (childDepth > maxChildDepth)
                        maxChildDepth = childDepth;
                }
            }
            break;
        }
        default:
            break;
    }
    
    
    return 1 + maxChildDepth;
}

void traverseART(ARTNode* node, map<string, int>& counts) {
    if (!node)
        return;

    // Determine the node type using the NodeType enum stored in the base class.
    string typeStr;
    switch (node->type) {
        case NodeType::NODE4:
            typeStr = "Node4";
            break;
        case NodeType::NODE16:
            typeStr = "Node16";
            break;
        case NodeType::NODE48:
            typeStr = "Node48";
            break;
        case NodeType::NODE256:
            typeStr = "Node256";
            break;
        case NodeType::LEAF:
            typeStr = "Leaf";
            break;
        default:
            typeStr = "Unknown";
            break;
    }
    counts[typeStr]++;

    // If it's a leaf, there are no children to traverse.
    if (node->type == NodeType::LEAF)
        return;

    // Traverse children according to the node type.
    if (node->type == NodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        // For Node4, the valid children are stored in the first n4->count slots.
        for (int i = 0; i < n4->count; i++) {
            traverseART(n4->children[i], counts);
        }
    } else if (node->type == NodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        // For Node16, iterate over the first n16->count entries.
        for (int i = 0; i < n16->count; i++) {
            traverseART(n16->children[i], counts);
        }
    } else if (node->type == NodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        // For Node48, iterate over all 256 possible slots.
        // Assume that an unused slot is marked with 255.
        for (int i = 0; i < 256; i++) {
            if (n48->childIndex[i] != 255) {
                int childPos = n48->childIndex[i];
                traverseART(n48->children[childPos], counts);
            }
        }
    } else if (node->type == NodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        // For Node256, iterate over all 256 child pointers.
        for (int i = 0; i < 256; i++) {
            if (n256->children[i] != nullptr) {
                traverseART(n256->children[i], counts);
            }
        }
    }
}

void printNonRootNode256(ARTNode* node, bool isRoot) {
    if (!node)
        return;

    // If the current node is a Node256 and it's not the root, print its details.
    if (node->type == NodeType::NODE256 && !isRoot) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        cout << "Non-root Node256 found with prefix: \"" 
             << vectorToString(node->prefix) << "\" (prefixLen = " 
             << node->prefixLen << ")" << endl;
        cout << "Children keys in this Node256: ";
        for (int i = 0; i < 256; i++) {
            if (n256->children[i] != nullptr) {
                cout << i << " ";
            }
        }
        cout << "\n-----------------------" << endl;
    }

    // Now, recursively traverse the children based on the node type.
    if (node->type == NodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        for (int i = 0; i < n4->count; i++) {
            printNonRootNode256(n4->children[i], false);
        }
    } else if (node->type == NodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        for (int i = 0; i < n16->count; i++) {
            printNonRootNode256(n16->children[i], false);
        }
    } else if (node->type == NodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        // For Node48, iterate over all 256 possible key bytes.
        // Here we assume that an unused entry in childIndex is marked with 255.
        for (int i = 0; i < 256; i++) {
            if (n48->childIndex[i] != 255) {
                int childPos = n48->childIndex[i];
                printNonRootNode256(n48->children[childPos], false);
            }
        }
    } else if (node->type == NodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        for (int i = 0; i < 256; i++) {
            if (n256->children[i] != nullptr) {
                printNonRootNode256(n256->children[i], false);
            }
        }
    }
    // Leaves have no children.
}

// Helper function to print the node type counts.
void printNodeCounts(ARTNode* root) {
    map<string, int> counts;
    traverseART(root, counts);
    cout << "Current ART Node counts:" << endl;
    for (const auto& kv : counts) {
        cout << "  " << kv.first << ": " << kv.second << endl;
    }
    cout << "-------------------------" << endl;
}


// This function recursively traverses the ART tree and prints details only for nodes of type Node48.
void printNode48Details(ARTNode* node) {
    if (!node)
        return;

    // If the current node is of type Node48, print its details.
    if (node->type == NodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        cout << "Found Node48 with prefix: \"" << vectorToString(node->prefix)
             << "\" (prefixLen = " << node->prefixLen << ")" << endl;
        // Iterate over all 256 possible key bytes.
        // We assume that an unused slot in childIndex is marked with 0xFF.
        for (int i = 0; i < 256; i++) {
            if (n48->childIndex[i] != 0xFF) {
                int childPos = n48->childIndex[i];
                cout << "  Child at key byte [" << i << "]: ";
                ARTNode* child = n48->children[childPos];
                if (child->type == NodeType::LEAF) {
                    ARTLeaf* leaf = static_cast<ARTLeaf*>(child);
                    cout << "Leaf with full key: \"" << vectorToString(leaf->fullKey) << "\"" << endl;
                } else {
                    string innerType;
                    switch (child->type) {
                        case NodeType::NODE4:   innerType = "Node4"; break;
                        case NodeType::NODE16:  innerType = "Node16"; break;
                        case NodeType::NODE48:  innerType = "Node48"; break;
                        case NodeType::NODE256: innerType = "Node256"; break;
                        default:                innerType = "Unknown"; break;
                    }
                    cout << innerType << " with prefix: \"" << vectorToString(child->prefix) << "\"" << endl;
                }
            }
        }
        cout << "--------------------------------" << endl;
    }

    // Recursively traverse children regardless of node type.
    if (node->type == NodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        for (int i = 0; i < n4->count; i++) {
            printNode48Details(n4->children[i]);
        }
    } else if (node->type == NodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        for (int i = 0; i < n16->count; i++) {
            printNode48Details(n16->children[i]);
        }
    } else if (node->type == NodeType::NODE48) {
        ARTNode48* n48 = static_cast<ARTNode48*>(node);
        // Traverse all children of the Node48.
        for (int i = 0; i < 256; i++) {
            if (n48->childIndex[i] != 0xFF) {
                int childPos = n48->childIndex[i];
                printNode48Details(n48->children[childPos]);
            }
        }
    } else if (node->type == NodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        for (int i = 0; i < 256; i++) {
            if (n256->children[i] != nullptr) {
                printNode48Details(n256->children[i]);
            }
        }
    }
    // Leaves have no children.
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
// Function to time the ART tree building process.
void timeARTBuilding(const vector<pair<vector<unsigned char>, IndexTuple*>>& artItems) {
    auto start = std::chrono::high_resolution_clock::now();
    ARTNode* root = ART_bulkLoad(artItems, 0);      // Assuming depth starts at 0.  
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Time taken to build ART tree: " << duration.count() << " micros." << endl;
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

    // 3. Aggregate posting data by trigram key.
    // We build a map from trigram (string) to a vector of TIDs.
    unordered_map<string, vector<TID>> postingMap;
    int processedRows = 0;
    for (const auto& trow : table) {
        // Assume trigram_generator returns a set<string> of unique trigrams for the row.
        set<string> trigramSet = trigram_generator(trow.data);
        for (const auto& tri : trigramSet) {
            postingMap[tri].push_back(TID(trow.id));
        }
        processedRows++;
        // if (processedRows % 10000 == 0) {
        //     cout << "Processed " << processedRows << " rows." << endl;
        // }
    }
    
    cout << "Number of unique trigrams: " << postingMap.size() << endl;
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
        // Print a counter each 1000th key
        static int counter = 0;
        counter++;
        // if (counter % 1000 == 0) {
        //     cout << "Inserted key: " << trigram << " with " << tids.size() << " TIDs." << endl;
        // }
    }
    // // 8. Print the final IndexTuples.
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
    // cout << "Number of IndexTuples formed: " << tuples.size() << endl;
    // bool found = false;
    // for (const auto* tup : tuples) {
    //     if (tup->key == "low") {
    //         cout << "Found trigram \"low\" with posting size " << tup->postingSize << endl;
    //         found = true;
    //         break;
    //     }
    // }
    // if (!found)
    //     cout << "Trigram \"low\" not found." << endl;

    // 5. Convert the IndexTuples into ART items.
    // Each ART item is a pair: { key (vector<unsigned char>), value (IndexTuple*) }.
    // We convert the trigram string (tup->key) into a vector of unsigned char.
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
    std::cout << "Total execution time: " << elapsed.count() << " seconds." << std::endl;

    // ARTNode* artRoot = ART_bulkLoad(artItems, 0);
    cout << "ART tree built via bulk loading on " << artItems.size() << " unique trigrams." << endl;

    // Additional tests: search for some specific trigrams.
    // const vector<string> testTrigrams = {"cho", "yel", "low"};
    // for (const auto& tri : testTrigrams) {
    //     vector<unsigned char> keyVec(tri.begin(), tri.end());
    //     IndexTuple* foundTuple = artRoot->search(keyVec.data(), keyVec.size(), 0);
    //     if (foundTuple ) {
    //         cout << "Trigram \"" << tri << "\" found with posting size " << foundTuple->postingSize << endl;
    //     } 
    //     else {
    //         cout << "Trigram \"" << tri << "\" not found." << endl;
    //     }
    // }
    // 8. Print the current counts of each node type.
    //printRootKeys(artRoot);
    // Now print the entire ART tree.
    // cout << "Printing ART tree:" << endl;
    // printART(artRoot);
    // int depth = calculateDepth(artRoot); // TO DO : need to fix this
    // std::cout << "The maximum depth of the ART is: " << depth << std::endl;
    // printNodeCounts(artRoot);
    //printNode48Details(artRoot);
    // 9. Cleanup.
    std::ofstream outFile("art_tree_output.txt");
    if (outFile.is_open()) {
        outFile << "ARTTree structure:\n";
        printART(artRoot, outFile);
        outFile << "-------------------------\n";
        outFile.close();
        cout << "EntryTree structure saved to entry_tree_output.txt\n";
    } else {
        cerr << "Failed to open file for writing.\n";
    }

    for (auto* tup : tuples) {
        if (tup->postingTree)
            delete tup->postingTree;
        delete tup;
    }
    postingMap.clear();
    sortedPostingMap.clear();
    tuples.clear();
    delete artRoot;
    return 0;
}
