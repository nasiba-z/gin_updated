#ifndef POSTING_TREE_H
#define POSTING_TREE_H
#include "tid.h"
#include "gin_posting_list.h"
#include "ginbtree_node.h"
#include <vector>
#include <cstdint>

struct GinPostingList;

class PostingTree {
    public:
        BTreeNode* root; // Pointer to root node.
        int t;           // Minimum degree (defines the range for number of keys)
        std::vector<GinPostingList*> segments; // Vector to hold posting list segments
    
        PostingTree(int _t) : t(_t) {
            root = new BTreeNode(true);
        }
    
        ~PostingTree() { delete root; }
    
        // Insert a key into the B-tree.
        void insert(const TID& k) {
            // If root is full, then tree grows in height.
            if (root->keys.size() == 2*t - 1) {
                BTreeNode* s = new BTreeNode(false);
                s->children.push_back(root);
                splitChild(s, 0, root);
                int i = 0;
                if (s->keys.size() > 0 && k > s->keys[0])
                    i++;
                insertNonFull(s->children[i], k);
                root = s;
            } else {
                insertNonFull(root, k);
            }
        }
    
        // Search for a key in the tree (optional).
        bool search(const TID& k) const {
            return search(root, k) != nullptr;
        }
    
    private:
        // Helper: Search a key in subtree rooted with node x.
        BTreeNode* search(BTreeNode* x, const TID& k) const {
            int i = 0;
            while (i < x->keys.size() && k > x->keys[i])
                i++;
            if (i < x->keys.size() && k.toString() == x->keys[i].toString())
                return x;
            if (x->leaf)
                return nullptr;
            return search(x->children[i], k);
        }
    
        // Helper: Insert a key into a non-full node.
        void insertNonFull(BTreeNode* x, const TID& k) {
            int i = x->keys.size() - 1;
            if (x->leaf) {
                // Insert into the leaf node.
                x->keys.push_back(k); // Temporary: add at end.
                while (i >= 0 && k < x->keys[i]) {
                    x->keys[i+1] = x->keys[i];
                    i--;
                }
                x->keys[i+1] = k;
            } else {
                // Find the child which is going to have the new key.
                while (i >= 0 && k < x->keys[i])
                    i--;
                i++;
                if (x->children[i]->keys.size() == 2*t - 1) {
                    splitChild(x, i, x->children[i]);
                    if (k > x->keys[i])
                        i++;
                }
                insertNonFull(x->children[i], k);
            }
        }
    
        // Helper: Split the child y of node x at index i.
        void splitChild(BTreeNode* x, int i, BTreeNode* y) {
            BTreeNode* z = new BTreeNode(y->leaf);
            // z will hold t-1 keys from y.
            for (int j = 0; j < t-1; j++) {
                z->keys.push_back(y->keys[j+t]);
            }
            // If y is not a leaf, copy the last t children to z.
            if (!y->leaf) {
                for (int j = 0; j < t; j++) {
                    z->children.push_back(y->children[j+t]);
                }
            }
            // Reduce the number of keys in y.
            y->keys.resize(t-1);
            if (!y->leaf)
                y->children.resize(t);
            // Insert new child into x.
            x->children.insert(x->children.begin() + i + 1, z);
            x->keys.insert(x->keys.begin() + i, y->keys[t-1]);
        }
    };

    PostingTree* createPostingTree(const std::vector<TID>& tids);

    #endif // POSTING_TREE_H