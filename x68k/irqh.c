// ---------------------------------------------------------------------------------------
//  IRQH.C - IRQ Handler (架空のデバイスにょ)
// ---------------------------------------------------------------------------------------

#include "common.h"
#include "../m68000/m68000.h"
#include "irqh.h"

extern int m68000_ICountBk;
extern int ICount;
extern int * C68KICount;

BYTE	IRQH_IRQ[8];
void	*IRQH_CallBack[8];

// -----------------------------------------------------------------------
//   初期化
// -----------------------------------------------------------------------
void IRQH_Init(void)
{
	ZeroMemory(IRQH_IRQ, 8);
}


// -----------------------------------------------------------------------
//   デフォルトのベクタを返す（これが起こったら変だお）
// -----------------------------------------------------------------------
DWORD FASTCALL IRQH_DefaultVector(BYTE irq)
{
	IRQH_IRQCallBack(irq);
	return -1;
}


// -----------------------------------------------------------------------
//   他の割り込みのチェック
//   各デバイスのベクタを返すルーチンから呼ばれます
// -----------------------------------------------------------------------
void IRQH_IRQCallBack(BYTE irq)
{
	//p6logd("In IRQCallback: %d\n", irq);
	int i;
	IRQH_IRQ[irq] = 0;
	m68000_reset_irq_line(irq);
	for (i=7; i>0; i--)
	{
		if (IRQH_IRQ[i])
		{
			m68000_set_irq_callback(IRQH_CallBack[i]);
			m68000_set_irq_line(i); // xxx 
			if ( *C68KICount) {					// 多重割り込み時（CARAT）
				m68000_ICountBk += *C68KICount;		// 強制的に割り込みチェックをさせる
				*C68KICount = 0;				// 苦肉の策 ^^;
			}
			break;
		}
	}
}


// -----------------------------------------------------------------------
//   割り込み発生
// -----------------------------------------------------------------------
void IRQH_Int(BYTE irq, void* handler)
{
	int i;
	IRQH_IRQ[irq] = 1;
	if (handler==NULL)
		IRQH_CallBack[irq] = &IRQH_DefaultVector;
	else
		IRQH_CallBack[irq] = handler;
	for (i=7; i>0; i--)
	{
		if (IRQH_IRQ[i])
		{
            m68000_set_irq_callback(IRQH_CallBack[i]);
            m68000_set_irq_line(i); //xxx
			if ( *C68KICount ) {					// 多重割り込み時（CARAT）
				m68000_ICountBk += *C68KICount;		// 強制的に割り込みチェックをさせる
				*C68KICount = 0;				// 苦肉の策 ^^;
			}
			return;
		}
	}
}
