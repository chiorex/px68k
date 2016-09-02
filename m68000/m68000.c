/******************************************************************************

	m68000.c

	M68000 CPUインタフェース関数

******************************************************************************/

#include "m68000.h"

#ifndef CYCLONE
#include "c68k.h"
#else
struct Cyclone m68k;
#endif


long long cycleEpoch;
long long lastTraceMicro = 0, lastTraceCycleEpoch = 0;
int tracing = 0;
clock_t microEpoch, microCurrent;

#include <stdlib.h>
#include <signal.h>

#include "../x68k/memory.h"



/******************************************************************************
	M68000インタフェース関数
******************************************************************************/

/*--------------------------------------------------------
	CPU初期化
--------------------------------------------------------*/
#ifdef CYCLONE

unsigned int read8(unsigned int a) {
	return (unsigned int) cpu_readmem24(a);
}

unsigned int read16(unsigned int a) {
	return (unsigned int) cpu_readmem24_word(a);
}

unsigned int MyCheckPc(unsigned int pc)
{


  pc-= m68k.membase; // Get the real program counter

  if (!tracing) {
  	//p6logd("CheckPC 0x%08X\n", pc);
  }

  if (pc <= 0xbfffff) 			       					{ m68k.membase=(int) MEM; return m68k.membase+pc; }
  if ((pc >= 0xfc0000) && (pc <= 0xffffff))	{ m68k.membase=(int) IPL - 0xfc0000; return m68k.membase+pc; }
  if ((pc >= 0xc00000) && (pc <= 0xc7ffff)) m68k.membase=(int) GVRAM - 0xc00000;
  if ((pc >= 0xe00000) && (pc <= 0xe7ffff))	m68k.membase=(int) TVRAM - 0xe00000;
  if ((pc >= 0xea0000) && (pc <= 0xea1fff))	m68k.membase=(int) SCSIIPL - 0xea0000;
  if ((pc >= 0xed0000) && (pc <= 0xed3fff))	m68k.membase=(int) SRAM - 0xed0000;
  if ((pc >= 0xf00000) && (pc <= 0xfbffff))	m68k.membase=(int) FONT - 0xf00000;

  return m68k.membase+pc; // New program counter
}

#endif

void m68000_init(void)
{
	#ifdef CYCLONE

	m68k.read8  = read8;
	m68k.read16 = read16;
	m68k.read32 = cpu_readmem24_dword;

	m68k.fetch8  = read8;
	m68k.fetch16 = read16;
	m68k.fetch32 = cpu_readmem24_dword;

	m68k.write8  = cpu_writemem24;
	m68k.write16 = cpu_writemem24_word;
	m68k.write32 = cpu_writemem24_dword;

	m68k.checkpc = MyCheckPc;

	CycloneInit();

	#else

	C68k_Init(&C68K);
	C68k_Set_ReadB(&C68K, Memory_ReadB);
	C68k_Set_ReadW(&C68K, Memory_ReadW);
	C68k_Set_WriteB(&C68K, Memory_WriteB);
	C68k_Set_WriteW(&C68K, Memory_WriteW);
  C68k_Set_Fetch(&C68K, 0x000000, 0xbfffff, (UINT32)MEM);
  C68k_Set_Fetch(&C68K, 0xc00000, 0xc7ffff, (UINT32)GVRAM);
  C68k_Set_Fetch(&C68K, 0xe00000, 0xe7ffff, (UINT32)TVRAM);
  C68k_Set_Fetch(&C68K, 0xea0000, 0xea1fff, (UINT32)SCSIIPL);
  C68k_Set_Fetch(&C68K, 0xed0000, 0xed3fff, (UINT32)SRAM);
  C68k_Set_Fetch(&C68K, 0xf00000, 0xfbffff, (UINT32)FONT);
  C68k_Set_Fetch(&C68K, 0xfc0000, 0xffffff, (UINT32)IPL);

    #endif
}


/*--------------------------------------------------------
	CPUリセット
--------------------------------------------------------*/

void m68000_reset(void)
{
	#ifdef CYCLONE

	CycloneReset(&m68k);
	m68k.state_flags = 0; // Go to default state (not stopped, halted, etc.)
	m68k.srh = 0x27; // Set supervisor mode
	cycleEpoch = 0;
	microEpoch = clock();
 
	#else

	C68k_Reset(&C68K);
	#endif
}


/*--------------------------------------------------------
	CPU停止
--------------------------------------------------------*/

void m68000_exit(void)
{
}


/*--------------------------------------------------------
	CPU実行
--------------------------------------------------------*/


int m68000_execute(int cycles)
{

	int cyclesLeft;
	float emuSampleSeconds, realSampleSeconds, emuTotalSeconds, realTotalSeconds;
	#ifdef CYCLONE

	m68k.cycles = cycles;
	CycloneRun(&m68k);
	cyclesLeft = m68k.cycles;
	
	#else

	cyclesLeft = cycles - C68k_Exec(&C68K, cycles);

	#endif

	cycleEpoch += (cycles - cyclesLeft);


	if (cycleEpoch > (lastTraceCycleEpoch + 10000000) ) {

		tracing = 1;

		microCurrent = clock();

		
		emuTotalSeconds = ((float) cycleEpoch) / 10000000.0;
		emuSampleSeconds = ((float) cycleEpoch - lastTraceCycleEpoch) / 10000000.0;

		realTotalSeconds = (microCurrent - microEpoch) / 1000000.0;
		realSampleSeconds = (microCurrent - lastTraceMicro) / 1000000.0;

		p6logd("Cycles: %lld Et: %.1fs Rt: %.1fs Es: %.2fs Rs: %.2fs Speed: %.1f \n", 
			cycleEpoch, 
			emuTotalSeconds, 
			realTotalSeconds,
			emuSampleSeconds,
			realSampleSeconds,
			emuSampleSeconds/  realSampleSeconds );
				
	
		p6logd("D0:%08X D1:%08X D2:%08X D3:%08X D4:%08X D5:%08X D6:%08X D7:%08X CR:%04X\n",
			m68000_get_reg(M68K_D0), m68000_get_reg(M68K_D1), m68000_get_reg(M68K_D2), m68000_get_reg(M68K_D3),
			m68000_get_reg(M68K_D4), m68000_get_reg(M68K_D5), m68000_get_reg(M68K_D6), m68000_get_reg(M68K_D7), 0);
		p6logd("A0:%08X A1:%08X A2:%08X A3:%08X A4:%08X A5:%08X A6:%08X A7:%08X SR:%04X\n",
			m68000_get_reg(M68K_A0), m68000_get_reg(M68K_A1), m68000_get_reg(M68K_A2), m68000_get_reg(M68K_A3),
			m68000_get_reg(M68K_A4), m68000_get_reg(M68K_A5), m68000_get_reg(M68K_A6), m68000_get_reg(M68K_A7), 0);
		p6logd("PC:%08X (%04X) \n", m68000_get_reg(M68K_PC), Memory_ReadW(m68000_get_reg(M68K_PC)));

		tracing = 0;

		lastTraceCycleEpoch = cycleEpoch;
		lastTraceMicro = microCurrent;
	}

	if (cycleEpoch > 20000000) {
//		exit(1);
	}

	return cyclesLeft;
}

int * m68000_get_cycles()
{
	#ifdef CYCLONE
	return & m68k.cycles;
	
	#else

	return & C68K.ICount;

	#endif
}

/*--------------------------------------------------------
	割り込み処理
--------------------------------------------------------*/

void m68000_reset_irq_line(int irqline)
{
	#ifdef CYCLONE

	m68k.irq = 0;
	
	#else

	if (irqline == IRQ_LINE_NMI)
		irqline = 7;

	C68k_Set_IRQ(&C68K, irqline, 0);

	#endif
}


void m68000_set_irq_line(int irqline)
{
	#ifdef CYCLONE

	m68k.irq = irqline;

	#else

	if (irqline == IRQ_LINE_NMI)
		irqline = 7;

	C68k_Set_IRQ(&C68K, irqline, HOLD_LINE);

	#endif
}


/*--------------------------------------------------------
	割り込みコールバック関数設定
--------------------------------------------------------*/

void m68000_set_irq_callback(int (*callback)(int line))
{
	#ifdef CYCLONE
	
	m68k.IrqCallback = callback;

	#else

	C68k_Set_IRQ_Callback(&C68K, callback);

	#endif
}


/*--------------------------------------------------------
	レジスタ取得
--------------------------------------------------------*/

UINT32 m68000_get_reg(int regnum)
{
	#ifdef CYCLONE

	switch (regnum)
	{
		case M68K_PC: return m68k.pc - m68k.membase;
		case M68K_SR: return CycloneGetSr(&m68k);
		case M68K_D0: return m68k.d[0];
		case M68K_D1: return m68k.d[1];
		case M68K_D2: return m68k.d[2];
		case M68K_D3: return m68k.d[3];
		case M68K_D4: return m68k.d[4];
		case M68K_D5: return m68k.d[5];
		case M68K_D6: return m68k.d[6];
		case M68K_D7: return m68k.d[7];
		case M68K_A0: return m68k.a[0];
		case M68K_A1: return m68k.a[1];
		case M68K_A2: return m68k.a[2];
		case M68K_A3: return m68k.a[3];
		case M68K_A4: return m68k.a[4];
		case M68K_A5: return m68k.a[5];
		case M68K_A6: return m68k.a[6];
		case M68K_A7: return m68k.a[7];
		default:
			break;
	}
	return 0x0BADC0DE;
	
	#else

	switch (regnum)
	{
	case M68K_PC:  return C68k_Get_Reg(&C68K, C68K_PC);
	case M68K_USP: return C68k_Get_Reg(&C68K, C68K_USP);
	case M68K_MSP: return C68k_Get_Reg(&C68K, C68K_MSP);
	case M68K_SR:  return C68k_Get_Reg(&C68K, C68K_SR);
	case M68K_D0:  return C68k_Get_Reg(&C68K, C68K_D0);
	case M68K_D1:  return C68k_Get_Reg(&C68K, C68K_D1);
	case M68K_D2:  return C68k_Get_Reg(&C68K, C68K_D2);
	case M68K_D3:  return C68k_Get_Reg(&C68K, C68K_D3);
	case M68K_D4:  return C68k_Get_Reg(&C68K, C68K_D4);
	case M68K_D5:  return C68k_Get_Reg(&C68K, C68K_D5);
	case M68K_D6:  return C68k_Get_Reg(&C68K, C68K_D6);
	case M68K_D7:  return C68k_Get_Reg(&C68K, C68K_D7);
	case M68K_A0:  return C68k_Get_Reg(&C68K, C68K_A0);
	case M68K_A1:  return C68k_Get_Reg(&C68K, C68K_A1);
	case M68K_A2:  return C68k_Get_Reg(&C68K, C68K_A2);
	case M68K_A3:  return C68k_Get_Reg(&C68K, C68K_A3);
	case M68K_A4:  return C68k_Get_Reg(&C68K, C68K_A4);
	case M68K_A5:  return C68k_Get_Reg(&C68K, C68K_A5);
	case M68K_A6:  return C68k_Get_Reg(&C68K, C68K_A6);
	case M68K_A7:  return C68k_Get_Reg(&C68K, C68K_A7);
	default: return 0;
	}

	#endif
}


/*--------------------------------------------------------
	レジスタ設定
--------------------------------------------------------*/

void m68000_set_reg(int regnum, UINT32 val)
{
	#ifdef CYCLONE
	switch (regnum)
	{
		case M68K_PC:
		  	m68k.pc = m68k.checkpc(val+m68k.membase);
			break;
		case M68K_SR:
		  	CycloneSetSr(&m68k, val);
			break;
		case M68K_D0: m68k.d[0] = val; break;
		case M68K_D1: m68k.d[1] = val; break;
		case M68K_D2: m68k.d[2] = val; break;
		case M68K_D3: m68k.d[3] = val; break;
		case M68K_D4: m68k.d[4] = val; break;
		case M68K_D5: m68k.d[5] = val; break;
		case M68K_D6: m68k.d[6] = val; break;
		case M68K_D7: m68k.d[7] = val; break;
		case M68K_A0: m68k.a[0] = val; break;
		case M68K_A1: m68k.a[1] = val; break;
		case M68K_A2: m68k.a[2] = val; break;
		case M68K_A3: m68k.a[3] = val; break;
		case M68K_A4: m68k.a[4] = val; break;
		case M68K_A5: m68k.a[5] = val; break;
		case M68K_A6: m68k.a[6] = val; break;
		case M68K_A7: m68k.a[7] = val; break;

		
		default: break;
	}
	
	#else

	switch (regnum)
	{
	case M68K_PC:  C68k_Set_Reg(&C68K, C68K_PC, val); break;
	case M68K_USP: C68k_Set_Reg(&C68K, C68K_USP, val); break;
	case M68K_MSP: C68k_Set_Reg(&C68K, C68K_MSP, val); break;
	case M68K_SR:  C68k_Set_Reg(&C68K, C68K_SR, val); break;
	case M68K_D0:  C68k_Set_Reg(&C68K, C68K_D0, val); break;
	case M68K_D1:  C68k_Set_Reg(&C68K, C68K_D1, val); break;
	case M68K_D2:  C68k_Set_Reg(&C68K, C68K_D2, val); break;
	case M68K_D3:  C68k_Set_Reg(&C68K, C68K_D3, val); break;
	case M68K_D4:  C68k_Set_Reg(&C68K, C68K_D4, val); break;
	case M68K_D5:  C68k_Set_Reg(&C68K, C68K_D5, val); break;
	case M68K_D6:  C68k_Set_Reg(&C68K, C68K_D6, val); break;
	case M68K_D7:  C68k_Set_Reg(&C68K, C68K_D7, val); break;
	case M68K_A0:  C68k_Set_Reg(&C68K, C68K_A0, val); break;
	case M68K_A1:  C68k_Set_Reg(&C68K, C68K_A1, val); break;
	case M68K_A2:  C68k_Set_Reg(&C68K, C68K_A2, val); break;
	case M68K_A3:  C68k_Set_Reg(&C68K, C68K_A3, val); break;
	case M68K_A4:  C68k_Set_Reg(&C68K, C68K_A4, val); break;
	case M68K_A5:  C68k_Set_Reg(&C68K, C68K_A5, val); break;
	case M68K_A6:  C68k_Set_Reg(&C68K, C68K_A6, val); break;
	case M68K_A7:  C68k_Set_Reg(&C68K, C68K_A7, val); break;
	default: break;
	}

	#endif
}


/*------------------------------------------------------
	セーブ/ロード ステート
------------------------------------------------------*/

#ifdef SAVE_STATE

STATE_SAVE( m68000 )
{
	#ifdef CYCLONE
	
	#else
	int i;
	UINT32 pc = C68k_Get_Reg(&C68K, C68K_PC);

	for (i = 0; i < 8; i++)
		state_save_long(&C68K.D[i], 1);
	for (i = 0; i < 8; i++)
		state_save_long(&C68K.A[i], 1);

	state_save_long(&C68K.flag_C, 1);
	state_save_long(&C68K.flag_V, 1);
	state_save_long(&C68K.flag_Z, 1);
	state_save_long(&C68K.flag_N, 1);
	state_save_long(&C68K.flag_X, 1);
	state_save_long(&C68K.flag_I, 1);
	state_save_long(&C68K.flag_S, 1);
	state_save_long(&C68K.USP, 1);
	state_save_long(&pc, 1);
	state_save_long(&C68K.HaltState, 1);
	state_save_long(&C68K.IRQLine, 1);
	state_save_long(&C68K.IRQState, 1);

	#endif
}

STATE_LOAD( m68000 )
{
	#ifdef CYCLONE
	
	#else

	int i;
	UINT32 pc;

	for (i = 0; i < 8; i++)
		state_load_long(&C68K.D[i], 1);
	for (i = 0; i < 8; i++)
		state_load_long(&C68K.A[i], 1);

	state_load_long(&C68K.flag_C, 1);
	state_load_long(&C68K.flag_V, 1);
	state_load_long(&C68K.flag_Z, 1);
	state_load_long(&C68K.flag_N, 1);
	state_load_long(&C68K.flag_X, 1);
	state_load_long(&C68K.flag_I, 1);
	state_load_long(&C68K.flag_S, 1);
	state_load_long(&C68K.USP, 1);
	state_load_long(&pc, 1);
	state_load_long(&C68K.HaltState, 1);
	state_load_long(&C68K.IRQLine, 1);
	state_load_long(&C68K.IRQState, 1);

	C68k_Set_Reg(&C68K, C68K_PC, pc);

	#endif
}

#endif /* SAVE_STATE */
