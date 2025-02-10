#ifndef GINBTREE_NODE_H
#define GINBTREE_NODE_H

#include "tid.h"
#include <vector>


struct BTreeNode {
    bool leaf;
    std::vector<TID> keys;
    std::vector<BTreeNode*> children;
    BTreeNode(bool isLeaf);
    ~BTreeNode();
};

#endif // GINBTREE_NODE_H