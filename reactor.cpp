/**
 * @file reactor.cpp
 * @brief
 * @author GongZhaohui
 * @version
 * @date 2016-02-21
 */

#include "logger.h"
#include "reactor.h"

using namespace amos;

Reactor::Reactor(ReactorImpl * impl)
    : impl_(impl), loop_(false)
{
    if (impl_)
        loop_ = true;
}

void Reactor::PollIOEvents(RegHandlerVec & list)
{
    MSEC nextTimeout = 0;
    // if there are pending events, it will poll without
    // waiting for timeout
    if (list.size() == 0)
    {
        nextTimeout = timerQ_.GetTimeout();
        if (nextTimeout > DEFAULT_REACT_INTERVAL)
            nextTimeout = DEFAULT_REACT_INTERVAL;
    }
    impl_->Demultiplex(handlerMap_, list, nextTimeout);
}

void Reactor::RunEventLoop()
{
    while (loop_)
    {
        //1.poll I/O events
        PollIOEvents(evList_);
        //2.poll timer events
        timerQ_.Schedule(evList_);
        //3.handle all events
        HandleEvents(evList_);
    }
}

void Reactor::ProcessOneHandler(RegHandler & rh)
{
    // 1. get all returned events
    EvMask ev = rh.revents; rh.revents = 0;
    TimerVec tv; tv.swap(rh.timeout_list);
    EventHandler * handler = rh.handler;
    assert(handler);
    if (ev == 0 || !handler) return;
    //2.handle all timeout timer
    if (ev & EventHandler::TIMER_MASK)
    {
        assert(tv.size() > 0);
        if (tv.size() > 0)
        {
            TimerVecIter titer = tv.begin();
            // traverse all timeid appended
            for (; titer != tv.end(); ++titer)
            {// HandleTimeout
                TIMER id = *titer;
                int ret = handler->HandleTimeout(id);
                if (ret)
                {// need remove timer
                    timerQ_.RemoveTimer(id);
                    rh.timers.erase(id);
                }
                else ResetTimer(id);// reschedule the timer if ret==0
            }
        }
    }
    //3.handle all I/O evetns
    EvMask closeMask = 0;
    if (ev & EventHandler::ERROR_MASK)
    {
        closeMask |= EventHandler::ERROR_MASK;
    }
    else
    {
        if(ev & EventHandler::READ_MASK)
        {
            int ret = handler->HandleInput(handler->Handle());
            if (ret)
                closeMask |= EventHandler::READ_MASK;
        }
        if (ev & EventHandler::WRITE_MASK)
        {
            int ret = handler->HandleOutput(handler->Handle());
            if (ret)
                closeMask |= EventHandler::WRITE_MASK;
        }
    }
    if (closeMask)
    {
        rh.state = EventHandler::CLOSED_STAT;
        handler->HandleClose(handler->Handle(), closeMask);
    }
    rh.revents = 0;
}

void Reactor::HandleEvents(RegHandlerVec & l)
{
    RegHandlerVec list;
    if (l.size() > 0)
    {
        list.swap(l); // clear all pending events
        RegHandlerVecIter iter;
        for (iter = list.begin(); iter != list.end(); ++iter)
        {
            RegHandler * p = *iter;
            if (p) ProcessOneHandler(*p);
        }
    }
}

int Reactor::RegisterHandler(EventHandler * p,
        EvMask mask, EventHandlerCreator * creator)
{
    assert(p);
    if (!p || !loop_) return -1;
    // not in handler map
    EventHandlerMapIter iter = handlerMap_.find(p->Handle());
    if (iter == handlerMap_.end())
    {
        int ret = 0;
        if ((mask & EventHandler::NOIO_MASK) == 0)
            ret = impl_->RegisterHandle(p->Handle(), mask);
        if (!ret)
        {
            RegHandler& rh = handlerMap_[p->Handle()];
            rh.handler = p;
            rh.events = mask;
            p->SetDeleter(creator);
            p->AddRef(); // add the ref count
            p->reactor(this);// set reactor to handler
            return 0;
        }
        LOG_DEBUG("Reg handler failed");
        return -1;
    }
    // modify the registered event if is in the handler map
    RegHandler & rh = iter->second;
    if (rh.handler != p)
    {// TODO log
        LOG_DEBUG("Wrong handler ptr");
        return -1;
    }
    // has NOIO_MASK then couldn't modify I/O events
    if (rh.events & EventHandler::NOIO_MASK) return -1;
    EvMask reg = (mask ^ rh.events) & mask;
    if (reg > 0)
    {
        rh.events |= mask;
        if (rh.state == EventHandler::NORMAL_STAT)
            return impl_->ModifyEvents(p->Handle(), rh.events);
    }
    return 0;
}

/**
 * @brief 删除Handler,如果无注册事件的话
 *        handler将会被移除
 *
 * @param    p
 * @param    mask
 *
 * @return
 */
int Reactor::RemoveHandler(EventHandler * p, EvMask mask)
{
    assert(p);
    if (!p || !loop_) return -1;
    return RemoveHandler(p, p->Handle(), mask);
}

int Reactor::RemoveHandler(EventHandler * p, HANDLE h, EvMask mask)
{
    assert(p);
    if (!p || !loop_) return -1;
    if (h == INVALID_HANDLE)
    {
        LOG_ERROR("[RemoveHandler] Remove handler failed, invalid handle");
        return -1;
    }
    EventHandlerMapIter iter = handlerMap_.find(h);
    if (iter == handlerMap_.end()) return 0;
    RegHandler & rh = iter->second;
    if (rh.handler != p)
    {// TODO log
        LOG_ERROR("[RemoveHandler] unmatch the handler");
        return -1;
    }
    EvMask reg = rh.events & mask;
    if (reg == rh.events || p->Handle() == INVALID_HANDLE)
    {// remove handler
        impl_->RemoveHandle(h);
        // clear all the timer on this handler
        TimerSetIter titer = rh.timers.begin();
        for (; titer != rh.timers.end(); ++titer)
            timerQ_.RemoveTimer(*titer);
        p->DelRef();// decrease the ref count
        handlerMap_.erase(iter);
        LOG_INFO("Handler=0x%lx removed", (long)p);
        return 0;
    }
    else if (reg > 0)
    {// remove registered events
        rh.events ^= reg;
        if (rh.state == EventHandler::NORMAL_STAT)
            impl_->ModifyEvents(p->Handle(), rh.events);
    }
    return 0;
}

int Reactor::SuspendHandler(EventHandler * p)
{
    assert(p);
    if (!p || !loop_) return -1;
    EventHandlerMapIter iter = handlerMap_.find(p->Handle());
    if (iter == handlerMap_.end()) return -1;
    RegHandler & rh = iter->second;
    if (rh.handler != p)
    {// TODO log
        return -1;
    }
    if (rh.state == EventHandler::NORMAL_STAT)
    {
        impl_->ModifyEvents(p->Handle(), EventHandler::NONE_MASK);
        rh.state = EventHandler::SUSPEND_STAT;
    }
    return 0;
}

int Reactor::ResumeHandler(EventHandler * p)
{
    assert(p);
    if (!p || !loop_) return -1;
    EventHandlerMapIter iter = handlerMap_.find(p->Handle());
    if (iter == handlerMap_.end()) return -1;
    RegHandler & rh = iter->second;
    assert(rh.handler == p);
    if (rh.handler != p)
    {// TODO log
        return -1;
    }
    if (rh.state == EventHandler::SUSPEND_STAT)
    {
        impl_->ModifyEvents(p->Handle(), rh.events);
        rh.state = EventHandler::NORMAL_STAT;
    }
    return 0;
}

int Reactor::TriggerHandler(EventHandler * p, EvMask mask)
{
    assert(p);
    if (!p || !loop_) return -1;
    EventHandlerMapIter iter = handlerMap_.find(p->Handle());
    if (iter == handlerMap_.end()) return -1;
    RegHandler & rh = iter->second;
    assert(rh.handler == p);
    if (rh.handler != p)
    {// TODO log
        return -1;
    }
    if (rh.state == EventHandler::NORMAL_STAT)
    {
        bool addToList = true;
        // already in the return list
        if (rh.revents > 0) addToList = false;
        rh.revents |= (mask & EventHandler::ALL_MASK);
        // add to the list
        if (rh.revents > 0 && addToList)
            evList_.push_back(&rh);
        return 0;
    }
    return -1;
}

TIMER Reactor::RegisterTimer(EventHandler * p, MSEC delay, TIMER id)
{
    assert(p);
    if (!p) return TimerQ::INVALID_TIMER;
    EventHandlerMapIter iter = handlerMap_.find(p->Handle());
    if (iter == handlerMap_.end()) return TimerQ::INVALID_TIMER;
    RegHandler & rh = iter->second;
    if (rh.handler != p)
    {// TODO log
        return TimerQ::INVALID_TIMER;
    }
    if (loop_)
    {
        TIMER ret = id;
        if (id > 0)
            ret = timerQ_.RegisterTimer(id, &rh, delay);
        else
            ret = timerQ_.RegisterTimer(&rh, delay);
        rh.events |= EventHandler::TIMER_MASK;
        return ret;
    }
    return TimerQ::INVALID_TIMER;
}

int Reactor::RemoveTimer(TIMER timerId)
{
    if (loop_)
    {
        timerQ_.RemoveTimer(timerId);
        return 0;
    }
    return -1;
}




