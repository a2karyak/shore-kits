/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file trx_driver.h
 *
 *  @brief Interface for the drivers of transaction clients.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TRX_DRIVER_H
#define __TRX_DRIVER_H


#include "stages/tpcc/common/trx_packet.h"

#include "workload/driver.h"
#include "workload/driver_directory.h"


using namespace qpipe;


ENTER_NAMESPACE(workload);


class trx_driver_t : public driver_t {

private:

    /** @note All the trx drivers should implement that interface */

    virtual void allocate_dbts()=0;
    virtual void deallocate_dbts()=0;
    virtual void reset_dbts()=0;

public:

    // whRange governs the range of warehouses queried by the client
    // the default value is RANGE
    int _whRange;

    // Indicates if the required placeholders for the Dbts have been
    // allocated
    int _allocated;


    trx_driver_t(const c_str& description)
        : driver_t(description)
    {
        // FIXME (ip) It is dangeround to call allocate_dbts here
        // allocate_dbts();
        _allocated = 0;
    }

    virtual ~trx_driver_t() { }

    virtual void submit(void* disp)=0;

    inline void setRange(const int aRange) {
        assert (aRange > 0);
        _whRange = aRange;
    }

};

EXIT_NAMESPACE(workload);

#endif
