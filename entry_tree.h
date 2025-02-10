#ifndef ENTRY_TREE_H
#define ENTRY_TREE_H

#include <vector>
#include <string>
#include "gin_index.h"


struct EntryTreeNode {
    bool leaf;                              // True if node is a leaf.
    std::vector<std::string> keys;          // Keys stored in the node.
    std::vector<EntryTreeNode*> children;   // Child pointers (empty if leaf).
    // For leaf nodes, we store values (pointers to IndexTuple).
    std::vector<IndexTuple*> values;         

    EntryTreeNode(bool isLeaf) : leaf(isLeaf) {}

    ~EntryTreeNode() {
        for (auto child : children)
            delete child;
        for (auto val : values)
            delete val;
    }
};

class EntryTree {
    public:
        EntryTreeNode* root;
        int t;  // Minimum degree
    
        EntryTree(int _t) : t(_t) { 
            root = new EntryTreeNode(true); 
        }
        ~EntryTree() { delete root; }
    
        // Search for a key in the tree (returns pointer to value if found, else nullptr).
        IndexTuple* search(const std::string &key) {
            return search(root, key);
        }
    
        // Insert a key and its associated IndexTuple into the B-tree.
        void insert(const std::string &key, IndexTuple* value) {
            if (root->keys.size() == 2*t - 1) {
                // Root is full, need to grow tree height.
                EntryTreeNode* s = new EntryTreeNode(false);
                s->children.push_back(root);
                splitChild(s, 0, root);
                int i = 0;
                if (s->keys[0] < key) i++;
                insertNonFull(s->children[i], key, value);
                root = s;
            } else {
                insertNonFull(root, key, value);
            }
        }
    
    private:
        // Helper search function.
        IndexTuple* search(EntryTreeNode* node, const std::string &key) {
            int i = 0;
            while (i < node->keys.size() && key > node->keys[i])
                i++;
            if (i < node->keys.size() && key == node->keys[i] && node->leaf)
                return node->values[i];
            if (node->leaf)
                return nullptr;
            return search(node->children[i], key);
        }
    
        // Insert into a non-full node.
        void insertNonFull(EntryTreeNode* node, const std::string &key, IndexTuple* value) {
            int i = node->keys.size() - 1;
            if (node->leaf) {
                // Insert new key and value into this leaf node.
                node->keys.push_back("");   // Make space
                node->values.push_back(nullptr);
                while (i >= 0 && key < node->keys[i]) {
                    node->keys[i+1] = node->keys[i];
                    node->values[i+1] = node->values[i];
                    i--;
                }
                node->keys[i+1] = key;
                node->values[i+1] = value;
            } else {
                while (i >= 0 && key < node->keys[i])
                    i--;
                i++;
                if (node->children[i]->keys.size() == 2*t - 1) {
                    splitChild(node, i, node->children[i]);
                    if (key > node->keys[i])
                        i++;
                }
                insertNonFull(node->children[i], key, value);
            }
        }
    
        // Split the full child y of node x at index i.
        void splitChild(EntryTreeNode* x, int i, EntryTreeNode* y) {
            EntryTreeNode* z = new EntryTreeNode(y->leaf);
            // z gets the last t-1 keys of y.
            for (int j = 0; j < t-1; j++) {
                z->keys.push_back(y->keys[j+t]);
            }
            if (y->leaf) {
                for (int j = 0; j < t-1; j++) {
                    z->values.push_back(y->values[j+t]);
                }
            } else {
                for (int j = 0; j < t; j++) {
                    z->children.push_back(y->children[j+t]);
                }
            }
            // Reduce keys in y.
            y->keys.resize(t-1);
            if (y->leaf)
                y->values.resize(t-1);
            else
                y->children.resize(t);
    
            x->children.insert(x->children.begin() + i + 1, z);
            x->keys.insert(x->keys.begin() + i, y->keys[t-1]);
        }
    };

EntryTree* createEntryTreeFromTuples(const std::vector<IndexTuple*>& tuples);

#endif