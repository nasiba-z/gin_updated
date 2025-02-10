// tid.h
#ifndef TID_H
#define TID_H

#include <string>

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
};

#endif // TID_H