#ifndef _LIST_H_
#define _LIST_H_
#include "typedef.h"

typedef struct _ListNode ListNode;

typedef struct _List
{
	void *First;
	void *Last;
	U16 Count;
}List;


#define List_InitStruct(x) \
	x.First = NULL; x.Last = NULL; x.Count = NULL

#define List_Add(x, node, datatype) \
{\
	if (x.First == NULL) \
	{ \
		x.First = node; \
		x.Last = node; \
		node->Next = NULL; \
		node->Prev = NULL; \
		x.Count++; \
	} \
	else if (node->Next == NULL && node->Prev == NULL && node != x.First && node != x.Last) \
	{ \
		((datatype *)x.Last)->Next = node; \
		node->Prev = (datatype *)x.Last; \
		x.Last = node; \
		x.Count++; \
	} \
}

#define List_Remove(x, node, datatype) \
{\
		if (node->Prev != NULL) \
		{ \
			node->Prev->Next = node->Next; \
		} \
		if (node->Next != NULL) \
		{ \
			node->Next->Prev = node->Prev; \
		} \
		if (x.First == node) \
		{ \
			x.First = node->Next; \
		} \
		if (x.Last == node) \
		{ \
			x.Last = node->Prev; \
		} \
		node->Next = NULL; \
		node->Prev = NULL; \
		x.Count--;  \
}

#endif

