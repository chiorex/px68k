// ---------------------------------------------------------------------------------------
//  IRQH.C - IRQ Handler (�Ͷ��ΥǥХ����ˤ�)
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
//   �����
// -----------------------------------------------------------------------
void IRQH_Init(void)
{
	ZeroMemory(IRQH_IRQ, 8);
}


// -----------------------------------------------------------------------
//   �ǥե���ȤΥ٥������֤��ʤ��줬�����ä����Ѥ�����
// -----------------------------------------------------------------------
DWORD FASTCALL IRQH_DefaultVector(BYTE irq)
{
	IRQH_IRQCallBack(irq);
	return -1;
}


// -----------------------------------------------------------------------
//   ¾�γ����ߤΥ����å�
//   �ƥǥХ����Υ٥������֤��롼���󤫤�ƤФ�ޤ�
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
			if ( *C68KICount) {					// ¿�ų����߻���CARAT��
				m68000_ICountBk += *C68KICount;		// ����Ū�˳����ߥ����å��򤵤���
				*C68KICount = 0;				// �����κ� ^^;
			}
			break;
		}
	}
}


// -----------------------------------------------------------------------
//   ������ȯ��
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
			if ( *C68KICount ) {					// ¿�ų����߻���CARAT��
				m68000_ICountBk += *C68KICount;		// ����Ū�˳����ߥ����å��򤵤���
				*C68KICount = 0;				// �����κ� ^^;
			}
			return;
		}
	}
}
