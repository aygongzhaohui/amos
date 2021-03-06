/**
 * @file basedef.h
 * @brief
 * @author GongZhaohui
 * @version
 * @date 2016-02-18
 */

#ifndef _AMOS_BASEDEF_H_
#define _AMOS_BASEDEF_H_

#include <time.h>
#include <vector>
#include <set>

#define MAX_INTTYPE_VAL(type) (~(1L << (sizeof(type) * 8 - 1)))
#define INVALID_HANDLE    (-1)

namespace amos
{

    typedef int HANDLE;

    typedef unsigned long TIMER;

    typedef std::vector<TIMER> TimerVec;
    typedef TimerVec::iterator TimerVecIter;
    typedef std::set<TIMER> TimerSet;
    typedef TimerSet::iterator TimerSetIter;

    typedef long MSEC;

    typedef unsigned EvMask;

    typedef struct timespec TIMESTAMP;

    /**
     * @brief    diff two timestamp
     *
     * @param    t1
     * @param    t2
     *
     * @return    calculate (t2 - t1) in millisec
     */
    MSEC TimeDiff(const TIMESTAMP & t1, const TIMESTAMP t2);

    void TimeNow(TIMESTAMP & t);

    /**
     * @brief    diff time between now
     *
     * @param    past
     *          can't be large than now
     *
     * @return    milliseconds elapse
     *          exception return -1
     */
    MSEC TimeDiffNow(const TIMESTAMP & past);

}






#endif

