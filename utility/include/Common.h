#pragma once
// Common code used by the various hashing sample applications.
#include "Vector.h"
#include "mString.h"
#include <cstring>
typedef uint64_t fnvHash_t;
// You may define additional helper routines here and in
// Common.cpp.



// Build a HashTable of strings and numbers of occurrences, given a vector
// of strings representing the words.  You may assume the vector and the
// strings will not be changed during the lifetime of the Hash.

// You may add additional default parameters.

// Caller is responsible for deleting the Hash.

//HashTable<const char *, size_t> *BuildHashTable(const vector<string> &words);


// Collect words read from a file specified on the command line
// as either individual word or whole lines in a vector of
// strings.

//void CollectWordsIn(int argc, char **argv, vector<string> &words);

// -v (verbose) command line option.

//extern bool optVerbose;

static fnvHash_t fnvHash( const char *data, size_t length )
    {
    static const size_t FnvOffsetBasis = 146959810393466560;
    static const size_t FnvPrime = 1099511628211ul;
    size_t hash = FnvOffsetBasis;
    for (size_t i = 0; i < length; ++i)
        {
        hash *= FnvPrime;
        hash ^= data[i];
        }
    return hash;
    }

static bool CompareEqual( const char *L, const char *R )
    {
    // Your code here.
    while (*L != '\0' || *R != '\0')
        {
        if (*L != *R)return false;
        L += 1;
        R += 1;
        }
    return true;
    }

static bool CompareEqual(const String &L,const String &R) {
    //cout << "In static ocmpare qequal" << endl;
    //std::cout << "comparing: " << L.cstr() << " and " << R.cstr() << std::endl;
    bool ret = strcmp(L.cstr(),R.cstr()) == 0;
    //cout << "Returning with : " << ret << endl;
    return ret;
}

template< class Type >
void Swap( Type& x, Type& y ) noexcept
    {
    Type tmp = x;
    x = y;
    y = tmp;
    }


extern bool optVerbose;
