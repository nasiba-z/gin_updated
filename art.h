// ART.h
#ifndef ART_H
#define ART_H

#include <vector>
#include <cstring>
#include <cassert>
#include "gin_index_art.h"
#include "gin_state.h"
#include "tid.h"
constexpr size_t LEAF_THRESHOLD =4;

// Enumeration for node types.
enum class NodeType { NODE4, NODE16, NODE48, NODE256, LEAF };

// Base class for all ART nodes.
class ARTNode {
public:
    NodeType type;
    std::vector<unsigned char> prefix; // Compressed prefix (can be empty)
    int prefixLen;                      // Number of prefix bytes
    ARTNode(NodeType t) : type(t), prefixLen(0) {}
    virtual ~ARTNode() {}

    // Insert a key/value pair starting at a given depth.
    virtual ARTNode* insert(const unsigned char* key, int keyLen, int depth, IndexTuple* value) = 0;
    virtual IndexTuple* search(const unsigned char* key, int keyLen, int depth) const = 0;
};

// Leaf node storing the full key and its associated value.
class ARTLeaf : public ARTNode {
    public:
        std::vector<unsigned char> fullKey;
        IndexTuple* value;   // Change this from int to IndexTuple*
    
        ARTLeaf(const std::vector<unsigned char>& key, IndexTuple* v);
        ARTNode* insert(const unsigned char* key, int keyLen, int depth, IndexTuple* newValue) override;
        IndexTuple* search(const unsigned char* key, int keyLen, int depth) const override;
    };

// Node4 class.
class ARTNode4 : public ARTNode {
public:
    unsigned char keys[4];
    ARTNode* children[4];
    int count;

    ARTNode4();
    ~ARTNode4() override;
    ARTNode* insert(const unsigned char* key, int keyLen, int depth, IndexTuple* value) override;
    IndexTuple* search(const unsigned char* key, int keyLen, int depth) const override;
};

// Node16: Up to 16 children. Similar to Node4, but with capacity 16.
class ARTNode16 : public ARTNode {
public:
    unsigned char keys[16];   // Array for keys.
    ARTNode* children[16];    // Array for child pointers.
    int count;                // Number of children.

    ARTNode16();
    ~ARTNode16() override;
    ARTNode* insert(const unsigned char* key, int keyLen, int depth, IndexTuple* value) override;
    IndexTuple* search(const unsigned char* key, int keyLen, int depth) const override;
};

// Node48: Uses a 256-element index array to map key bytes to a secondary array of up to 48 children.
class ARTNode48 : public ARTNode {
public:
    unsigned char childIndex[256]; // Stores indexes that point to'children' array.
    ARTNode* children[48];         // For now, array for up to 48 child pointers
    int count;                     // Number of children.

    ARTNode48();
    ~ARTNode48() override;
    ARTNode* insert(const unsigned char* key, int keyLen, int depth, IndexTuple* value) override;
    IndexTuple* search(const unsigned char* key, int keyLen, int depth) const override;
};

// Node256: Uses a direct array of 256 child pointers.
class ARTNode256 : public ARTNode {
public:
    ARTNode* children[256];        // Direct mapping from key byte to child. All are pointers.
    int count;                     // Number of children.

    ARTNode256();
    ~ARTNode256() override;
    ARTNode* insert(const unsigned char* key, int keyLen, int depth, IndexTuple* value) override;
    IndexTuple* search(const unsigned char* key, int keyLen, int depth) const override;
};

// Function prototype: Bulk-load ART from sorted key/value pairs.
// Each item is a pair: { key (vector<unsigned char>), value (int) }.
// It is assumed that the items are sorted lexicographically by key.
ARTNode* ART_bulkLoad(const std::vector<std::pair<std::vector<unsigned char>, IndexTuple*>>& items, int depth);

#endif // ART_H
