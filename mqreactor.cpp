/**
 * @file mqreactor.cpp
 * @brief	
 * @author GongZhaohui
 * @version 
 * @date 2016-02-23
 */

#include "mqreactor.h"

using namespace amos;

void MQReactor::ProcessMqMsg()
{
	ReactorMq tmq;
	if (!mq_.empty())
	{
		ScopeLock lock(mqlock_);
		tmq.swap(mq_);
	}
	else
	{
		int i = 0;
		while (i < tmq.size())
		{
			ReactorMsg &msg = tmq[i];
			switch (msg.mtype)
			{
			case RMSG_REGHANDLER: 
				{
					EventHandler * handler = msg.handler;
					EvMask mask = (EvMask)msg.arg0.val;
					EventHandlerCreator * creator =
						(EventHandlerCreator*)msg.arg1.ptr;
					if (Reactor::RegisterHandler(handler, mask, creator))
					{// TODO log print
					}
				}
				break;
			case RMSG_RMHANDLER: 
				{
					EventHandler * handler = msg.handler;
					EvMask mask = (EvMask)msg.arg0.val;
					if (Reactor::RemoveHandler(handler, mask))
					{// TODO log print
					}
				}
				break;
			case RMSG_REGTIMER: 
				{
					EventHandler * handler = msg.handler;
					TIMER id = (TIMER)msg.arg0.val;
					MSEC delay = (MSEC)msg.arg1.val;
					if (Reactor::RegisterTimer(handler, delay, id))
					{// TODO log print
					}
				}
				break;
			case RMSG_RMTIMER: 
				{
					TIMER id = (TIMER)msg.arg0.val;
					if (Reactor::RemoveTimer(id))
					{// TODO log print
					}
				}
				break;
			default:
				break;
			}// end switch
			++i;
		}// end while
	}// end if else
}

void MQReactor::RunEventLoop()
{
	while (loop_)
	{
		EventHandlerVec ehList;
		//1. process all msgs in mq
		ProcessMqMsg();
		//2.poll timer events
		MSEC nextTimeout = timerQ_.Schedule(ehList);
		if (nextTimeout > DEFAULT_REACT_INTERVAL) 
			nextTimeout = DEFAULT_REACT_INTERVAL;
		//3.poll I/O events
		impl_->Demultiplex(handlerMap_, ehList, nextTimeout); 
		//4.handle all events
		if (ehList.size() > 0)
			HandleEvents(ehList);
		ehList.clear();
	}
}



