/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file   confing_file_example.cpp
 *
 *  @brief  Program to demonstrate ConfigFile class
 */

#include "sm/shore/shore_conf.h"

using namespace shore;

using std::string;
using std::cout;
using std::cerr;
using std::endl;


int main( void )
{
  // A configuration file can be loaded with a simple
	
    ConfigFile config( "example.inp" );
	
    // Values can be read from the file by name
    try {
	
        int apples;
        config.readInto( apples, "apples" );
        cout << "The number of apples is " << apples << endl;
	
        double price;
        config.readInto( price, "price" );
        cout << "The price is $" << price << endl;
        string title;
        config.readInto( title, "title" );
        cout << "The title of the song is " << title << endl;
	
        // We can provide default values in case the name is not found
	
        int oranges;
        config.readInto( oranges, "oranges", 0 );
        cout << "The number of oranges is " << oranges << endl;
	
        int fruit = 0;
        fruit += config.read( "apples", 0 );
        fruit += config.read( "pears", 0 );
        fruit += config.read( "oranges", 0 );
        cout << "The total number of apples, pears, and oranges is ";
        cout << fruit << endl;
	
        // Sometimes we must tell the compiler what data type we want to
        // read when it's not clear from arguments given to read()
	
        int pears = config.read<int>( "pears" );
        cout << "The number of pears is " << pears;
        cout << ", but you knew that already" << endl;
	
        // The value is interpreted as the requested data type
	
        cout << "The weight is ";
        cout << config.read<string>("weight");
        cout << " as a string" << endl;
	
        cout << "The weight is ";
        cout << config.read<double>("weight");
        cout << " as a double" << endl;
	
        cout << "The weight is ";
        cout << config.read<int>("weight");
        cout << " as an integer" << endl;
	
        // When reading boolean values, a wide variety of words are
        // recognized, including "true", "yes", and "1"
	
        if( config.read( "sale", false ) )
            cout << "The fruit is on sale" << endl;
        else
            cout << "The fruit is full price" << endl;
	
        // We can also read user-defined types, as long as the input and
        // output operators, >> and <<, are defined
	
	
        // The readInto() functions report whether the named value was found
	
        int pommes = 0;
        if( config.readInto( pommes, "pommes" ) )
            cout << "The input file is in French:  ";
        else if( config.readInto( pommes, "apples" ) )
            cout << "The input file is in English:  ";
        cout << "The number of pommes (apples) is " << pommes << endl;
	
        // Named values can be added to a ConfigFile
	
        config.add( "zucchini", 12 );
        int zucchini = config.read( "zucchini", 0 );
        cout << "The number of zucchini was set to " << zucchini << endl;
	
        // And values can be removed
	
        config.remove( "pears" );
        if( config.readInto( pears, "pears" ) )
            cout << "The pears are ready" << endl;
        else
            cout << "The pears have been eaten" << endl;
	
        // An entire ConfigFile can written (and restored)
	
        cout << "Here is the modified configuration file:" << endl;
        cout << config;

        cout << "Writing to initial config file" << endl;
        config.saveCurrentConfig();

    }
    catch (...) {
        cerr << "Exception thrown..." << endl;
    }
	
    return 0;
}
