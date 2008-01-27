/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_msg.h
 *
 *  @brief:  Messaging mechanism
 *
 *  @author: Mengzhi Wang, April 2001
 *  @author: Ippokratis Pandis, January 2008
 *
 */

#ifndef __SHORE_MSG_H
#define __SHORE_MSG_H

#include "util.h"

ENTER_NAMESPACE(shore);


/** Messaging/Tracing mechanism */

struct Message {
        Message( const char * s ) {
                cout << "SHORE: " << s << endl;
	}
        Message( const char * pfx, const char * s ) {
                cout << "SHORE: " << pfx << s << endl;
        }
};

#define PFX_MESSAGE(pfx,s) { static Message msg ## __LINE__ ((pfx),(s)); }
#define MESSAGE(s) { static Message msg ## __LINE__ ((s)); }
#define NEWO_MESSAGE(s) { static Message msg ## __LINE__ ("APP:NO: ",(s)); }
#define PAY_MESSAGE(s) { static Message msg ## __LINE__ ("APP:PAY: ",(s)); }
#define SLEV_MESSAGE(s) { static Message msg ## __LINE__ ("APP:SL: ",(s)); }
#define DELV_MESSAGE(s) { static Message msg ## __LINE__ ("APP:DEL: ",(s)); }
#define ORDS_MESSAGE(s) { static Message msg ## __LINE__ ("APP:OS: ",(s)); }


EXIT_NAMESPACE(shore);

#endif /* __SHORE_MSG_H */
