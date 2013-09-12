#include "STF/Interface/STFTimer.h"

STFTimer * SystemTimer;

STFTimer::STFTimer(void)
	: STFThread("Event scheduler", 10000, STFTP_HIGHEST)
	{			
	count = 0;

	for (int i = 0; i < 16; i++)
		pendingEvents[i].sink = NULL;

	StartThread();
	}

STFTimer::~STFTimer()
	{
	}


void STFTimer::ThreadEntry(void)
	{
	STFHiPrec64BitTime   	currentTime, nextTime;
	STFLoPrec32BitDuration	duration;

	STFMessageSink	*	      sink;
	uint32				      id;
	int					      nextEvent;
	int					      i;
	bool					      timeout;

	while (!terminate)
		{
		nextEvent = -1;

		mutex.Enter();

		for(i=0; i<16; i++)
			{
			if (pendingEvents[i].sink && (nextEvent == -1 || pendingEvents[i].time < nextTime))
				{
				nextTime = pendingEvents[i].time;
				nextEvent = i;
				}
			}

		if (nextEvent >= 0)
			{
			GetTime(currentTime);
			if (nextTime < currentTime)
				{
				sink = pendingEvents[nextEvent].sink;						
				id = pendingEvents[nextEvent].id;
				if (pendingEvents[nextEvent].recurrent)
					{
					pendingEvents[nextEvent].time += pendingEvents[nextEvent].dueCycleDuration;
					}
				else
					{
					pendingEvents[nextEvent].sink = NULL;
					}
				mutex.Leave();
				sink->SendMessage(pendingEvents[nextEvent].message, false);
				}
			else
				{
				duration = nextTime - currentTime;

				mutex.Leave();
				timeout = signal.WaitTimeout(duration) == STFRES_TIMEOUT;
				}

			}
		else
			{
			mutex.Leave();
			signal.Wait();
			}
		}
	}

STFResult STFTimer::NotifyThreadTermination(void)
	{
	signal.SetSignal();

	STFRES_RAISE_OK;
	}		

STFResult STFTimer::ScheduleEvent(const STFHiPrec64BitTime & time, STFMessageSink * sink, const STFMessage & message, uint32 & timer)
	{
	int i;
	STFResult err = STFRES_OK;

	mutex.Enter();

	i = 0;
	while (i < 16 && pendingEvents[i].sink)
		{
		i++;
		}

	if (i < 16)
		{
		pendingEvents[i].sink = sink;
		pendingEvents[i].message = message;
		pendingEvents[i].time = time;
		pendingEvents[i].id = timer = i + count;
		pendingEvents[i].recurrent = false;
		count += 16;

		signal.SetSignal();
		}
	else
		err = STFRES_OBJECT_FULL;

	mutex.Leave();

	STFRES_RAISE(err);
	}

STFResult STFTimer::ScheduleRecurrentEvent(const STFHiPrec64BitTime & time, const STFHiPrec64BitDuration & dueClycleDuration, STFMessageSink * sink, const STFMessage & message, uint32 & timer)
	{
	int i;
	STFResult err = STFRES_OK;

	mutex.Enter();

	i = 0;
	while (i < 16 && pendingEvents[i].sink)
		{
		i++;
		}

	if (i < 16)
		{
		pendingEvents[i].sink = sink;
		pendingEvents[i].message = message;
		pendingEvents[i].time = time;
		pendingEvents[i].id = timer = i + count;
		pendingEvents[i].recurrent = true;
		pendingEvents[i].dueCycleDuration = dueClycleDuration;
		count += 16;

		signal.SetSignal();
		}
	else
		err = STFRES_OBJECT_FULL;

	mutex.Leave();

	STFRES_RAISE(err);
	}


STFResult STFTimer::CancelEvent(uint32 timer)
	{
	mutex.Enter();

	if (pendingEvents[timer & 15].id == timer)
		{
		pendingEvents[timer & 15].sink = NULL;
		}

	mutex.Leave();

	STFRES_RAISE_OK;
	}						

STFResult InitializeSTFTimer(void)
	{
	STFRES_REASSERT(InitializeOSSTFTimer());

	SystemTimer = new STFTimer();
	if (SystemTimer == NULL)
		STFRES_RAISE(STFRES_NOT_ENOUGH_MEMORY);

	STFRES_RAISE_OK;
	}

