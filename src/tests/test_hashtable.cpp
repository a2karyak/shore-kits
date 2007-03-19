/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "util/hashtable.h"
#include <string.h>

#define UNKNOWN "UNKNOWN"

const char* int_strings[] = {
    "ZERO",
    "ONE",
    "TWO",
    "THREE",
    "FOUR",
    "FIVE",
    "SIX",
    "SEVEN",
    "EIGHT",
    "NINE",
    "TEN",
    "ELEVEN",
    "TWELVE",
    "THIRTEEN",
    "FOURTEEN",
    "FIFTEEN"
    "SIXTEEN",
    "SEVENTEEN",
    "EIGHTEEN",
    "NINETEEN",
    "TWENTY",
    "TWENTY-ONE",
    "TWENTY-TWO",
    "TWENTY-THREE",
    "TWENTY-FOUR",
    "TWENTY-FIVE",
    "TWENTY-SIX",
    "TWENTY-SEVEN",
    "TWENTY-EIGHT",
    "TWENTY-NINE",
    "THIRTY",
};


class EqualInt {
public:
    bool operator()(const int a, const int b) {
        return a == b;
    }
};

class EqualIntString {
public:
    bool operator()(const char* s1, const char* s2) {
        if (s1 == NULL)
            return false;
        if (s2 == NULL)
            return false;
        if (!strcmp(s1, UNKNOWN))
            return false;
        if (!strcmp(s2, UNKNOWN))
            return false;
        return (bool)(!strcmp(s1, s2));
    }
};

class ExtractInt {
public:
    int operator()(const char* s) {
        int num_strings = sizeof(int_strings)/sizeof(int_strings[0]);
        for (int i = 0; i < num_strings; i++) {
            if ( !strcmp(s, int_strings[i]) )
                return i;
        }
        return -1;
    }
};

class HashInt {
public:
    size_t operator()(const int a) {
        return (size_t)a;
    }
};




int main(int, char**)
{
    util_init();

    TRACE(TRACE_ALWAYS, "hello world from %s\n", thread_get_self()->thread_name().data());
 
    EqualInt       equalint;
    EqualIntString equalintstring;
    ExtractInt     extractint;
    HashInt        hashint;

  
    /* REMOVE CC Compiler warnings */
    equalint = equalint;
    equalintstring = equalintstring;
    extractint = extractint;
    hashint = hashint;


    TRACE(TRACE_ALWAYS, "(5, 4) = %s\n", equalint(5, 4) ? "TRUE" : "FALSE");
    TRACE(TRACE_ALWAYS, "(5, 5) = %s\n", equalint(5, 5) ? "TRUE" : "FALSE");

  
    hashtable <const char*, int, ExtractInt, EqualInt, EqualIntString, HashInt>
        ht(40, extractint, equalint, equalintstring, hashint);
  

    /* Insert three copies of the numbers 0..9 */
    for (int t = 0; t < 3; t++) {
        for (int i = 0; i < 10; i++) {
            ht.insert_noresize(int_strings[i]);
        }
    }

    /* Check contains() against 0..19. Half should exist. */
    for (int i = 0; i < 20; i++) {
        TRACE(TRACE_ALWAYS, "ht.contains(%d) = %s\n",
              i,
              ht.contains(int_strings[i]) ? "TRUE" : "FALSE");
    }
  
    /* Check insert_unique_noresize() on 0..19. The numbers 10..19
       should be inserted successfully. */
    for (int i = 0; i < 20; i++) {
        TRACE(TRACE_ALWAYS, "ht.insert_unique_noresize(%d) = %s\n",
              i,
              ht.insert_unique_noresize(int_strings[i]) ? "TRUE" : "FALSE");
    }

    /* Check equal_range() on 0..30. The numbers in 0..9 should have
       three entries each. The numbers in 10..19 should have one entry
       each. The numbers in 20..29 should have no entries. */
    for (int i = 0; i < 30; i++) {

        std::pair
            <hashtable<const char*, int, ExtractInt, EqualInt, EqualIntString, HashInt>::iterator,
            hashtable<const char*, int, ExtractInt, EqualInt, EqualIntString, HashInt>::iterator>
            itpair = ht.equal_range(extractint(int_strings[i]));

        hashtable<const char*, int, ExtractInt, EqualInt, EqualIntString, HashInt>::iterator
            one = itpair.first;
        hashtable<const char*, int, ExtractInt, EqualInt, EqualIntString, HashInt>::iterator
            end = itpair.second;
      
        if (one == end) {
            TRACE(TRACE_ALWAYS, "No data for i = %d\n", i);
        }
        for ( ; one != end; ++one) {
            const char* v = *one;
            TRACE(TRACE_ALWAYS, "Got value %s for i = %d\n",
                  v,
                  i);
        }
    }


    TRACE(TRACE_ALWAYS, "This should lead to an abort...\n");
    ht.insert_noresize(int_strings[1]);
  
    return 0;
}
