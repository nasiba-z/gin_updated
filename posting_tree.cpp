#include "posting_tree.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <queue>

// ------------------------------------------------------------------
// BTreeNode Implementation
// ------------------------------------------------------------------
BTreeNode::BTreeNode(bool isLeaf) : leaf(isLeaf) {
    // keys and children are default-initialized.
}

BTreeNode::~BTreeNode() {
    for (auto child : children)
        delete child;
}

// ------------------------------------------------------------------
// PostingTree Implementation
// ------------------------------------------------------------------
PostingTree::PostingTree() : root(nullptr) {}

PostingTree::~PostingTree() {
    if (root)
        delete root;
}

// ------------------------------------------------------------------
// Helper function: Build leaf nodes from sorted TIDs (bulkLoad approach).
// ------------------------------------------------------------------
std::vector<BTreeNode*> PostingTree::buildLeafNodes(const std::vector<TID>& sortedTIDs) {
    std::vector<BTreeNode*> leaves;
    size_t n = sortedTIDs.size();
    size_t i = 0;
    while (i < n) {
        // Determine how many TIDs to put in this leaf node.
        // We aim for a node size close to LeafTargetCount, but we cannot exceed LeafMaxCount.
        size_t count = LeafTargetCount;
        //print LeafTargetCount, LeafMaxCount, LeafMinCount, count, n, i);
        // std::cout<<LeafTargetCount<<','<<LeafMaxCount<<','<<LeafMinCount<<','<<n<<std::endl;
        if (i + count > n)
            count = n - i;
        // If count is less than the minimum and leaves already exist, merge with previous leaf.
        if (!leaves.empty() && count < LeafMinCount) {
            BTreeNode* lastLeaf = leaves.back();
            lastLeaf->keys.insert(lastLeaf->keys.end(), sortedTIDs.begin() + i, sortedTIDs.end());
            break;
        }
        // Ensure we do not exceed the maximum.
        if (count > LeafMaxCount)
            count = LeafMaxCount;
        BTreeNode* leaf = new BTreeNode(true);
        leaf->keys.insert(leaf->keys.end(), sortedTIDs.begin() + i, sortedTIDs.begin() + i + count);
        leaves.push_back(leaf);
        i += count;
    }
    return leaves;
}

// ------------------------------------------------------------------
// Helper function: Build internal nodes from a vector of child nodes (bulkLoad). One reference was good enough
// avoid copying the vector of children. The move was not necessary, but it was used to avoid copying the vector of children
// TODO: Revert to not using move, alternatively debug with breakpoints.
// ------------------------------------------------------------------
// In posting_tree.h, change the signature back to take a const ref:
BTreeNode* buildInternalLevel(const std::vector<BTreeNode*>& children);

// In posting_tree.cpp:
BTreeNode*
PostingTree::buildInternalLevel(const std::vector<BTreeNode*>& children)
{
    // Base case: if there’s only one subtree, that’s your root
    if (children.size() == 1)
        return children[0];

    std::vector<BTreeNode*> parents;
    constexpr size_t B = 16;         // your branching factor
    size_t n = children.size();
    size_t i = 0;

    // Group children into batches of up to B
    while (i < n)
    {
        size_t count = std::min(B, n - i);
        BTreeNode* parent = new BTreeNode(false);

        // Copy the next `count` child pointers into this parent
        parent->children.insert(
            parent->children.end(),
            children.begin() + i,
            children.begin() + i + count);

        // For each child after the first, pick its first key as a separator
        for (size_t j = 1; j < count; ++j)
        {
            parent->keys.push_back(parent->children[j]->keys.front());
        }

        parents.push_back(parent);
        i += count;
    }

    // Recurse on the newly‐built parents vector
    return buildInternalLevel(parents);
}


// ------------------------------------------------------------------
// Bulk-load: Build the PostingTree from a sorted vector of TIDs.
// ------------------------------------------------------------------
void PostingTree::bulkLoad(const std::vector<TID>& sortedTIDs) {
    std::vector<BTreeNode*> leaves = buildLeafNodes(sortedTIDs);
    root = buildInternalLevel(leaves);
}

// ------------------------------------------------------------------
// getTotalSize: Compute the total "logical" size of the posting tree.
// ------------------------------------------------------------------
size_t PostingTree::getTotalSize() const {
    size_t total = 0;
    std::function<void(BTreeNode*)> traverse = [&](BTreeNode* node) {
        if (node->leaf) {
            total += sizeof(BTreeNode) + node->keys.size() * sizeof(TID);
        } else {
            for (BTreeNode* child : node->children)
                traverse(child);
        }
    };
    if (root)
        traverse(root);
    return total;
}

// ------------------------------------------------------------------
// Incremental Insertion Functions
// ------------------------------------------------------------------

// Public method: Insert a TID into the tree.
void PostingTree::insert(const TID& tid) {
    if (root == nullptr) {
        root = new BTreeNode(true);
    }
    // If the root is full, split it and create a new root.
    if (root->keys.size() >= LeafMaxCount) {
        BTreeNode* newRoot = new BTreeNode(false);
        newRoot->children.push_back(root);
        splitChild(newRoot, 0);
        root = newRoot;
    }
    insertNonFull(root, tid);
}

// Helper: Recursively insert tid into a node that is assumed not full.
void PostingTree::insertNonFull(BTreeNode* node, const TID& tid) {
    if (node->leaf) {
        auto pos = std::lower_bound(node->keys.begin(), node->keys.end(), tid);
        node->keys.insert(pos, tid);
    } else {
        int i = node->keys.size() - 1;
        while (i >= 0 && tid < node->keys[i])
            i--;
        i++; // child index to descend.
        if (node->children[i]->keys.size() >= LeafMaxCount) {
            splitChild(node, i);
            if (tid > node->keys[i])
                i++;
        }
        insertNonFull(node->children[i], tid);
    }
}

// Helper: Split the full child node at index i of the given parent.
void PostingTree::splitChild(BTreeNode* parent, size_t i) {
    BTreeNode* child = parent->children[i];
    BTreeNode* sibling = new BTreeNode(child->leaf);
    size_t mid = child->keys.size() / 2;
    TID median = child->keys[mid];

    // Sibling gets keys from mid+1 to end.
    sibling->keys.assign(child->keys.begin() + mid + 1, child->keys.end());
    child->keys.resize(mid);
    
    if (!child->leaf) {
        sibling->children.assign(child->children.begin() + mid + 1, child->children.end());
        child->children.resize(mid + 1);
    }
    
    parent->keys.insert(parent->keys.begin() + i, median);
    parent->children.insert(parent->children.begin() + i + 1, sibling);
}

// ------------------------------------------------------------------
// Search Function
// ------------------------------------------------------------------
static bool searchInNode(BTreeNode* node, const TID& k) {
    auto it = std::lower_bound(node->keys.begin(), node->keys.end(), k);
    int idx = it - node->keys.begin();
    if (it != node->keys.end() && *it == k)
        return true;
    if (node->leaf)
        return false;
    return searchInNode(node->children[idx], k);
}

bool PostingTree::search(const TID& k) const {
    if (!root)
        return false;
    return searchInNode(root, k);
}

// ------------------------------------------------------------------
// Create Posting Tree from a vector of TIDs,
// mimicking createPostingTree from GIN.
// ------------------------------------------------------------------
void PostingTree::createFromVector(const std::vector<TID>& items) {
    if (items.empty())
        return;
    
    // Define an inline limit, simulating the maximum number of TIDs that fit in one "page."
    const size_t inlineLimit = LeafTargetCount;  // Use LeafTargetCount as the inline limit.
    
    // Initialize root as a leaf node.
    root = new BTreeNode(true);
    
    size_t nrootitems = 0;
    // Copy as many TIDs as fit into the root.
    while (nrootitems < items.size() && nrootitems < inlineLimit) {
        root->keys.push_back(items[nrootitems]);
        nrootitems++;
    }
    
    // If there are remaining TIDs, insert them incrementally.
    for (size_t i = nrootitems; i < items.size(); i++) {
        insert(items[i]);
    }
   
}

// Helper function to traverse the tree and collect TIDs from leaf nodes.
void collectTIDs(BTreeNode* node, std::vector<TID>& tids) {
    if (!node) return;
    if (node->leaf) {
        // If the node is a leaf, add all its keys (TIDs) to the result.
        tids.insert(tids.end(), node->keys.begin(), node->keys.end());
    } else {
        // If the node is not a leaf, recursively collect TIDs from its children.
        for (BTreeNode* child : node->children) {
            collectTIDs(child, tids);
        }
    }
}

// New no-argument version of getTIDs.
std::vector<TID> PostingTree::getTIDs() const {
    std::vector<TID> tids;
    collectTIDs(root, tids);
    return tids;
}
