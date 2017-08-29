/*****************************************************************************
 *          Copyright c E-LEAD Electronics All Rights Reserved
 *****************************************************************************			
 * Description:
 *
 *****************************************************************************/
#include <string.h>
#include "List.h"
#include "TimerManager.h"
#include "NUC505_Timer.h"


#define TM_BASE_PROID	1000L
#define TM_TIMER_PORT	1

typedef struct _TimerData TimerData;
typedef struct _OnTimerEventData OnTimerEventData;

struct _OnTimerEventData
{
	TimerID ID;
	OnTimerEventData *Next;
	OnTimerEventData *Prev;
	TM_OnTimerEvent OnTime;
};

struct _TimerData
{
	U8  Mode : 2;
	U8  InterruptCallback : 1;
	U8  State;
	U32 Proid;
	U32 Count;
	OnTimerEventData Event;
	TimerData *Next;
	TimerData *Prev;
};

static TimerData _timerPool[40];

static U8 _MaxUseCount;
static U8 _UseCount;

static List _runList; //Timer Running List
static List _cbList;  //Callback List

/******************************************************************************

******************************************************************************/
void TM_Init()
{
	memset(_timerPool, 0, sizeof(_timerPool));
	List_InitStruct(_runList);
	List_InitStruct(_cbList);
	_UseCount = 0;
	_MaxUseCount = 0;
}

/******************************************************************************

******************************************************************************/
void TM_TimerISR(U8 timer)
{
	TimerData *ptr;
	TimerData *ptr_next;
	U8 i = 0;

	if (_runList.First == NULL)
		return;

	ptr = (TimerData *)_runList.First;

	while( ptr != NULL )  
	{
		if (ptr->Count == 0)  //check count one by one
		{
			if (ptr->Mode == 2)
			{
				ptr->Count = ptr->Proid - 1;
				ptr->State = TIMER_RUNNING;
				ptr_next = ptr->Next;
			}
			else
			{
				ptr->State = TIMER_STOP;
				ptr_next = ptr->Next;
				List_Remove(_runList, ptr, TimerData);
			}

			if (ptr->Event.OnTime != NULL)
			{
				if (ptr->InterruptCallback != 0)
					ptr->Event.OnTime(i + 1);
				else
					List_Add(_cbList, (&ptr->Event), OnTimerEventData);
			}
		}
		else
		{
			ptr->Count--;
			ptr->State = TIMER_RUNNING;
			ptr_next = ptr->Next;
		}
		ptr = ptr_next;
	}
}

/******************************************************************************

******************************************************************************/
void TM_Start()
{
	TIM_SetTimer( TM_TIMER_PORT, TM_BASE_PROID, TM_TimerISR );
	TIM_TurnOn( TM_TIMER_PORT );
}

/******************************************************************************

******************************************************************************/
void TM_Stop()
{
	TIM_TurnOff(TM_TIMER_PORT);
}


/******************************************************************************
callbackType: 0 = Disable Interrupt Callback, 1 = Enable Interrupt Callback
mode: TIMER_ONECYCLE or TIMER_CONTINUOUS
priod: unit ms
event: Callback Handler
******************************************************************************/
static TimerID AllocTimer(U8 callbackType, U8 mode, U32 priod, TM_OnTimerEvent event)
{
	U8 i;
	
	for (i = 0; i < ArrayLength(_timerPool); ++i)
	{
		if (!_timerPool[i].Mode)
		{
			TIM_IntDisable(TM_TIMER_PORT);
			_timerPool[i].State = TIMER_STOP;
			_timerPool[i].Proid = priod;
			_timerPool[i].Count = _timerPool[i].Proid - 1;
			_timerPool[i].Event.OnTime = event;
			_timerPool[i].Event.ID = i + 1;
			if (mode == TIMER_ONECYCLE)
				_timerPool[i].Mode = 1; //one-cycle
			else
				_timerPool[i].Mode = 2; //Continuous

			if (callbackType)  
				_timerPool[i].InterruptCallback = 1; //Enable Interrupt Callback
			else
				_timerPool[i].InterruptCallback = 0; //disable Interrupt Callback

			_UseCount++;
			if (_UseCount > _MaxUseCount)
				_MaxUseCount = _UseCount;

			TIM_IntEnable( TM_TIMER_PORT );

			return i + 1;
		}
	}

	//DBG_Print("Error:Timer lack of resource!\n");
	for( ;; )
	
	return 0;
}

/******************************************************************************

******************************************************************************/
TimerID TM_CreateTimer(U8 mode, U32 priod, TM_OnTimerEvent event)
{
	return AllocTimer( 0, mode, priod, event );
}

/******************************************************************************

******************************************************************************/
TimerID TM_CreateIntTimer(U8 mode, U32 priod, TM_OnTimerEvent event)
{
	return AllocTimer(1, mode, priod, event);
}

/******************************************************************************

******************************************************************************/
void TM_FreeTimer(TimerID id)
{
	TimerData *ptr;
	if (id > 0 && id <= ArrayLength(_timerPool))
	{
		ptr = &_timerPool[id - 1];
		TIM_IntDisable(TM_TIMER_PORT);

		if (ptr->Prev != NULL || ptr->Next != NULL)
			List_Remove(_runList, ptr, TimerData);

		if (ptr->Event.Next != NULL || ptr->Event.Prev != NULL)
			List_Remove(_cbList, (&ptr->Event), OnTimeEventData);


		_timerPool[id - 1].Mode = 0;
		_timerPool[id - 1].State = TIMER_STOP;
		_UseCount--;
		TIM_IntEnable(TM_TIMER_PORT);
	}
}

/******************************************************************************

******************************************************************************/
void TM_StartTimer(TimerID id)
{
	TimerData *data;
	if (id > 0 && id <= ArrayLength(_timerPool))
	{
		data = &_timerPool[id - 1];
		TIM_IntDisable(TM_TIMER_PORT);
		data->Count = data->Proid;
		data->State = TIMER_RUNNING;
		List_Add(_runList, data, TimerData);
		TIM_IntEnable(TM_TIMER_PORT);
	}
}

/******************************************************************************

******************************************************************************/
void TM_StopTimer(TimerID id)
{
	TimerData *data;
	if (id > 0 && id <= ArrayLength(_timerPool))
	{
		data = &_timerPool[id - 1];

		TIM_IntDisable(TM_TIMER_PORT);
		data->State = TIMER_STOP;
		
		if (data->Prev != NULL || data->Next != NULL)
			List_Remove(_runList, data, TimerData);

		if (data->Event.Next != NULL || data->Event.Prev != NULL)
			List_Remove(_cbList, (&data->Event), OnTimeEventData);

		TIM_IntEnable(TM_TIMER_PORT);
	}
}

/******************************************************************************

******************************************************************************/
U8 TM_GetTimerState(TimerID id)
{
	if (id > 0 && id <= ArrayLength(_timerPool))
	{
		if (_timerPool[id - 1].Mode)
		{
			return _timerPool[id - 1].State;
		}
	}
	return TIMER_INVALID;
}

/******************************************************************************

******************************************************************************/
void TM_ConfigTimer(TimerID id, U32 priod, TM_OnTimerEvent event)
{
	TimerData *data;

	if (id > 0 && id <= ArrayLength(_timerPool))
	{
		data = &_timerPool[id - 1];
		TIM_IntDisable(TM_TIMER_PORT);
		_timerPool[id - 1].Proid = priod;
		_timerPool[id - 1].Event.OnTime = event;
		_timerPool[id - 1].State = TIMER_STOP;
		if (data->Prev != NULL || data->Next != NULL)
			List_Remove(_runList, data, TimerData);

		if (data->Event.Next != NULL || data->Event.Prev != NULL)
			List_Remove(_cbList, (&data->Event), OnTimeEventData);
		TIM_IntEnable(TM_TIMER_PORT);
	}
}


void TM_PrintResource()
{
	#if 0
	U8 listCount = 0;
	U8 useCount = 0;
	U8 maxUseCount = 0;
	U8 cbCount = 0;

	listCount = _runList.Count;
	useCount = _UseCount;
	maxUseCount = _MaxUseCount;
	cbCount = _cbList.Count;

	DBG_Print("TM R=");
	DBG_PrintHexByte(listCount);
	DBG_Print(" CB=");
	DBG_PrintHexByte(cbCount);
	DBG_Print(" Use=");
	DBG_PrintHexByte(useCount);
	DBG_Print(" Max=");
	DBG_PrintHexByte(maxUseCount);
	DBG_Print("\n");
	#endif
}

void TM_Process()
{
	OnTimerEventData *ptr;

	if( _cbList.First == NULL )
		return;

	TIM_IntDisable( TM_TIMER_PORT );
	
	ptr = (OnTimerEventData *)_cbList.First;
	
	List_Remove(_cbList, ptr, OnTimeEventData);
	
	TIM_IntEnable(TM_TIMER_PORT);
	
	ptr->OnTime( ptr->ID );
}




