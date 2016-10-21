// ---------------------------------------------------------------------------------------
//  GVRAM.C - Graphic VRAM
// ---------------------------------------------------------------------------------------

#include	"common.h"
#include	"windraw.h"
#include	"winx68k.h"
#include	"crtc.h"
#include	"palette.h"
#include	"tvram.h"
#include	"gvram.h"
#include	"m68000.h"
#include	"memory.h"

#include <syscall.h>

BYTE	GVRAM[0x80000];

__THREAD WORD	Grp_LineBuf[1024];
__THREAD WORD	__attribute__ ((aligned (16))) Grp_LineBufSP[1024];		// ∆√ºÅE◊•È•§•™•ÅE∆•£°ø»æ∆©Ã¿Õ—•–•√•’•°
__THREAD WORD	Grp_LineBufSP2[1024];		// »æ∆©Ã¿•Ÿ°º•π•◊•ÅEº•ÛÕ—•–•√•’•°° »Û»æ∆©Ã¿•”•√•»≥ «º°À
__THREAD WORD	Grp_LineBufSP_Tr[1024];

WORD	Pal16Adr[256];			// 16bit color •—•ÅE√•»•¢•…•ÅEπ∑◊ªªÕ—


void debugThreadInfo(const char * where) {

#ifdef MULTI_THREADx 
	p6logd("In thread LWP: %d from %s\n", syscall(SYS_gettid), where);
	p6logd("          LWP: %d Grp_LineBuf    at %08X \n", syscall(SYS_gettid), Grp_LineBuf    );
	p6logd("          LWP: %d Grp_LineBufSP  at %08X \n", syscall(SYS_gettid), Grp_LineBufSP  );
	p6logd("          LWP: %d Grp_LineBufSP2 at %08X \n", syscall(SYS_gettid), Grp_LineBufSP2 );
#endif
}

// xxx: for little endian only
//	#define GET_WORD_W8(src) (*(BYTE *)(src) | *((BYTE *)(src) + 1) << 8)
#define GET_WORD_W8(src) (*src)


// -----------------------------------------------------------------------
//   ΩÈ¥ÅEΩ°¡
// -----------------------------------------------------------------------
void GVRAM_Init(void)
{
	int i;

	ZeroMemory(GVRAM, 0x80000);
	for (i=0; i<128; i++)			// 16bit color •—•ÅE√•»•¢•…•ÅEπ∑◊ªªÕ—
	{
		Pal16Adr[i*2] = i*4;
		Pal16Adr[i*2+1] = i*4+1;
	}
}


// -----------------------------------------------------------------------------------
//  π‚¬Æ•Ø•ÅE¢Õ—•ÅEº•¡•ÅE
// -----------------------------------------------------------------------------------

void FASTCALL GVRAM_FastClear(void)
{
	DWORD v, h;
	v = ((CRTC_Regs[0x29]&4)?512:256);
	h = ((CRTC_Regs[0x29]&3)?512:256);
{
	WORD *p;
	DWORD x, y;
	DWORD offx, offy;

	offy = (GrphScrollY[0] & 0x1ff) << 10;
	for (y = 0; y < v; ++y) {
		offx = GrphScrollX[0] & 0x1ff;
		p = (WORD *)(GVRAM + offy + offx * 2);

		for (x = 0; x < h; ++x) {
			*p++ &= CRTC_FastClrMask;
			offx = (offx + 1) & 0x1ff;
		}

		offy = (offy + 0x400) & 0x7fc00;
	}
}
}


// -----------------------------------------------------------------------
//   VRAM Read
// -----------------------------------------------------------------------
BYTE FASTCALL GVRAM_Read(DWORD adr)
{
	BYTE ret=0;
	BYTE page;
	WORD *ram = (WORD*)(&GVRAM[adr&0x7fffe]);
	adr ^= 1;
	adr -= 0xc00000;

	if (CRTC_Regs[0x28]&8) {			// ∆…§ﬂπ˛§ﬂ¬¶§ÅE5536•‚°º•…§ŒVRAM«€√÷° ∂ÅEﬂ∆¨ · ™ƒ¢°À
		if (adr<0x80000) ret = GVRAM[adr];
	} else {
		switch(CRTC_Regs[0x28]&3)
		{
		case 0:					// 16 colors
			if (!(adr&1))
			{
				if (CRTC_Regs[0x28]&4)		// 1024dot
				{
					ram = (WORD*)(&GVRAM[((adr&0xff800)>>1)+(adr&0x3fe)]);
					page = (BYTE)((adr>>17)&0x08);
					page += (BYTE)((adr>>8)&4);
					ret = (((*ram)>>page)&15);
				}
				else
				{
					page = (BYTE)((adr>>17)&0x0c);
					ret = (((*ram)>>page)&15);
				}
			}
			break;
		case 1:					// 256
		case 2:					// Unknown
			if ( adr<0x100000 )
			{
				if (!(adr&1))
				{
					page = (BYTE)((adr>>16)&0x08);
					ret = (BYTE)((*ram)>>page);
				}
			}
//			else
//				BusErrFlag = 1;
			break;
		case 3:					// 65536
			if (adr<0x80000)
				ret = GVRAM[adr];
//			else
//				BusErrFlag = 1;
			break;
		}
	}
	return ret;
}


// -----------------------------------------------------------------------
//   VRAM Write
// -----------------------------------------------------------------------
void FASTCALL GVRAM_Write(DWORD adr, BYTE data)
{
	BYTE page;
	int line = 1023, scr = 0;
	WORD *ram = (WORD*)(&GVRAM[adr&0x7fffe]);
	WORD temp;

	adr ^= 1;
	adr -= 0xc00000;


	if (CRTC_Regs[0x28]&8)				// 65536•‚°º•…§ŒVRAM«€√÷°©° Nemesis°À
	{
		if ( adr<0x80000 )
		{
			GVRAM[adr] = data;
			line = (((adr&0x7ffff)/1024)-GrphScrollY[0])&511;
		}
	}
	else
	{
		switch(CRTC_Regs[0x28]&3)
		{
		case 0:					// 16 colors
			if (adr&1) break;
			if (CRTC_Regs[0x28]&4)		// 1024dot
			{
				ram = (WORD*)(&GVRAM[((adr&0xff800)>>1)+(adr&0x3fe)]);
				page = (BYTE)((adr>>17)&0x08);
				page += (BYTE)((adr>>8)&4);
				temp = ((WORD)data&15)<<page;
				*ram = ((*ram)&(~(0xf<<page)))|temp;
				line = ((adr/2048)-GrphScrollY[0])&1023;
			}
			else
			{
				page = (BYTE)((adr>>17)&0x0c);
				temp = ((WORD)data&15)<<page;
				*ram = ((*ram)&(~(0xf<<page)))|temp;
				switch(adr/0x80000)
				{
					case 0:	scr = GrphScrollY[0]; break;
					case 1: scr = GrphScrollY[1]; break;
					case 2: scr = GrphScrollY[2]; break;
					case 3: scr = GrphScrollY[3]; break;
				}
				line = (((adr&0x7ffff)/1024)-scr)&511;
			}
			break;
		case 1:					// 256 colors
		case 2:					// Unknown
			if ( adr<0x100000 )
			{
				if ( !(adr&1) )
				{
					scr = GrphScrollY[(adr>>18)&2];
					line = (((adr&0x7ffff)>>10)-scr)&511;
					TextDirtyLine[line] = 1;			// 32øß4ÃÃ§ﬂ§ø§§§ ª»Õ— ˝À°ª˛
					scr = GrphScrollY[((adr>>18)&2)+1];		//
					line = (((adr&0x7ffff)>>10)-scr)&511;		//
					if (adr&0x80000) adr+=1;
					adr &= 0x7ffff;
					GVRAM[adr] = data;
				}
			}
//			else
//			{
//				BusErrFlag = 1;
//				return;
//			}
			break;
		case 3:					// 65536 colors
			if ( adr<0x80000 )
			{
				GVRAM[adr] = data;
				line = (((adr&0x7ffff)>>10)-GrphScrollY[0])&511;
			}
//			else
//			{
//				BusErrFlag = 1;
//				return;
//			}
			break;
		}
		TextDirtyLine[line] = 1;
	}
}


// -----------------------------------------------------------------------
//   §≥§√§´§È∏Â§œ•È•§•Û√±∞Ã§«§Œ≤ËÃÃ≈∏≥´…ÅE
// -----------------------------------------------------------------------
LABEL void Grp_DrawLine16(void)
{
	WORD *srcp, *destp;
	DWORD x, y;
	DWORD i;
	WORD v, v0;

	quit_if_main_thread();
	debugThreadInfo("Grp_DrawLine16");

	y = GrphScrollY[0] + CURRENT_VLINE;
	if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
		y += CURRENT_VLINE;
	y = (y & 0x1ff) << 10;

	x = GrphScrollX[0] & 0x1ff;
	srcp = (WORD *)(GVRAM + y + x * 2);
	destp = (WORD *)Grp_LineBuf;

	x = (x ^ 0x1ff) + 1;

	v = v0 = 0;
	i = 0;
	if (x < TextDotX) {
		for (; i < x; ++i) {
			v = *srcp++;
			if (v != 0) {
				v0 = (v >> 8) & 0xff;
				v &= 0x00ff;

				v = Pal_Regs[Pal16Adr[v]];
				v |= Pal_Regs[Pal16Adr[v0] + 2] << 8;
				v = Pal16[v];
			}
			*destp++ = v;
		}
		srcp -= 0x200;
	}

	for (; i < TextDotX; ++i) {
		v = *srcp++;
		if (v != 0) {
			v0 = (v >> 8) & 0xff;
			v &= 0x00ff;

			v = Pal_Regs[Pal16Adr[v]];
			v |= Pal_Regs[Pal16Adr[v0] + 2] << 8;
			v = Pal16[v];
		}
		*destp++ = v;
	}
}


LABEL void FASTCALL Grp_DrawLine8(int page, int opaq)
{
	WORD  *__restrict srcp;
	WORD * __restrict destp;
	DWORD x, x0;
	DWORD y, y0;
	DWORD off;
	DWORD i;
	WORD v;

	quit_if_main_thread();

	page &= 1;

	y = GrphScrollY[page * 2] + CURRENT_VLINE;
	y0 = GrphScrollY[page * 2 + 1] + CURRENT_VLINE;
	if ((CRTC_Regs[0x29] & 0x1c) == 0x1c) {
		y += CURRENT_VLINE;
		y0 += CURRENT_VLINE;
	}
	y = ((y & 0x1ff) << 10) + page;
	y0 = ((y0 & 0x1ff) << 10) + page;

	x = GrphScrollX[page * 2] & 0x1ff;
	x0 = GrphScrollX[page * 2 + 1] & 0x1ff;

	off = y0 + x0 * 2;
	srcp = (WORD *)(GVRAM + y + x * 2);
	destp = (WORD *)Grp_LineBuf;

	x = (x ^ 0x1ff) + 1;

	v = 0;
	i = 0;

	if (opaq) {
		if (x < TextDotX) {
			for (; i < x; ++i) {
				v = GET_WORD_W8(srcp);
				srcp++;
				v = GrphPal[(GVRAM[off] & 0xf0) | (v & 0x0f)];
				*destp++ = v;

				off += 2;
				if ((off & 0x3fe) == 0x000)
					off -= 0x400;
			}
			srcp -= 0x200;
		}

		for (; i < TextDotX; ++i) {
			v = GET_WORD_W8(srcp);
			srcp++;
			v = GrphPal[(GVRAM[off] & 0xf0) | (v & 0x0f)];
			*destp++ = v;

			off += 2;
			if ((off & 0x3fe) == 0x000)
				off -= 0x400;
		}
	} else {
		if (x < TextDotX) {
			for (; i < x; ++i) {
				v = GET_WORD_W8(srcp);
				srcp++;
				v = (GVRAM[off] & 0xf0) | (v & 0x0f);
				if (v != 0x00)
					*destp = GrphPal[v];
				destp++;

				off += 2;
				if ((off & 0x3fe) == 0x000)
					off -= 0x400;
			}
			srcp -= 0x200;
		}

		for (; i < TextDotX; ++i) {
			v = GET_WORD_W8(srcp);
			srcp++;
			v = (GVRAM[off] & 0xf0) | (v & 0x0f);
			if (v != 0x00)
				*destp = GrphPal[v];
			destp++;

			off += 2;
			if ((off & 0x3fe) == 0x000)
				off -= 0x400;
		}
	}
}

				// Manhattan Requiem Opening 7.0¢™7.5MHz
LABEL void FASTCALL Grp_DrawLine4(DWORD page, int opaq)
{
	WORD *srcp, *destp;	// XXX: ALIGN
	DWORD x, y;
	DWORD off;
	DWORD i;
	WORD v;

	page &= 3;

	y = GrphScrollY[page] + CURRENT_VLINE;
	if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
		y += CURRENT_VLINE;
	y = (y & 0x1ff) << 10;

	x = GrphScrollX[page] & 0x1ff;
	off = y + x * 2;

	x ^= 0x1ff;

	srcp = (WORD *)(GVRAM + off + (page >> 1));
	destp = (WORD *)Grp_LineBuf;

	v = 0;
	i = 0;

	if (page & 1) {
		if (opaq) {
			if (x < TextDotX) {
				for (; i < x; ++i) {
					v = GET_WORD_W8(srcp);
					srcp++;
					v = GrphPal[(v >> 4) & 0xf];
					*destp++ = v;
				}
				srcp -= 0x200;
			}
			for (; i < TextDotX; ++i) {
				v = GET_WORD_W8(srcp);
				srcp++;
				v = GrphPal[(v >> 4) & 0xf];
				*destp++ = v;
			}
		} else {
			if (x < TextDotX) {
				for (; i < x; ++i) {
					v = GET_WORD_W8(srcp);
					srcp++;
					v = (v >> 4) & 0x0f;
					if (v != 0x00)
						*destp = GrphPal[v];
					destp++;
				}
				srcp -= 0x200;
			}
			for (; i < TextDotX; ++i) {
				v = GET_WORD_W8(srcp);
				srcp++;
				v = (v >> 4) & 0x0f;
				if (v != 0x00)
					*destp = GrphPal[v];
				destp++;
			}
		}
	} else {
		if (opaq) {
			if (x < TextDotX) {
				for (; i < x; ++i) {
					v = GET_WORD_W8(srcp);
					srcp++;
					v = GrphPal[v & 0x0f];
					*destp++ = v;
				}
				srcp -= 0x200;
			}
			for (; i < TextDotX; ++i) {
				v = GET_WORD_W8(srcp);
				srcp++;
				v = GrphPal[v & 0x0f];
				*destp++ = v;
			}
		} else {
			if (x < TextDotX) {
				for (; i < x; ++i) {
					v = GET_WORD_W8(srcp);
					srcp++;
					v &= 0x0f;
					if (v != 0x00)
						*destp = GrphPal[v];
					destp++;
				}
				srcp -= 0x200;
			}
			for (; i < TextDotX; ++i) {
				v = GET_WORD_W8(srcp);
				srcp++;
				v &= 0x0f;
				if (v != 0x00)
					*destp = GrphPal[v];
				destp++;
			}
		}
	}
} 

					// §≥§Œ≤ËÃÃ•‚°º•…§œ¥™ €§∑§∆≤º§µ§§°ƒ
void FASTCALL Grp_DrawLine4h(void)
{
	WORD *srcp, *destp;
	DWORD x, y;
	DWORD i;
	WORD v;
	int bits;

	y = GrphScrollY[0] + CURRENT_VLINE;
	if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
		y += CURRENT_VLINE;
	y &= 0x3ff;

	if ((y & 0x200) == 0x000) {
		y <<= 10;
		bits = (GrphScrollX[0] & 0x200) ? 4 : 0;
	} else {
		y = (y & 0x1ff) << 10;
		bits = (GrphScrollX[0] & 0x200) ? 12 : 8;
	}

	x = GrphScrollX[0] & 0x1ff;
	srcp = (WORD *)(GVRAM + y + x * 2);
	destp = (WORD *)Grp_LineBuf;

	x = ((x & 0x1ff) ^ 0x1ff) + 1;

	for (i = 0; i < TextDotX; ++i) {
		v = *srcp++;
		*destp++ = GrphPal[(v >> bits) & 0x0f];

		if (--x == 0) {
			srcp -= 0x200;
			bits ^= 4;
			x = 512;
		}
	}
}


// -------------------------------------------------
// --- »æ∆©Ã¿°ø∆√ºÅEri§Œ•Ÿ°º•π§»§ §ÅE⁄°º•∏§Œ…¡≤ÅE---
// -------------------------------------------------
void FASTCALL Grp_DrawLine16SP(void)
{
	DWORD x, y;
	DWORD off;
	DWORD i;
	WORD v;

	y = GrphScrollY[0] + CURRENT_VLINE;
	if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
		y += CURRENT_VLINE;
	y = (y & 0x1ff) << 10;

	x = GrphScrollX[0] & 0x1ff;
	off = y + x * 2;
	x = (x ^ 0x1ff) + 1;

	for (i = 0; i < TextDotX; ++i) {
		v = (Pal_Regs[GVRAM[off+1]*2] << 8) | Pal_Regs[GVRAM[off]*2+1];
	
		if ((GVRAM[off] & 1) == 0) {
			Grp_LineBufSP[i] = 0;
			Grp_LineBufSP2[i] = Pal16[v & 0xfffe];
		} else {
			Grp_LineBufSP[i] = Pal16[v & 0xfffe];
			Grp_LineBufSP2[i] = 0;
		}

		off += 2;
		if (--x == 0)
			off -= 0x400;
	}
}


void FASTCALL Grp_DrawLine8SP(int page)
{
	DWORD x, x0;
	DWORD y, y0;
	DWORD off, off0;
	WORD woff, woff0;
	DWORD i;
	BYTE v;

	page &= 1;

	y = GrphScrollY[page * 2] + CURRENT_VLINE;
	y0 = GrphScrollY[page * 2 + 1] + CURRENT_VLINE;
	if ((CRTC_Regs[0x29] & 0x1c) == 0x1c) {
		y += CURRENT_VLINE;
		y0 += CURRENT_VLINE;
	}
	y = (y & 0x1ff) << 10;
	y0 = (y0 & 0x1ff) << 10;

	x = GrphScrollX[page * 2] & 0x1ff;
	x0 = GrphScrollX[page * 2 + 1] & 0x1ff;

//	off = y + x * 2 + page
//	off0 = y0 + x0 * 2 + page;
	off = y + x * 2;
	off0 = y0 + x0 * 2 ;

	x = (x ^ 0x1ff) + 1;
	woff  = (off  & 0x3fe) >> 1;
	woff0 = (off0 & 0x3fe) >> 1;

	off  &= 0xfffffc00;
	off0 &= 0xfffffc00;
	off  |= page;
	off0 |= page;

	#pragma GCC ivdep
	for (i = 0; i < TextDotX; ++i, woff++, woff0++) {
		v = (GVRAM[off | ((woff << 1) & 0x3fe )] & 0x0f) | (GVRAM[off0 | ((woff0 << 1) & 0x3fe) ] & 0xf0);
		Grp_LineBufSP[i]  = (-( v&1)) &  (GrphPal[v & 0xfe] | Ibit);
		Grp_LineBufSP2[i] = (-(~v&1)) &   (v ?  GrphPal[v] : 0);
		Grp_LineBufSP_Tr[i] = (-(~v&1)) & (v ? !GrphPal[v] : 0);
	}
}


void FASTCALL Grp_DrawLine4SP(DWORD page/*, int opaq*/)
{
	DWORD scrx, scry;
	page &= 3;
	switch(page)		// »˛§∑§Ø§ §µ§π§Æ§ÅE æ–°À
	{
	case 0:	scrx = GrphScrollX[0]; scry = GrphScrollY[0]; break;
	case 1: scrx = GrphScrollX[1]; scry = GrphScrollY[1]; break;
	case 2: scrx = GrphScrollX[2]; scry = GrphScrollY[2]; break;
	case 3: scrx = GrphScrollX[3]; scry = GrphScrollY[3]; break;
	}

{
	DWORD x, y;
	DWORD off;
	DWORD i;
	WORD v;

	if (page & 1) {
		y = scry + CURRENT_VLINE;
		if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
			y += CURRENT_VLINE;
		y = (y & 0x1ff) << 10;

		x = scrx & 0x1ff;
		off = y + x * 2;
		if (page & 2)
			off++;
		x = (x ^ 0x1ff) + 1;

		for (i = 0; i < TextDotX; ++i) {
			v = GVRAM[off] >> 4;
			if ((v & 1) == 0) {
				v &= 0x0e;
				Grp_LineBufSP[i] = 0;
				Grp_LineBufSP2[i] = GrphPal[v];
			} else {
				v &= 0x0e;
				Grp_LineBufSP[i] = GrphPal[v];
				Grp_LineBufSP2[i] = 0;
			}

			off += 2;
			if (--x == 0)
				off -= 0x400;
		}
	} else {
		y = scry + CURRENT_VLINE;
		if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
			y += CURRENT_VLINE;
		y = (y & 0x1ff) << 10;

		x = scrx & 0x1ff;
		off = y + x * 2;
		if (page & 2)
			off++;
		x = (x ^ 0x1ff) + 1;

		for (i = 0; i < TextDotX; ++i) {
			v = GVRAM[off];
			if ((v & 1) == 0) {
				v &= 0x0e;
				Grp_LineBufSP[i] = 0;
				Grp_LineBufSP2[i] = GrphPal[v];
			} else {
				v &= 0x0e;
				Grp_LineBufSP[i] = GrphPal[v];
				Grp_LineBufSP2[i] = 0;
			}

			off += 2;
			if (--x == 0)
				off -= 0x400;
		}
	}
}
}


void FASTCALL Grp_DrawLine4hSP(void)
{
	WORD *srcp;
	DWORD x, y;
	DWORD i;
	int bits;
	WORD v;

	y = GrphScrollY[0] + CURRENT_VLINE;
	if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
		y += CURRENT_VLINE;
	y &= 0x3ff;

	if ((y & 0x200) == 0x000) {
		y <<= 10;
		bits = (GrphScrollX[0] & 0x200) ? 4 : 0;
	} else {
		y = (y & 0x1ff) << 10;
		bits = (GrphScrollX[0] & 0x200) ? 12 : 8;
	}

	x = GrphScrollX[0] & 0x1ff;
	srcp = (WORD *)(GVRAM + y + x * 2);
	x = ((x & 0x1ff) ^ 0x1ff) + 1;

	for (i = 0; i < TextDotX; ++i) {
		v = *srcp++ >> bits;
		if ((v & 1) == 0) {
			Grp_LineBufSP[i] = 0;
			Grp_LineBufSP2[i] = GrphPal[v & 0x0e];
		} else {
			Grp_LineBufSP[i] = GrphPal[v & 0x0e];
			Grp_LineBufSP2[i] = 0;
		}

		if (--x == 0)
			srcp -= 0x400;
	}
}



// -------------------------------------------------
// --- »æ∆©Ã¿§Œ¬–æ›§»§ §ÅE⁄°º•∏§Œ…¡≤ÅE--------------
// 2•⁄°º•∏∞ æÂ§¢§ÅE∞•È•’•£•√•Ø•‚°º•…§Œ§ﬂ§ §Œ§«°¢
// 256øß2ÃÃ or 16øß4ÃÃ§Œ•‚°º•…§Œ§ﬂ°£
// 256øßª˛§œ°¢Opaque§«§ §§ ˝§Œ•‚°º•…§œ§§§È§ §§§´§‚°ƒ
// ° …¨§∫Opaque•‚°º•…§Œ»¶°À
// -------------------------------------------------
// §≥§≥§œ§ﬁ§¿32øßx4ÃÃ•‚°º•…§Œº¬¡ı§Ú§∑§∆§ §§§ÅEπ°ƒ
// ° §ÅE∏§π§ø¬≠§Í§ §§§Ë§•°ƒ°À
// -------------------------------------------------
							// §‰§±§À§π§√§≠§ÅE
LABEL void FASTCALL
Grp_DrawLine8TR(int page, int opaq)
{
	if (opaq) {
		DWORD x, y;
		DWORD v, v0;
		DWORD i;

		page &= 1;

		y = GrphScrollY[page * 2] + CURRENT_VLINE;
		if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
			y += CURRENT_VLINE;
		y = ((y & 0x1ff) << 10) + page;
		x = GrphScrollX[page * 2] & 0x1ff;

		for (i = 0; i < TextDotX; ++i, x = (x + 1) & 0x1ff) {
			v0 = Grp_LineBufSP[i];
			v = GVRAM[y + x * 2];

			if (v0 != 0) {
				if (v != 0) {
					v = GrphPal[v];
					if (v != 0) {
						v0 &= Pal_HalfMask;
						if (v & Ibit)
							v0 |= Pal_Ix2;
						v &= Pal_HalfMask;
						v += v0;
						v >>= 1;
					}
				}
			} else
				v = GrphPal[v];
			Grp_LineBuf[i] = (WORD)v;
		}
	}
}

LABEL void FASTCALL
Grp_DrawLine8TR_GT(int page, int opaq)
{
	if (opaq) {
		DWORD x, y;
		DWORD v, v0;
		DWORD i;

		page &= 1;

		y = GrphScrollY[page * 2] + CURRENT_VLINE;
		if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
			y += CURRENT_VLINE;
		y = ((y & 0x1ff) << 10) + page;
		x = GrphScrollX[page * 2] & 0x1ff;

		for (i = 0; i < TextDotX; ++i, x = (x + 1) & 0x1ff) {
			Grp_LineBuf[i] = (Grp_LineBufSP[i] || Grp_LineBufSP_Tr[i]) ? 0 : GrphPal[GVRAM[y + x * 2]];
			Grp_LineBufSP_Tr[i] = 0;
		}
	}
}

LABEL void FASTCALL
Grp_DrawLine4TR(DWORD page, int opaq)
{
	DWORD x, y;
	DWORD v, v0;
	DWORD i;

	page &= 3;

	y = GrphScrollY[page] + CURRENT_VLINE;
	if ((CRTC_Regs[0x29] & 0x1c) == 0x1c)
		y += CURRENT_VLINE;
	y = (y & 0x1ff) << 10;
	x = GrphScrollX[page] & 0x1ff;

	if (page & 1) {
		page >>= 1;
		y += page;

		if (opaq) {
			for (i = 0; i < TextDotX; ++i, x = (x + 1) & 0x1ff) {
				v0 = Grp_LineBufSP[i];
				v = GVRAM[y + x * 2] >> 4;

				if (v0 != 0) {
					if (v != 0) {
						v = GrphPal[v];
						if (v != 0) {
							v0 &= Pal_HalfMask;
							if (v & Ibit)
								v0 |= Pal_Ix2;
							v &= Pal_HalfMask;
							v += v0;
							v >>= 1;
						}
					}
				} else
					v = GrphPal[v];
				Grp_LineBuf[i] = (WORD)v;
			}
		} else {
			for (i = 0; i < TextDotX; ++i, x = (x + 1) & 0x1ff) {
				v0 = Grp_LineBufSP[i];

				if (v0 == 0) {
					v = GVRAM[y + x * 2] >> 4;
					if (v != 0)
						Grp_LineBuf[i] = GrphPal[v];
				} else {
					v = GVRAM[y + x * 2] >> 4;
					if (v != 0) {
						v = GrphPal[v];
						if (v != 0) {
							v0 &= Pal_HalfMask;
							if (v & Ibit)
								v0 |= Pal_Ix2;
							v &= Pal_HalfMask;
							v += v0;
							v = GrphPal[v >> 1];
							Grp_LineBuf[i]=(WORD)v;
						}
					} else
						Grp_LineBuf[i] = (WORD)v;
				}
			}
		}
	} else {
		page >>= 1;
		y += page;

		if (opaq) {
			for (i = 0; i < TextDotX; ++i, x = (x + 1) & 0x1ff) {
				v = GVRAM[y + x * 2] & 0x0f;
				v0 = Grp_LineBufSP[i];

				if (v0 != 0) {
					if (v != 0) {
						v = GrphPal[v];
						if (v != 0) {
							v0 &= Pal_HalfMask;
							if (v & Ibit)
								v0 |= Pal_Ix2;
							v &= Pal_HalfMask;
							v += v0;
							v >>= 1;
						}
					}
				} else
					v = GrphPal[v];
				Grp_LineBuf[i] = (WORD)v;
			}
		} else {
			for (i = 0; i < TextDotX; ++i, x = (x + 1) & 0x1ff) {
				v = GVRAM[y + x * 2] & 0x0f;
				v0 = Grp_LineBufSP[i];

				if (v0 != 0) {
					if (v != 0) {
						v = GrphPal[v];
						if (v != 0) {
							v0 &= Pal_HalfMask;
							if (v & Ibit)
								v0 |= Pal_Ix2;
							v &= Pal_HalfMask;
							v += v0;
							v >>= 1;
							Grp_LineBuf[i]=(WORD)v;
						}
					} else
						Grp_LineBuf[i] = (WORD)v;
				} else if (v != 0)
					Grp_LineBuf[i] = GrphPal[v];
			}
		}
	}
}

/*
MS-C §Œ§ÅEgas §Œ•§•Û•È•§•Û•¢•ª•Û•÷•Í§À§∆§±§»°º§À —¥π

s/\<.*:/.&/
s/[^ \t]/"&/
s/[^ \t:]$/&;"/
s/[^ \t]:$/&"/
s/\<e*[abcd][xlh]\>/%%&/g
s/\<e*[ds]i\>/%%&/g
s/\<[0-9]*[^h]\>/$&/g
s/\([^%]\)\(\<0*[0-9a-fA-F]*\)h/\1$0x\2/g
s/"\([^ \t]*\)[ \t][ \t]*\(.*\), \(.*\);/"\1 \3, \2;/
                                            + §≥§≥§œ•ø•÷
s/\[/(/g
s/]/)/g
*/
