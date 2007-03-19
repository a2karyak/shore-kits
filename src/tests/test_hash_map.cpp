/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include <map>
#include <string>

using std::string;



int main(int, char**)
{
    util_init();

    const char* key = "hello";


    std::map<string, int> s_string;
    s_string[key] = 5;
  
    TRACE(TRACE_ALWAYS, "s_string[\"%s\"] maps to %d\n",
          key,
          s_string[key]);
  

    std::map<c_str, int> s_cstr;
    s_cstr[c_str(key)] = 5;

    s_cstr.find(key);
    TRACE(TRACE_ALWAYS, "s_cstr[\"%s\"]   maps to %d\n",
          key,
          s_cstr[c_str(key)]);
  
    return 0;
}
