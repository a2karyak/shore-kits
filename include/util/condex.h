/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __UTIL_CONDEX_H
#define __UTIL_CONDEX_H

#include <cstdlib>


/******************************************************************** 
 *
 * @struct: condex
 *
 * @brief:  Struct of a cond var with a flag indicating if it was fired
 *
 ********************************************************************/

struct condex 
{
    pthread_cond_t _cond;
    pthread_mutex_t _lock;
    bool _fired;

    condex() : _fired(false) { 
        if (pthread_cond_init(&_cond,NULL)) {
            assert (0); // failed to init cond var
        }
        if (pthread_mutex_init(&_lock,NULL)) {
            assert (0); // failed to init mutex
        }
    }

    ~condex() {
        pthread_cond_destroy(&_cond);
        pthread_mutex_destroy(&_lock);
    }

    void signal() {
	CRITICAL_SECTION(cs, _lock);
	_fired = true;
	pthread_cond_signal(&_cond);
    }

    void wait() {
	CRITICAL_SECTION(cs, _lock);
	while(!_fired)
	    pthread_cond_wait(&_cond,&_lock);
	_fired = false;
    }

}; // EOF: condex



/******************************************************************** 
 *
 * @struct: condex_pair
 *
 * @brief:  Struct of two condexes with a ready flag and an pointer to
 *          the active condex
 *
 ********************************************************************/

struct condex_pair 
{
    condex wait[2];
    int index;
    bool take_one;

    condex_pair() : index(0), take_one(false) { }

    ~condex_pair() { }

};  // EOF: condex_pair


// my_condex = {
//     {
// 	{PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, false},
// 	{PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, false},
//     },
//     0, false
// };



#endif /** __UTIL_CONDEX_H */
