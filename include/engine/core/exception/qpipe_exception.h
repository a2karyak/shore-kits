
#ifndef __QPIPE_EXCEPTION_H
#define __QPIPE_EXCEPTION_H

#include <exception>
#include <string>
#include <sstream>


using std::exception;
using std::string;
using std::ostringstream;



#include "engine/namespace.h"



class QPipeException : public std::exception
{

private:

  string _message;


  static string IntToString(int num)
  {
    // creates an ostringstream object
    ostringstream myStream;
    myStream << num;
    myStream.flush();

    /*
     * outputs the number into the string stream and then flushes
     * the buffer (makes sure the output is put into the stream)
     */
  
    // returns the string form of the stringstream object
    return(myStream.str());
  }

public:

  QPipeException(const string& m,
                 const char* filename, int line_num, const char* function_name)
    : _message(string(filename)
               + ":" + string(function_name)
               + ":" + IntToString(line_num)
               + ":" + m)
  {
  }

  virtual const char* what() const throw() {
    return _message.c_str();
  }
 
  virtual ~QPipeException() throw() { }
};



#include "engine/namespace.h"



#endif
