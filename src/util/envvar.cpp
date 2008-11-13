/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   envvar.cpp
 *
 *  @brief:  "environment variables" singleton class
 *
 *  @author: Ippokratis Pandis (ipandis)
 */


#include "util/envvar.h"


/*********************************************************************
 *
 *  @fn:    setVar/getVar/readConfVar
 *  
 *  @brief: Environment variables manipulation
 *          
 *********************************************************************/


// helper: reads the conf file for a specific param
// !!! the caller should have the lock !!!
string envVar::_readConfVar(const string& sParam, const string& sDefValue)
{
    if (sParam.empty()||sDefValue.empty()) {
        TRACE( TRACE_ALWAYS, "Invalid Param or Value input\n");
        return ("");
    }
    assert (_pfparser);
    string tmp;
    // probe config file
    _pfparser->readInto(tmp,sParam,sDefValue); 
    // set entry in the env map
    _evm[sParam] = tmp;
    TRACE( TRACE_DEBUG, "(%s) (%s)\n", sParam.c_str(), tmp.c_str());
    return (tmp);
}



// sets a new parameter
const int envVar::setVar(const string& sParam, const string& sValue)
{
    if ((!sParam.empty())&&(!sValue.empty())) {
        TRACE( TRACE_DEBUG, "(%s) (%s)\n", sParam.c_str(), sValue.c_str());
        CRITICAL_SECTION(evm_cs,_lock);
        _evm[sParam] = sValue;
        return (_evm.size());
    }
    return (0);
}

const int envVar::setVarInt(const string& sParam, const int& iValue)
{    
    return (setVar(sParam,_toString(iValue)));
}



// refreshes all the env vars from the conf file
const int envVar::refreshVars(void)
{
    TRACE( TRACE_DEBUG, "Refreshing environment variables\n");
    CRITICAL_SECTION(evm_cs,_lock);
    for (envVarIt it= _evm.begin(); it != _evm.end(); ++it)
        _readConfVar(it->first,it->second);    
    return (0);
}



// checks the map for a specific param
// if it doesn't find it checks also the config file
string envVar::getVar(const string& sParam, const string& sDefValue)
{
    if (sParam.empty()) {
        TRACE( TRACE_ALWAYS, "Invalid Param input\n");
        return ("");
    }

    CRITICAL_SECTION(evm_cs,_lock);
    envVarIt it = _evm.find(sParam);
    if (it==_evm.end()) {        
        //TRACE( TRACE_DEBUG, "(%s) param not set. Searching conf\n", sParam.c_str()); 
        return (_readConfVar(sParam,sDefValue));
    }
    return (it->second);
}

int envVar::getVarInt(const string& sParam, const int& iDefValue)
{
    return (atoi(getVar(sParam,_toString(iDefValue)).c_str()));
}



// checks if a specific param is set at the map or (fallback) the conf file
void envVar::checkVar(const string& sParam)
{
    string r;
    CRITICAL_SECTION(evm_cs,_lock);
    // first searches the map
    envVarIt it = _evm.find(sParam);
    if (it!=_evm.end()) {
        r = it->second + " (map)";
    }
    else {
        // if not found on map, searches the conf file
        if (_pfparser->keyExists(sParam)) {
            _pfparser->readInto(r,sParam,string("Not found"));
            r = r + " (conf)";
        }
        else {
            r = string("Not found");
        }        
    }
    TRACE( TRACE_ALWAYS, "%s -> %s\n", sParam.c_str(), r.c_str()); 
}


string envVar::getSysname()
{
    string config = getVar("db-config","invalid");
    config = config + "-system";
    return (getVar(config,"invalid"));
}




// prints all the env vars
void envVar::printVars(void)
{
    TRACE( TRACE_DEBUG, "Environment variables\n");
    CRITICAL_SECTION(evm_cs,_lock);
    for (envVarConstIt cit= _evm.begin(); cit != _evm.end(); ++cit)
        TRACE( TRACE_DEBUG, "%s -> %s\n", cit->first.c_str(), cit->second.c_str()); 
}



// sets as input another conf file
void envVar::setConfFile(const string& sConfFile)
{
    assert (!sConfFile.empty());
    CRITICAL_SECTION(evm_cs,_lock);
    _cfname = sConfFile;
    _pfparser = new ConfigFile(_cfname);
    assert (_pfparser);
}



// parses a SET request
const int envVar::parseOneSetReq(const string& in)
{
    string param;
    string value;
    boost::char_separator<char> valuesep("=");            

    tokenizer valuetok(in, valuesep);  
    tokit vit = valuetok.begin();
    if (vit == valuetok.end()) {
        TRACE( TRACE_DEBUG, "(%s) is malformed\n", in);
        return(1);
    }
    param = *vit;
    ++vit;
    if (vit == valuetok.end()) {
        TRACE( TRACE_DEBUG, "(%s) is malformed\n", in);
        return(2);
    }
    value = *vit;
    TRACE( TRACE_DEBUG, "%s -> %s\n", param.c_str(), value.c_str()); 
    CRITICAL_SECTION(evm_cs,_lock);
    _evm[param] = value;
    return(0);
}
    


// parses a string of (multiple) SET requests
const int envVar::parseSetReq(const string& in)
{
    int cnt=0;

    // FORMAT: SET [<clause_name>=<clause_value>]*
    boost::char_separator<char> clausesep(" ");
    tokenizer clausetok(in, clausesep);
    tokit cit = clausetok.begin();
    ++cit; // omit the SET cmd
    for (; cit != clausetok.end(); ++cit) {
        parseOneSetReq(*cit);
        ++cnt;
    }
    return (cnt);
}


