// tid.h
#ifndef TID_H
#define TID_H

#include <string>
#include <unordered_map>
extern std::unordered_map<int, std::string> rowData;

struct TID {
    // we don't need pageId as we are working in memory
    int rowId;  // Here, we use the trigram integer

    TID() : rowId(0) {}
    TID( int r) :  rowId(r) {}

    //for easy storage
    std::string toString() const ;

    //for comparison later
    bool operator>(const TID& other) const;

    bool operator<(const TID& other) const;
    // Define equality operator:
    bool operator==(const TID& other) const;
};
std::string getRowText(const TID &tid);
#endif // TID_H