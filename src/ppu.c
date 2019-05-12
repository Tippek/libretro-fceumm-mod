/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2003 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include        <string.h>
#include        <stdio.h>
#include        <stdlib.h>

#include        "fceu-types.h"
#include        "x6502.h"
#include        "fceu.h"
#include        "ppu.h"
#include        "nsf.h"
#include        "sound.h"
#include        "general.h"
#include        "fceu-endian.h"
#include        "fceu-memory.h"
#include        "ppuview.h"

#include        "cart.h"
#include        "palette.h"
#include        "state.h"
#include        "video.h"
#include        "input.h"

#define VBlankON        (PPU[0] & 0x80)		/* Generate VBlank NMI */
#define Sprite16        (PPU[0] & 0x20)		/* Sprites 8x16/8x8 */
#define BGAdrHI         (PPU[0] & 0x10)		/* BG pattern adr $0000/$1000 */
#define SpAdrHI         (PPU[0] & 0x08)		/* Sprite pattern adr $0000/$1000 */
#define INC32           (PPU[0] & 0x04)		/* auto increment 1/32 */

#define SpriteON        (PPU[1] & 0x10)		/* Show Sprite */
#define ScreenON        (PPU[1] & 0x08)		/* Show screen */
#define GRAYSCALE       (PPU[1] & 0x01)		/* Grayscale (AND palette entries with 0x30) */
#define READPAL(ofs)    (PALRAM[(ofs)] & (GRAYSCALE ? 0x30 : 0xFF))
#define READUPAL(ofs) (UPALRAM[(ofs)] & (GRAYSCALE ? 0x30 : 0xFF))
#define PPU_status      (PPU[2])

#define Pal             (PALRAM)

static void FetchSpriteData(void);
static void FASTAPASS(1) RefreshLine(int lastpixel);
static void RefreshSprites(void);
static void CopySprites(uint8 *target);
int sprites256;
int exscanlines;
int vblines;
int vt03_mmc3_flag;
int vt03_mode;
int wscre, wssub;

uint8 stopclock;
uint8 spr_add_flag;
uint8 spr_add_atr[0x100];
static void Fixit1(void);
static void Fixit12(void);
static uint32 ppulut1[256];
static uint32 ppulut2[256];
static uint32 ppulut3[128];
static uint32 ppulut4[256];
static uint32 ppulut5[256];
static uint32 ppulut6[256*8];
static uint32 ppulut7[256*8];
uint8 extra_ppu[0x10000];
static void makeppulut(void) {
	int x;
	int y;
	int cc, xo, pixel;


	for (x = 0; x < 256; x++) {
		ppulut1[x] = 0;
		for (y = 0; y < 8; y++)
			ppulut1[x] |= ((x >> (7 - y)) & 1) << (y * 4);
		ppulut2[x] = ppulut1[x] << 1;
		ppulut4[x] = ppulut1[x] << 2;
		ppulut5[x] = ppulut1[x] << 3;
	}
	  for(cc=0;cc<256;cc++)
  {
   for(xo=0;xo<8;xo++)
   {
    ppulut6[xo|(cc<<3)]=0;
    for(pixel=0;pixel<8;pixel++)
    {
     int shiftr;
      shiftr=(pixel+xo)/8;
      shiftr*=4;
      ppulut6[xo|(cc<<3)]|=(( cc>>shiftr )&0xF)<<(0+pixel*4);
    }
//    printf("%08x\n",ppulut3[xo|(cc<<3)]);
   }
  }
	for (cc = 0; cc < 16; cc++) {
		for (xo = 0; xo < 8; xo++) {
			ppulut3[xo | (cc << 3)] = 0;
			for (pixel = 0; pixel < 8; pixel++) {
				int shiftr;
				shiftr = (pixel + xo) / 8;
				shiftr *= 2;
				ppulut3[xo | (cc << 3)] |= ((cc >> shiftr) & 3) << (2 + pixel * 4);

			}
		}
	}
}

static int ppudead = 1;
static int kook = 0;
int fceuindbg = 0;

int MMC5Hack = 0, PEC586Hack = 0;
uint32 MMC5HackVROMMask = 0;
uint8 *MMC5HackExNTARAMPtr = 0;
uint8 *MMC5HackVROMPTR = 0;
uint8 MMC5HackCHRMode = 0;
uint8 MMC5HackSPMode = 0;
uint8 MMC50x5130 = 0;
uint8 MMC5HackSPScroll = 0;
uint8 MMC5HackSPPage = 0;

uint8 VRAMBuffer = 0, PPUGenLatch = 0;
uint8 *vnapage[4];
uint8 *vnapage2[4];
uint8 PPUNTARAM = 0;
uint8 PPUCHRRAM = 0;

/* Color deemphasis emulation.  Joy... */
static uint8 deemp = 0;
static int deempcnt[8];

void (*GameHBIRQHook)(void), (*GameHBIRQHook2)(void);
void FP_FASTAPASS(1) (*PPU_hook)(uint32 A);

uint8 vtoggle = 0;
uint8 XOffset = 0;
uint8 vtoggle2 = 0;
uint8 XOffset2 = 0;
uint32 TempAddr = 0, RefreshAddr = 0;
uint32 TempAddr2 = 0, RefreshAddr2 = 0;
static int maxsprites = 8; //mod: who need limits?
							//mod: Felix Cat require limit...okay

/* scanline is equal to the current visible scanline we're on. */
int scanline;
static uint32 scanlines_per_frame;

typedef struct {
	uint8 y, no, atr, x;
} SPR;

typedef struct {
	uint8 ca[8];
     uint8 atr, x;
} SPRB;

uint16 PPU[4];
static SPRB SPRBUF[0x400];
int PPUSPL;
uint8 NTARAM[0x800], PALRAM[0x100], SPRAM[0x400], NTARAM2[0x800], NTATRIB[0x800], NTATRIB2[0x800];
uint8 UPALRAM[0x03];/* for 0x4/0x8/0xC addresses in palette, the ones in
					 * 0x20 are 0 to not break fceu rendering.
					 */

#define MMC5SPRVRAMADR(V)   &MMC5SPRVPage[(V) >> 10][(V)]
#define VRAMADR(V)          &VPage[(V) >> 10][(V)]

uint8 * MMC5BGVRAMADR(uint32 V) {
	if (!Sprite16) {
		extern uint8 mmc5ABMode;				/* A=0, B=1 */
		if (mmc5ABMode == 0)
			return MMC5SPRVRAMADR(V);
		else
			return &MMC5BGVPage[(V) >> 10][(V)];
	} else return &MMC5BGVPage[(V) >> 10][(V)];
}

static DECLFR(A2002) {
	uint8 ret;

	FCEUPPU_LineUpdate();
	ret = PPU_status;
	ret |= PPUGenLatch & 0x1F;

#ifdef FCEUDEF_DEBUGGER
	if (!fceuindbg)
#endif
	{
		vtoggle = 0;
		vtoggle2 = 0;
		PPU_status &= 0x7F;
		PPUGenLatch = ret;
	}

	return ret;
}

static DECLFR(A200x) {	/* Not correct for $2004 reads. */
	FCEUPPU_LineUpdate();
	return PPUGenLatch;
}

static DECLFR(A2007) {
	uint8 ret;
	uint32 tmp = RefreshAddr & 0xFFFF;
if(!sprites256) tmp &= 0x3FFF;
	FCEUPPU_LineUpdate();

		if (tmp >= 0x3F00 && tmp <= 0x3FFF) {	// Palette RAM tied directly to the output data, without VRAM buffer
			if (!(tmp & 0xF)) {
			if (!(tmp & 0xC))
				ret = PALRAM[0x00];
			else
				ret = UPALRAM[((tmp & 0xC) >> 2) - 1];
		} else
			ret = PALRAM[tmp & 0xFF];
		if (GRAYSCALE)
			ret &= 0x30;
		#ifdef FCEUDEF_DEBUGGER
		if (!fceuindbg)
		#endif
		{
				if ((tmp - 0x1000) < 0x2000 || (tmp - 0x1000)>0x3fff)
				{
					if(tmp<0x4000)VRAMBuffer = VPage[(tmp - 0x1000) >> 10][tmp - 0x1000];
					else VRAMBuffer =extra_ppu[tmp-0x1000];
				}
			else
				VRAMBuffer = vnapage[((tmp - 0x1000) >> 10) & 0x3][(tmp - 0x1000) & 0x3FF];
			if (PPU_hook) PPU_hook(tmp);
		}
	} else {
		ret = VRAMBuffer;
		#ifdef FCEUDEF_DEBUGGER
		if (!fceuindbg)
		#endif
		{
			if (PPU_hook) PPU_hook(tmp);
			PPUGenLatch = VRAMBuffer;
				if (tmp < 0x2000 || tmp > 0x7fff) {
				if(tmp<0x4000 || (tmp<0xA000 && vt03_mmc3_flag))VRAMBuffer = VPage[tmp >> 10][tmp];
					else VRAMBuffer = extra_ppu[tmp];
			} else if (tmp < 0x3F00)
				VRAMBuffer = vnapage[(tmp >> 10) & 0x3][tmp & 0x3FF];
		}
	}

	#ifdef FCEUDEF_DEBUGGER
	if (!fceuindbg)
	#endif
	{
		if ((ScreenON || SpriteON) && (scanline < 240)) {
			uint32 rad = RefreshAddr;
			if ((rad & 0x7000) == 0x7000) {
				rad ^= 0x7000;
				if ((rad & 0x3E0) == 0x3A0)
					rad ^= 0xBA0;
				else if ((rad & 0x3E0) == 0x3e0)
					rad ^= 0x3e0;
				else
					rad += 0x20;
			} else
				rad += 0x1000;
			RefreshAddr = rad;
		} else {
		  if (INC32)				{
					if(!wscre)RefreshAddr += 32;
					else RefreshAddr += 64;
				}
			else
				RefreshAddr++;
		}
		if (PPU_hook) PPU_hook(RefreshAddr & 0xffff);
	}
	return ret;
}

static DECLFW(B2000) {
	FCEUPPU_LineUpdate();
	PPUGenLatch = V;

	if (!(PPU[0] & 0x80) && (V & 0x80) && (PPU_status & 0x80))
		TriggerNMI2();

	PPU[0] = V;
	TempAddr &= 0xF3FF;
	TempAddr |= (V & 3) << 10;
}

static DECLFW(B3000) {
	FCEUPPU_LineUpdate();
	PPUGenLatch = V;

	TempAddr2 &= 0xF3FF;
	TempAddr2 |= (V & 3) << 10;
}
static DECLFW(B2001) {
	FCEUPPU_LineUpdate();
	PPUGenLatch = V;
	PPU[1] = V;
	if (V & 0xE0)
		deemp = V >> 5;
}

static DECLFW(B2002) {
	PPUGenLatch = V;
}

static DECLFW(B2003) {
	PPUGenLatch = V;
	PPU[3] = V;
	PPUSPL = V & 0x7;
}

static DECLFW(B2004) {
	PPUGenLatch = V;
	if (PPUSPL >= 8) {
		if (PPU[3] >= 8)
			SPRAM[PPU[3]] = V;
	} else {
		SPRAM[PPUSPL] = V;
	}
	PPU[3]++;
	PPUSPL++;
		if (sprites256) { if (PPU[3] == 0x400)PPU[3] = 0; }
		if (!sprites256) { if (PPU[3] == 0x100)PPU[3] = 0; }
		if (sprites256) { if (PPUSPL == 0x400)PPUSPL = 0; }
		if (!sprites256) { if (PPUSPL == 0x100)PPUSPL = 0; }
}

static DECLFW(B2005) {
	uint32 tmp = TempAddr;
	FCEUPPU_LineUpdate();
	PPUGenLatch = V;
	if (!vtoggle) {
		tmp &= 0xFFE0;
		tmp |= V >> 3;
		XOffset = V & 7;
	} else {
		tmp &= 0x8C1F;
		tmp |= ((V & ~0x7) << 2);
		tmp |= (V & 7) << 12;
	}
	TempAddr = tmp;
	vtoggle ^= 1;
}

static DECLFW(B3005) {
	uint32 tmp = TempAddr2;
	FCEUPPU_LineUpdate();
	PPUGenLatch = V;
	if (!vtoggle2) {
		tmp &= 0xFFE0;
		tmp |= V >> 3;
		XOffset2 = V & 7;
	} else {
		tmp &= 0x8C1F;
		tmp |= ((V & ~0x7) << 2);
		tmp |= (V & 7) << 12;
	}
	TempAddr2 = tmp;
	vtoggle2 ^= 1;
}

static DECLFW(B2006) {
	FCEUPPU_LineUpdate();

	PPUGenLatch = V;
	if (!vtoggle) {
		TempAddr &= 0x00FF;
		TempAddr |= (V & 0xff) << 8;
	} else {
		TempAddr &= 0xFF00;
		TempAddr |= V;

		RefreshAddr = TempAddr;
		if (PPU_hook)
			PPU_hook(RefreshAddr);
	}
	if(!sprites256)TempAddr &= 0x3FFF;
	vtoggle ^= 1;
}
static DECLFW(B3006) {
	FCEUPPU_LineUpdate();

	PPUGenLatch = V;
	if (!vtoggle2) {
		TempAddr2 &= 0x00FF;
		TempAddr2 |= (V & 0xff) << 8;
	} else {
		TempAddr2 &= 0xFF00;
		TempAddr2 |= V;

		RefreshAddr2 = TempAddr2;
		if (PPU_hook)
			PPU_hook(RefreshAddr2);
	}
	vtoggle2 ^= 1;
}
static DECLFW(B2007) {
	uint32 tmp = RefreshAddr & 0xfFFF;
	if(!sprites256) tmp &= 0x3FFF;
		FCEUPPU_LineUpdate();
	PPUGenLatch = V;
	if (tmp < 0x2000 || tmp > 0x3FFF) {
//mod: we don't need protected vram in extra mode?
if (PPUCHRRAM & (1 << (tmp >> 10)) || !sprites256) //mod: still need for original games(codemaster?)
			if(tmp<0x4000)VPage[tmp >> 10][tmp] = V;

					extra_ppu[tmp]=V;				//mod: save all vram anyway(debug and some codes below)
	} else if (tmp < 0x3F00) 
	{
		
		if(!wscre) if(PPUNTARAM&(1<<((tmp&0xF00)>>10)))vnapage[((tmp&0xF00)>>10)][tmp&0x3FF]=V;
		//mod: emulating 64x32 tiles(like Genesis), just convert in standard format
		if(wscre)
		{
			if(tmp<0x2800) //mod: tiles
			{
				if(tmp&0x20)
				{
					int temp1 = tmp&0x1F;
					int temp2 = ((tmp&0x7C0)>>1)+temp1+0x2400;
                    vnapage[((temp2&0xF00)>>10)][temp2&0x3FF]=V;
				}
				else
				{
					int temp1 = tmp&0x1F;
					int temp2 = ((tmp&0x7E0)>>1)+temp1+0x2000;
					vnapage[((temp2&0xF00)>>10)][temp2&0x3FF]=V;
				}
			}
			else  //mod: attributes
			{
				if(tmp&0x20)
				{
					int temp1 = tmp&0x1F;
					int temp2 = ((tmp&0x7C0)>>1)+temp1+0x2400;
                    NTATRIB[temp2&0x7FF]=V;
				}
				else
				{
					int temp1 = tmp&0x1F;
					int temp2 = ((tmp&0x7E0)>>1)+temp1+0x2000;
                    NTATRIB[temp2&0x7FF]=V;
				}
			}
		}
		
	} else if(tmp<0x4000) {
			if(!V) V = 0x2d; //mod: we need 0x00 color for tranparency 
			if(vt03_mode)
			{
			if (!(tmp & 0xF)) {
				if (!(tmp & 0x30))
					PALRAM[0x00] = PALRAM[0x10] = PALRAM[0x20] = PALRAM[0x30] = V & 0x7f;
			} else
				PALRAM[tmp & 0xfF] = V & 0x7f;
			}
			else
			{
			if(!sprites256) V &=0x3F;
			if (!(tmp & 0x3)) {
				if (!(tmp & 0x0C))
					PALRAM[0x00] = PALRAM[0x4] = PALRAM[0x8] = PALRAM[0xC] = V & 0x7f;
				else
					UPALRAM[((tmp & 0xC) >> 2) - 1] = V & 0x7f;
			} else
				PALRAM[tmp & 0xfF] = V & 0x7f;
			}
		}
 

	if (INC32)
	{
		if(!wscre)RefreshAddr += 32;
		else RefreshAddr += 64;
	}
	else
		RefreshAddr++;
	if (PPU_hook)
		PPU_hook(RefreshAddr & 0xffff);
}

static DECLFW(B3007) {
	uint32 tmp = RefreshAddr2;
	PPUGenLatch = V;
	if(!wscre)
	{
		if(tmp<0x2000 || tmp>0x4000)extra_ppu[tmp+0xE000] = V;
		else vnapage2[((tmp & 0xF00) >> 10)][tmp & 0x3FF] = V;
	}
	else //mod: unfinished part, no demo for testing this...
	{
		if(tmp<0x2800)
		{
			if(tmp&0x20)
			{
				int temp1 = tmp&0x1F;
				int temp2 = ((tmp&0x7C0)>>1)+temp1+0x2400;
				vnapage2[((temp2&0xF00)>>10)][temp2&0x3FF]=V;
				
			}
			else
			{
				int temp1 = tmp&0x1F;
				int temp2 = ((tmp&0x7E0)>>1)+temp1+0x2000;
				vnapage2[((temp2&0xF00)>>10)][temp2&0x3FF]=V;
			}
		}
		else 
		{
			if(tmp&0x20)
			{
				int temp1 = tmp&0x1F;
				int temp2 = ((tmp&0x7C0)>>1)+temp1+0x2400;
				NTATRIB2[temp2&0x7FF]=V;
				
			}
			else
			{
				int temp1 = tmp&0x1F;
				int temp2 = ((tmp&0x7E0)>>1)+temp1+0x2000;
				NTATRIB2[temp2&0x7FF]=V;
			}
		}
	}
	
	
	
	if (INC32)
	{
		if(!wscre)RefreshAddr += 32;
		else RefreshAddr += 64;
	}
	else
		RefreshAddr2++;
	
	if (PPU_hook)
			PPU_hook(RefreshAddr2 &0xffff);
}
static DECLFW(B12005) {
	uint32 tmp = TempAddr;
	FCEUPPU_LineUpdate();
	PPUGenLatch = V;
	if (!vtoggle) {
		tmp &= 0xFFE0;
		tmp |= V >> 3;
		XOffset = V & 7;
	} else {
		tmp &= 0x8C1F;
		tmp |= ((V & ~0x7) << 2);
		tmp |= (V & 7) << 12;
	}
	TempAddr = tmp;
	vtoggle ^= 1;
}


static DECLFW(B12006) {
	FCEUPPU_LineUpdate();

	PPUGenLatch = V;
	if (!vtoggle) {
		TempAddr &= 0x00FF;
		TempAddr |= (V & 0x3f) << 8;
	} else {
		TempAddr &= 0xFF00;
		TempAddr |= V;

		RefreshAddr = TempAddr;
		if (PPU_hook)
			PPU_hook(RefreshAddr);
	}
	vtoggle ^= 1;
}

static DECLFW(B12007) {
	uint32 tmp = RefreshAddr & 0x3FFF;
	PPUGenLatch = V;
	if (tmp < 0x2000) {
		if (PPUCHRRAM & (1 << (tmp >> 10)))
			VPage[tmp >> 10][tmp] = V;
	} else if (tmp < 0x3F00) {
		if (PPUNTARAM & (1 << ((tmp & 0xF00) >> 10)))
			vnapage[((tmp & 0xF00) >> 10)][tmp & 0x3FF] = V;
	} else {
		if (!(tmp & 3)) {
			if (!(tmp & 0xC))
				PALRAM[0x00] = PALRAM[0x04] = PALRAM[0x08] = PALRAM[0x0C] = V & 0x3F;
			else
				UPALRAM[((tmp & 0xC) >> 2) - 1] = V & 0x3F;
		} else
			PALRAM[tmp & 0x1F] = V & 0x3F;
	}
	if (INC32)
		RefreshAddr += 32;
	else
		RefreshAddr++;
	if (PPU_hook)
		PPU_hook(RefreshAddr & 0x3fff);
}
uint8 okk;
uint8 colrnum;
uint8 colr;
uint8 colg;
uint8 colb;
static DECLFW(B401B) {
	if(!okk) colrnum = V;
	if(okk == 1) colr = V;
	if(okk == 2) colg = V;
	if(okk == 3)
	{
		okk = 0;
		palo[colrnum].r = colr;
		palo[colrnum].g = colg;
		palo[colrnum].b = V;
		WritePalette();
		
	}
	else okk++;
}
   int dmatgl;
 uint16 dmaread;
 uint16 dmawrite;
 uint16 dmalenght;
 static DECLFW(B4018) //mod: mini dma code
 {
	
	if(!dmatgl) dmaread = V;
	if(dmatgl==1) dmaread |= V<<8;
	if(dmatgl==2) dmawrite = V;
	if(dmatgl==3) dmawrite |= V<<8;
	if(dmatgl==4) dmalenght = V;
	if(dmatgl==5) dmalenght |= V<<8;
	if(dmatgl==6)								//mod: mode byte 0: rom/ram to ram 1: vram/vrom to vram 2: rom/ram to vram
	{
		dmatgl = 0;
		int Ax = V;
		switch(Ax)
		{
			case 0:
			{
			 for(int x = 0; x<dmalenght; x++)
				 X6502_DMW2(dmawrite+x, X6502_DMR2(dmaread+x));
			 break;
			}
			case 1:
			{
			 if(dmaread<0x2000 && dmawrite<0x2000)
			 {
				 for( int x = 0; x<dmalenght; x++)
				 {
				VPage[(dmawrite+x) >> 10][dmawrite+x] = VPage[(dmaread+x) >> 10][dmaread+x];
				extra_ppu[dmawrite+x] = VPage[(dmaread+x) >> 10][dmaread+x];	//
				 }
			 }
			 else if(dmaread<0x2000 && dmawrite>0x7FFF)
			 {
				 for( int x = 0; x<dmalenght; x++)
				 {
				extra_ppu[dmawrite+x] = VPage[(dmaread+x) >> 10][dmaread+x];
				 }
			 }
			 else if(dmaread>0x7FFF && dmawrite<0x2000)
			 {
			    for( int x = 0; x<dmalenght; x++)
				 {
				VPage[(dmawrite+x) >> 10][dmawrite+x] = extra_ppu[dmaread+x];
				 }
			 }
			 break;
			}
			case 2:
			{
				if(dmawrite<0x2000)
				{
				for( int x = 0; x<dmalenght; x++)
				 {
				VPage[(dmawrite+x) >> 10][dmawrite+x] = X6502_DMR2(dmaread+x);
				 }
				}
				else if(dmawrite>0x7FFF)
				{
				for( int x = 0; x<dmalenght; x++)
				 {
				extra_ppu[dmawrite+x] = X6502_DMR2(dmaread+x);
				 }
				}
				break;
			}
			
		}
	
	}
	else dmatgl++;

	
 }
 static DECLFW(B4021)
{
if(!spr_add_flag)spr_add_flag = 1;
int t = V<<8;
for(int x = 0; x<0x100; x++)
{
spr_add_atr[x]=X6502_DMR2(t+x);
}
}
static DECLFW(B4019) {  //mod: clean 0x1000 bytes of vram 
int t = V<<8;
for(int x = 0; x<0x1000; x++)
	extra_ppu[t+x] = 0;
}
static DECLFR(A12007) {
	uint8 ret;
	uint32 tmp = RefreshAddr & 0x3FFF;

	FCEUPPU_LineUpdate();

	if (tmp >= 0x3F00) {	/* Palette RAM tied directly to the output data, without VRAM buffer */
		if (!(tmp & 3)) {
			if (!(tmp & 0xC))
				ret = PALRAM[0x00];
			else
				ret = UPALRAM[((tmp & 0xC) >> 2) - 1];
		} else
			ret = PALRAM[tmp & 0x1F];
		if (GRAYSCALE)
			ret &= 0x30;
		#ifdef FCEUDEF_DEBUGGER
		if (!fceuindbg)
		#endif
		{
			if ((tmp - 0x1000) < 0x2000)
				VRAMBuffer = VPage[(tmp - 0x1000) >> 10][tmp - 0x1000];
			else
				VRAMBuffer = vnapage[((tmp - 0x1000) >> 10) & 0x3][(tmp - 0x1000) & 0x3FF];
			if (PPU_hook) PPU_hook(tmp);
		}
	} else {
		ret = VRAMBuffer;
		#ifdef FCEUDEF_DEBUGGER
		if (!fceuindbg)
		#endif
		{
			if (PPU_hook) PPU_hook(tmp);
			PPUGenLatch = VRAMBuffer;
			if (tmp < 0x2000) {
				VRAMBuffer = VPage[tmp >> 10][tmp];
			} else if (tmp < 0x3F00)
				VRAMBuffer = vnapage[(tmp >> 10) & 0x3][tmp & 0x3FF];
		}
	}

	#ifdef FCEUDEF_DEBUGGER
	if (!fceuindbg)
	#endif
	{
		if ((ScreenON || SpriteON) && (scanline < 240)) {
			uint32 rad = RefreshAddr;
			if ((rad & 0x7000) == 0x7000) {
				rad ^= 0x7000;
				if ((rad & 0x3E0) == 0x3A0)
					rad ^= 0xBA0;
				else if ((rad & 0x3E0) == 0x3e0)
					rad ^= 0x3e0;
				else
					rad += 0x20;
			} else
				rad += 0x1000;
			RefreshAddr = rad;
		} else {
			if (INC32)
				RefreshAddr += 32;
			else
				RefreshAddr++;
		}
		if (PPU_hook) PPU_hook(RefreshAddr & 0x3fff);
	}
	return ret;
}
static DECLFW(B401C) {

	stopclock = V;
}
static DECLFW(B401D) {

	minus_line = V;
}

static DECLFW(B4014) {
	uint32 t = V << 8;
	int x;

	if (sprites256) {
		for (x = 0;x<1024;x++)
		{
			if(x<768)X6502_DMW2(0x2004, X6502_DMR2(t + x));
			else X6502_DMW(0x2004, X6502_DMR(t + x));
		}
	}
	else {
		for (x = 0;x<256;x++)
			X6502_DMW(0x2004, X6502_DMR(t + x));
	}
}

#define PAL(c)  ((c) + cc)

#define GETLASTPIXEL    (PAL ? ((timestamp * 48 - linestartts) / 15) : ((timestamp * 48 - linestartts) >> 4))

static uint8 *Pline, *Plinef;
static int firsttile;
static int linestartts;
static int tofix = 0;

static DECLFR(A2004)
{
        if(Pline) //InPPUActiveArea)
        {
         int poopix = GETLASTPIXEL;

         if(poopix > 320 && poopix < 340)
          return(0);
         return(0xFF);
        }
        else
        {
         uint8 ret = SPRAM[PPU[3]];
         return(ret);
        }
}
static void ResetRL(uint8 *target) {
	memset(target, 0xFF, 512);
	if (InputScanlineHook)
		InputScanlineHook(0, 0, 0, 0);
	Plinef = target;
	Pline = target;
	firsttile = 0;
	linestartts = timestamp * 48 + X.count;
	tofix = 0;
	FCEUPPU_LineUpdate();
	tofix = 1;
}

static uint8 sprlinebuf[512 + 8];
static uint8 sprlinebuf2[512 + 8]; //mod: garbage?
void FCEUPPU_LineUpdate(void) {
#ifdef FCEUDEF_DEBUGGER
	if (!fceuindbg)
#endif
	if (Pline) {
		int l = GETLASTPIXEL;
		RefreshLine(l);
	}
}

static int tileview = 0;
static int rendis = 0;

void FCEUI_ToggleTileView(void) {
	tileview ^= 1;
}

void FCEUI_SetRenderDisable(int sprites, int bg) {
	if (sprites >= 0) {
		if (sprites == 2) rendis ^= 1;
		else rendis = (rendis & ~1) | sprites ? 1 : 0;
	}
	if (bg >= 0) {
		if (bg == 2) rendis ^= 2;
		else rendis = (rendis & ~2) | bg ? 2 : 0;
	}
}

static void CheckSpriteHit(int p);

static void EndRL(void) {
	RefreshLine(wss+16);
	if (tofix)
	{
		Fixit1();
		Fixit12();
	}
	CheckSpriteHit(wss+16);
	Pline = 0;
}

static int32 sphitx;
static uint8 sphitdata;

static void CheckSpriteHit(int p) {
	int l = p - 16;
	int x;

	if (sphitx == 0x100) return;

	for (x = sphitx; x < (sphitx + 8) && x < l; x++) {
		if ((sphitdata & (0x80 >> (x - sphitx))) && !(Plinef[x] & 128) && x < 255) {
			PPU_status |= 0x40;
			sphitx = 0x100;
			break;
		}
	}
}

/* spork the world.  Any sprites on this line? Then this will be set to 1.
 * Needed for zapper emulation and *gasp* sprite emulation.
 */
static int spork = 0;

/* lasttile is really "second to last tile." */
static void FASTAPASS(1) RefreshLine(int lastpixel) {
	static uint32 pshift[4];
	static uint32 pshift2[4];
	static uint32 atlatch;
	static uint32 atrib;
	static uint32 atlatch2;
	uint32 smorkus = RefreshAddr;
    uint32 smorkus2 = RefreshAddr2;
	#define RefreshAddr smorkus
	#define RefreshAddr2 smorkus2
	uint32 vofs;
	int X1;

	register uint8 *P = Pline;
	int lasttile = lastpixel >> 3;
	int numtiles;
	static int norecurse = 0;	/* Yeah, recursion would be bad.
								 * PPU_hook() functions can call
								 * mirroring/chr bank switching functions,
								 * which call FCEUPPU_LineUpdate, which call this
								 * function.
								 */
	if (norecurse) return;

	if (sphitx != 0x100 && !(PPU_status & 0x40)) {
		if ((sphitx < (lastpixel - 16)) && !(sphitx < ((lasttile - 2) * 8)))
			lasttile++;
	}

	if(wscre || (wss>256))
	{
	if (lasttile > 66) lasttile = 66; //mod: 256+ widht
	}
	else
	{
	if (lasttile > 34) lasttile = 34; 
	}
	numtiles = lasttile - firsttile;

	if (numtiles <= 0) return;

	P = Pline;

	vofs = 0;
	int vofs2 = 0;
	if(PEC586Hack)
		vofs = ((RefreshAddr & 0x200) << 3) | ((RefreshAddr >> 12) & 7);
	else
		vofs = ((PPU[0] & 0x10) << 8) | ((RefreshAddr >> 12) & 7);
	
	if(vt03_mode)vofs = ((PPU[0] & 0x10) << 11) | ((RefreshAddr >> 12) & 7);
	if(vt03_mode)vofs2 = ((0x10) << 11) | ((RefreshAddr2 >> 12) & 7);
	if (!ScreenON && !SpriteON) {
		uint32 tem;
		tem = Pal[0] | (Pal[0] << 8) | (Pal[0] << 16) | (Pal[0] << 24);
		tem |= 0x80808080;
		FCEU_dwmemset(Pline, tem, numtiles * 8);
		P += numtiles * 8;
		Pline = P;

		firsttile = lasttile;

		#define TOFIXNUM (wss+16 - 0x4)
		if (lastpixel >= TOFIXNUM && tofix) {
			Fixit1();
			Fixit12();
			tofix = 0;
		}

		if (InputScanlineHook && (lastpixel - 16) >= 0) {
			InputScanlineHook(Plinef, spork ? sprlinebuf : 0, linestartts, lasttile * 8 - 16);
		}
		return;
	}

	/* Priority bits, needed for sprite emulation. */
if(vt03_mode)
{
	PALRAM[0]    |= 128;
	PALRAM[0x10] |= 128;
	PALRAM[0x20] |= 128;
	PALRAM[0x30] |= 128;
}
else
{
	PALRAM[0]   |= 128;
	PALRAM[0x4] |= 128;
	PALRAM[0x8] |= 128;
	PALRAM[0xC] |= 128;
}

	/* This high-level graphics MMC5 emulation code was written for MMC5 carts in "CL" mode.
	 * It's probably not totally correct for carts in "SL" mode.
	 */

#define PPUT_MMC5
	if (MMC5Hack && geniestage != 1) {
		if (MMC5HackCHRMode == 0 && (MMC5HackSPMode & 0x80)) {
			int tochange = MMC5HackSPMode & 0x1F;
			tochange -= firsttile;
			for (X1 = firsttile; X1 < lasttile; X1++) {
				if ((tochange <= 0 && MMC5HackSPMode & 0x40) || (tochange > 0 && !(MMC5HackSPMode & 0x40))) {
					#define PPUT_MMC5SP
					#include "pputile.h"
					#undef PPUT_MMC5SP
				} else {
					#include "pputile.h"
				}
				tochange--;
			}
		} else if (MMC5HackCHRMode == 1 && (MMC5HackSPMode & 0x80)) {
			int tochange = MMC5HackSPMode & 0x1F;
			tochange -= firsttile;

			#define PPUT_MMC5SP
			#define PPUT_MMC5CHR1
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
			#undef PPUT_MMC5CHR1
			#undef PPUT_MMC5SP
		} else if (MMC5HackCHRMode == 1) {
			#define PPUT_MMC5CHR1
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
			#undef PPUT_MMC5CHR1
		} else {
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
		}
	}
	#undef PPUT_MMC5
	else if (PPU_hook) {
		norecurse = 1;
		#define PPUT_HOOK
		if (PEC586Hack) {
			#define PPU_BGFETCH
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
			#undef PPU_BGFETCH
		} else {
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
		}
		#undef PPUT_HOOK
		norecurse = 0;
	} else {
		if (PEC586Hack) {
			#define PPU_BGFETCH
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
			#undef PPU_BGFETCH
		} else {
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
		}
	}

#undef vofs
#undef RefreshAddr
#undef RefreshAddr2

	/* Reverse changes made before. */
	if(vt03_mode)
	{
	PALRAM[0]    &= 127;
	PALRAM[0x10] &= 127;
	PALRAM[0x20] &= 127;
	PALRAM[0x30] &= 127;
	}
	else
	{
	PALRAM[0]   &= 127;
	PALRAM[0x4] &= 127;
	PALRAM[0x8] &= 127;
	PALRAM[0xC] &= 127;
	}

	RefreshAddr = smorkus;
	RefreshAddr2 = smorkus2;
	if (firsttile <= 2 && 2 < lasttile && !(PPU[1] & 2)) {
		uint32 tem;
		tem = Pal[0] | (Pal[0] << 8) | (Pal[0] << 16) | (Pal[0] << 24);
		tem |= 0x80808080;
		*(uint32*)Plinef = *(uint32*)(Plinef + 4) = tem;
	}

	if (!ScreenON) {
		uint32 tem;
		int tstart, tcount;
		tem = Pal[0] | (Pal[0] << 8) | (Pal[0] << 16) | (Pal[0] << 24);
		tem |= 0x80808080;

		tcount = lasttile - firsttile;
		tstart = firsttile - 2;
		if (tstart < 0) {
			tcount += tstart;
			tstart = 0;
		}
		if (tcount > 0)
			FCEU_dwmemset(Plinef + tstart * 8, tem, tcount * 8);
	}

	if (lastpixel >= TOFIXNUM && tofix) {
		Fixit1();
		Fixit12();
		tofix = 0;
	}

	/* This only works right because of a hack earlier in this function. */
	CheckSpriteHit(lastpixel);

	if (InputScanlineHook && (lastpixel - 16) >= 0) {
		InputScanlineHook(Plinef, spork ? sprlinebuf : 0, linestartts, lasttile * 8 - 16);
	}
	Pline = P;
	firsttile = lasttile;
}

static INLINE void Fixit2(void) {
	if (ScreenON || SpriteON) {
		uint32 rad = RefreshAddr;
		rad &= 0xFBE0;
		rad |= TempAddr & 0x041f;
		RefreshAddr = rad;
	}
}
static INLINE void Fixit22(void) {
	if(!vt03_mode) return;
	if (ScreenON || SpriteON) {
		uint32 rad = RefreshAddr2;
		rad &= 0xFBE0;
		rad |= TempAddr2 & 0x041f;
		RefreshAddr2 = rad;
	}
}
static void Fixit1(void) {
	if(!wscre)
	{if (ScreenON || SpriteON) {
		uint32 rad = RefreshAddr;

		if ((rad & 0x7000) == 0x7000) {
			rad ^= 0x7000;
			if ((rad & 0x3E0) == 0x3A0)
				rad ^= 0xBA0;
			else if ((rad & 0x3E0) == 0x3e0)
				rad ^= 0x3e0;
			else
				rad += 0x20;
		} else
			rad += 0x1000;
		RefreshAddr = rad;
}
	}
else	if (ScreenON || SpriteON) {
		uint32 rad = RefreshAddr;

		if ((rad & 0x7000) == 0x7000) {
			rad ^= 0x7000;
			if ((rad & 0x3E0) == 0x3E0)
				rad ^= 0x3E0;
			else if ((rad & 0x3E0) == 0x3e0)
				rad ^= 0x3e0;
			else
				rad += 0x20;
		} else
			rad += 0x1000;
		RefreshAddr = rad;
	}
}
static void Fixit12(void) {
	if(!vt03_mode) return;
	if (ScreenON || SpriteON) {
		uint32 rad = RefreshAddr2;

		if ((rad & 0x7000) == 0x7000) {
			rad ^= 0x7000;
			if ((rad & 0x3E0) == 0x3A0)
				rad ^= 0xBA0;
			else if ((rad & 0x3E0) == 0x3e0)
				rad ^= 0x3e0;
			else
				rad += 0x20;
		} else
			rad += 0x1000;
		RefreshAddr2 = rad;
	}
}
void MMC5_hb(int);		/* Ugh ugh ugh. */
static void DoLine(void)
{
	

   int x;
   uint8 *target = NULL;
   if (scanline >= 240 && scanline != totalscanlines) //mod: todo: fix it with '240 overlocked line'
   {
		X6502_Run(256 + 69);
		scanline++;
		X6502_Run(16);
		return;
	}

	target = XBuf + ((scanline < 240 ? scanline : 240)*wss);

	if (MMC5Hack && (ScreenON || SpriteON)) MMC5_hb(scanline);

	X6502_Run(256);
	EndRL();

	if (rendis & 2) {/* User asked to not display background data. */
		uint32 tem;
		tem = Pal[0] | (Pal[0] << 8) | (Pal[0] << 16) | (Pal[0] << 24);
		tem |= 0x80808080;
		FCEU_dwmemset(target, tem, 512);
	}

	if (SpriteON)
		CopySprites(target);

	if (ScreenON || SpriteON) {	/* Yes, very el-cheapo. */
		if (PPU[1] & 0x01) {
			for (x = (wss/2-1); x >= 0; x--)
				*(uint32*)&target[x << 2] = (*(uint32*)&target[x << 2]) & 0x30303030;
		}
	}
	if ((PPU[1] >> 5) == 0x7) {
		for (x = (wss/2-1); x >= 0; x--)
			*(uint32*)&target[x << 2] = ((*(uint32*)&target[x << 2]) & 0x7f7f7f7f) | 0xc0c0c0c0;
	} else if (PPU[1] & 0xE0)
		for (x = (wss/2-1); x >= 0; x--)
			*(uint32*)&target[x << 2] = (*(uint32*)&target[x << 2]) | 0x80808080;
	else
		for (x = (wss/2-1); x >= 0; x--)
			*(uint32*)&target[x << 2] = ((*(uint32*)&target[x << 2]) & 0x7f7f7f7f) | 0x80808080;

	sphitx = 0x100;

	if (ScreenON || SpriteON)
		FetchSpriteData();

   X6502_Run(6);
   Fixit2();
   Fixit22();
	if (GameHBIRQHook && (ScreenON || SpriteON) && ((PPU[0] & 0x38) != 0x18)) {
		X6502_Run(4);
		GameHBIRQHook();
		X6502_Run(85 - 16 - 10);
	} else {

		X6502_Run(85 - 6 - 16);

		/* A semi-hack for Star Trek: 25th Anniversary */
		if (GameHBIRQHook && (ScreenON || SpriteON) && ((PPU[0] & 0x38) != 0x18))
			GameHBIRQHook();
	}

	if (SpriteON)
		RefreshSprites();
	if (GameHBIRQHook2 && (ScreenON || SpriteON))
		GameHBIRQHook2();
	scanline++;
	if (scanline < 240) {
		ResetRL(XBuf + (scanline*wss));
	}
	X6502_Run(16);
}

#define V_FLIP  0x80
#define H_FLIP  0x40
#define SP_BACK 0x20



void FCEUI_DisableSpriteLimitation(int a) {
	maxsprites = a ?  8: 256;
}

static uint8 numsprites, SpriteBlurp;
static void FetchSpriteData(void) {
	uint8 ns, sb;
	uint8 H;
	int vofs;
	uint8 P0 = PPU[0];
	H = 8;

	ns = sb = 0;

	vofs = (unsigned int)(P0 & 0x8 & (((P0 & 0x20) ^ 0x20) >> 2)) << 9;
if(vt03_mode)vofs = (unsigned int)(P0 & 0x8 & (((P0 & 0x20) ^ 0x20) >> 2)) << 12; //mod: if/else faster?
	H += (P0 & 0x20) >> 2;

	if (!sprites256) {
		for (int n = 0;n < 64;n++)
		{
			SPR *spr = (SPR *)&SPRAM[n * 4];

			if (n < 2)
			{
				spr = (SPR *)&SPRAM[(n * 4 + PPUSPL) & 0xFF];
			}

			if ((unsigned int)(scanline - spr->y) >= H) continue;

			if (ns < 64)
			{
				SPRB *dst = &SPRBUF[ns];

				if (n == 0)
					sb = 1;

				uint8 *C;
				int t;
				unsigned int vadr;

				t = ((int)scanline - (spr->y));

				if (Sprite16) {
					vadr = ((spr->no & 1) << 12) + ((spr->no & 0xFE) << 4);
					if (vt03_mode)
					{
						vadr = ((spr->no & 1) << 15) + ((spr->no & 0xFE) << 5);
					}
					if(vt03_mode)if((vadr<0x3fff)&&((spr->atr & 0x10) == 0x10))vadr += 0xA000;
					if((vadr>0x4000)&&((spr->atr & 0x10) == 0x10))vadr += 0x2000;
				}
				else
				{
					vadr = (spr->no << 4) + vofs;
					if(vt03_mode)vadr = (spr->no << 5) + vofs;
					if((vadr<0x4000)&&(spr->atr & 0x10)&&!vt03_mode)vadr += 0x6000; //mod: meh
					if((vadr<0x4000)&&(spr->atr & 0x10)&&vt03_mode)vadr += 0x8000;
					if((vadr<0x4000)&&(spr->atr & 0x10))vadr += 0x6000;
				}
				
				if (spr->atr&V_FLIP)
				{
					vadr += 7;
					vadr -= t;
					vadr += (P0 & 0x20) >> 1;
					vadr -= t & 8;
				}
				else
				{
					vadr += t;
					vadr += t & 8;
				}
				if (Sprite16&&(vadr&0x10&&vt03_mode))vadr += 0x10;
	 
				
				if (MMC5Hack && geniestage != 1 && Sprite16) C = MMC5SPRVRAMADR(vadr);
				else 
				{				
					if(vadr<0x4000)C = VRAMADR(vadr);
					else
					{
					if(vt03_mmc3_flag && vadr <0xA000) C = VRAMADR(vadr);
					else C = &extra_ppu[vadr];
					}
				}
			dst->ca[0] = C[0];

			if (PPU_hook && ns < 8)
			{
				PPU_hook(0x10000);
		             PPU_hook(vadr);
			}

		dst->ca[1] = C[8];
		if (PPU_hook && ns < 8) {
			PPU_hook(vadr | 8);
		}

		if(vt03_mode)
		{
		dst->ca[2] = C[16];

		dst->ca[3] = C[24];

		if (Sprite16)
		{
			
			dst->ca[4] = C[32];
			dst->ca[5] = C[40];
			dst->ca[6] = C[16];
			dst->ca[7] = C[24];
		}
		}
				dst->x = spr->x;
				dst->atr = spr->atr;

				ns++;
			}
			else
			{
				PPU_status |= 0x20;
				break;
			}
		}
	}
	if (sprites256) {
		for (int n = 0;n < 256;n++)
		{
				//mod: convert new oam to standard
			SPR spr = *(SPR *)SPRAM;
			spr.y = SPRAM[n];
			spr.no = SPRAM[n + 0x100];
			spr.atr = SPRAM[n + 0x200];
			spr.x = SPRAM[n + 0x300];


			if ((unsigned int)(scanline - spr.y) >= H) continue;

			if (ns<maxsprites)
			{
				SPRB *dst = &SPRBUF[ns];
				if (n == 0)
					sb = 1;

				uint8 *C;
				int t;
				unsigned int vadr;

				t = (int)scanline - (spr.y);

				if (Sprite16) {
					vadr = ((spr.no & 1) << 12) + ((spr.no & 0xFE) << 4);
					if (vt03_mode)
					{
						vadr = ((spr.no & 1) << 15) + ((spr.no & 0xFE) << 5);
					}
					if((vadr<0x4000)&&((spr.atr & 0x10) == 0x10))vadr += 0xA000;
					if((vadr>0x4000)&&((spr.atr & 0x10) == 0x10))vadr += 0x4000;
				}
				else
				{
					vadr = (spr.no << 4) + vofs;
					if (vt03_mode)vadr = (spr.no << 5) + vofs;
					if((vadr<0x4000)&&(spr.atr & 0x10)&&!vt03_mode)vadr += 0x6000;
					if((vadr<0x4000)&&(spr.atr & 0x10)&&vt03_mode)vadr += 0x8000;
					if((vadr>0x3fff)&&(spr.atr & 0x10))vadr += 0x2000;
					
				}
				
					
				if (spr.atr&V_FLIP)
				{
					vadr += 7;
					vadr -= t;
					vadr += (P0 & 0x20) >> 1;
					vadr -= t & 8;
				}
				else
				{
					vadr += t;
					vadr += t & 8;
				}
				if (Sprite16&&(vadr&0x10&&vt03_mode))vadr += 0x10;
				if (MMC5Hack && geniestage != 1 && Sprite16) C = MMC5SPRVRAMADR(vadr);
				else 
				{

				if(vadr<0x4000)C = VRAMADR(vadr);
				else
				{
				if(vt03_mmc3_flag && vadr <0xA000 && vt03_mode) C = VRAMADR(vadr);
				else 
					C = &extra_ppu[vadr];
				}
				if(!vt03_mmc3_flag && vt03_mode) C = &extra_ppu[vadr];
				}
				dst->ca[0] = C[0];

				if (PPU_hook && ns < 8)
				{
					PPU_hook(0x10000);
					PPU_hook(vadr);
				}

				dst->ca[1] = C[8];
				dst->ca[2] = C[16];
				dst->ca[3] = C[24];
				if (PPU_hook && ns<8)
					PPU_hook(vadr | 8);

				dst->x = spr.x;
				dst->atr = spr.atr;

				ns++;
			}
			else
			{
				PPU_status |= 0x20;
				break;
			}
		}
	}

	//if(ns>=7)
	//printf("%d %d\n",scanline,ns);
	if (ns>8) PPU_status |= 0x20;      /* Handle case when >8 sprites per
									   scanline option is enabled. */
	else if (PPU_hook)
	{
		for (int n = 0; n < (8 - ns); n++)
		{
			PPU_hook(0x10000);
			PPU_hook(vofs);
		}
	}
	numsprites = ns;
	SpriteBlurp = sb;
}

static void RefreshSprites(void) {
	int n;
	SPRB *spr;
int ex_pal;
	spork = 0;
	if (!numsprites) return;

	FCEU_dwmemset(sprlinebuf, 0x0, 512);
	numsprites--;
	spr = (SPRB*)SPRBUF + numsprites;

	for (n = numsprites; n >= 0; n--, spr--) {
		uint32 pixdata;
		uint32 pixdata2;
		uint8 J, atr;
		atr = spr->atr;
		int x = spr->x;
		uint8 *C;
		uint8 *C2;
		int VB;
	if(vt03_mode)	J = spr->ca[0] | spr->ca[1] | spr->ca[2] | spr->ca[3];
	else J = spr->ca[0] | spr->ca[1];
	
			for ( int z=0x20;  z<0x100; z++)
			{
			 if(PALRAM[z] !=0 && PALRAM[z] != PALRAM[0])ex_pal=1;
			}
			//mod: detect 3F20-3FFF write
		 if(!sprites256)ex_pal=0;
         if(ex_pal==1)VB = (0x10)+((atr&0xF)<<2);
		 else VB = (0x10)+((atr&0x3)<<2);
		 if(vt03_mode) VB = (0x40)+((atr&0xF)<<4); //mod: 16 colors palette
		 
		 
		pixdata = ppulut1[spr->ca[0]] | ppulut2[spr->ca[1]];
		if (vt03_mode)pixdata = ppulut1[spr->ca[0]] | ppulut2[spr->ca[1]] | ppulut4[spr->ca[2]] | ppulut5[spr->ca[3]];


		pixdata2 = pixdata;
		if (J) {
		if (n == 0 && SpriteBlurp && !(PPU_status & 0x40)) {
			sphitx = x;
			sphitdata = J;
			if (atr & H_FLIP)
				sphitdata = ((J << 7) & 0x80) |
							((J << 5) & 0x40) |
							((J << 3) & 0x20) |
							((J << 1) & 0x10) |
							((J >> 1) & 0x08) |
							((J >> 3) & 0x04) |
							((J >> 5) & 0x02) |
							((J >> 7) & 0x01);

		}
			if(!ex_pal && !spr_add_flag && (wss>256)) x += ((atr&0x08)<<5); //mod: read attribute for spr.x hi byte
            
			C = sprlinebuf + x;

		
			if(vt03_mode)
	
			{
			if (atr & SP_BACK) {
				if (atr & H_FLIP) {
					if (J & 0x80) C[7] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;									
					if (J & 0x40) C[6] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;									
					if (J & 0x20) C[5] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;									 
					if (J & 0x10) C[4] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;									 
					if (J & 0x08) C[3] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;									
					if (J & 0x04) C[2] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;									 
					if (J & 0x02) C[1] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;									
					if (J & 0x01) C[0] = READPAL(VB | pixdata)        & 0x7F | 0x80;

				} else {
					if (J & 0x80) C[0] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;
					if (J & 0x40) C[1] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;
					if (J & 0x20) C[2] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;
					if (J & 0x10) C[3] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;
					if (J & 0x08) C[4] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;
					if (J & 0x04) C[5] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;
					if (J & 0x02) C[6] = READPAL(VB | (pixdata & 0xF))& 0x7F | 0x80;
					pixdata >>= 4;
					if (J & 0x01) C[7] = READPAL(VB | pixdata)        & 0x7F | 0x80;
				}
			} else {
				if (atr & H_FLIP) {
					if (J & 0x80) C[7] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x40) C[6] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x20) C[5] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x10) C[4] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x08) C[3] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x04) C[2] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x02) C[1] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x01) C[0] = READPAL(VB | pixdata);

				} else {
					if (J & 0x80) C[0] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x40) C[1] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x20) C[2] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x10) C[3] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x08) C[4] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x04) C[5] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x02) C[6] = READPAL(VB | (pixdata & 0xF));
					pixdata >>= 4;
					if (J & 0x01) C[7] = READPAL(VB | pixdata);

				}
			}
			}
			else
			{
				if (atr & SP_BACK) {
				if (atr & H_FLIP) {
					if (J & 0x80) C[7] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x40) C[6] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x20) C[5] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x10) C[4] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x08) C[3] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x04) C[2] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x02) C[1] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x01) C[0] = READPAL(VB | pixdata) | 0x80;

				} else {
					if (J & 0x80) C[0] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x40) C[1] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x20) C[2] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x10) C[3] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x08) C[4] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x04) C[5] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x02) C[6] = READPAL(VB | (pixdata & 0xF)) | 0x80;
					pixdata >>= 4;
					if (J & 0x01) C[7] = READPAL(VB | pixdata) | 0x80;

				}
			} else {
				if (atr & H_FLIP) {
					if (J & 0x80) C[7] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x40) C[6] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x20) C[5] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x10) C[4] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x08) C[3] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x04) C[2] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x02) C[1] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x01) C[0] = READPAL(VB | pixdata);

				} else {
					if (J & 0x80) C[0] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x40) C[1] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x20) C[2] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x10) C[3] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x08) C[4] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x04) C[5] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x02) C[6] = READPAL(VB | (pixdata & 0x3));
					pixdata >>= 4;
					if (J & 0x01) C[7] = READPAL(VB | pixdata);

			}
				}
		}
	}
	}
	SpriteBlurp = 0;
	spork = 1;
}

static void CopySprites(uint8 *target) {
	uint16 n = ((PPU[1] & 4) ^ 4) << 1;
	uint8 *P = target;
	if (!spork) return;
	spork = 0;



 loopskie:
	{
		uint32 t = *(uint32*)(sprlinebuf + n);
		
		if ((t & 0x7F))
			if (!(t & 0x80) || (P[n] & 0x80))		// Normal sprite || behind bg sprite
				P[n] = sprlinebuf[n];
		if (((t>>8)&0x7F)) 
			if (!(t & 0x8000) || (P[n + 1] & 0x80))		// Normal sprite || behind bg sprite
				P[n + 1] = (sprlinebuf + 1)[n];

		if (((t>>16)&0x7F))// {
			if (!(t & 0x800000) || (P[n + 2] & 0x80))	// Normal sprite || behind bg sprite
				P[n + 2] = (sprlinebuf + 2)[n];

		if ((t>>24)&0x7F) 
			if (!(t & 0x80000000) || (P[n + 3] & 0x80))	// Normal sprite || behind bg sprite
				P[n + 3] = (sprlinebuf + 3)[n];
	}
	n += 4;
	if(n==wss) n=0;
	if (n) goto loopskie;

}

void FCEUPPU_SetVideoSystem(int w) {
	if (w) {
		scanlines_per_frame = dendy ? 262 : 312;
		FSettings.FirstSLine = FSettings.UsrFirstSLine[1];
		FSettings.LastSLine = FSettings.UsrLastSLine[1];
	} else {
		scanlines_per_frame = 262;
		FSettings.FirstSLine = FSettings.UsrFirstSLine[0];
		FSettings.LastSLine = FSettings.UsrLastSLine[0];
	}
}

void FCEUPPU_Init(void) {
	makeppulut();
}

void FCEUPPU_Reset(void) {
	VRAMBuffer = PPU[0] = PPU[1] = PPU_status = PPU[3] = 0;
	for(int x = 0; x<0x10000; x++)extra_ppu[x] = 0;

	PPUSPL = 0;
	PPUGenLatch = 0;
	RefreshAddr = TempAddr = 0;
	RefreshAddr2 = TempAddr2 = 32;
	vtoggle = vtoggle2 = 0;
	ppudead = 2;
	kook = 0;
	okk		= 0;
	colrnum	= 0;
	colr	= 0;
	colg	= 0;
	colb	= 0;
	 dmatgl    = 0;
	 dmaread   = 0;
	 dmawrite  = 0;
	 dmalenght = 0;
	
	
}

void FCEUPPU_Power(void) {
	int x;
	if(wscre) wss = 512; //mod: hmmm, can be change in future
	memset(NTARAM, 0x00, 0x800);
	memset(NTARAM2, 0x00, 0x800);
	memset(PALRAM, 0x00, 0x100);
	memset(UPALRAM, 0x00, 0x03);
	memset(SPRAM, 0x00, 0x400);
	FCEUPPU_Reset();

	for (x = 0x2000; x < 0x3000; x += 8) {
		ARead[x] = A200x;
		BWrite[x] = B2000;
		ARead[x + 1] = A200x;
		BWrite[x + 1] = B2001;
		ARead[x + 2] = A2002;
		BWrite[x + 2] = B2002;
		ARead[x + 3] = A200x;
		BWrite[x + 3] = B2003;
		ARead[x + 4] = A2004;
		BWrite[x + 4] = B2004;
		ARead[x + 5] = A200x;
		BWrite[x + 5] = B2005;
		ARead[x + 6] = A200x;
		BWrite[x + 6] = B2006;
		ARead[x + 7] = A2007;
		BWrite[x + 7] = B2007;
	}
	if(!sprites256)for (x = 0x2000; x < 0x4000; x += 8) {
		ARead[x] = A200x;
		BWrite[x] = B2000;
		ARead[x + 1] = A200x;
		BWrite[x + 1] = B2001;
		ARead[x + 2] = A2002;
		BWrite[x + 2] = B2002;
		ARead[x + 3] = A200x;
		BWrite[x + 3] = B2003;
		ARead[x + 4] = A2004;
		BWrite[x + 4] = B2004;
		ARead[x + 5] = A200x;
		BWrite[x + 5] = B12005;
		ARead[x + 6] = A200x;
		BWrite[x + 6] = B12006;
		ARead[x + 7] = A12007;
		BWrite[x + 7] = B12007;
	}
	BWrite[0x4014] = B4014;
	if(sprites256)   //mod: some codemaster games use 401C? hmm
	{
	
	BWrite[0x4018] = B4018;
	BWrite[0x4019] = B4019;
	BWrite[0x401B] = B401B;
	BWrite[0x401C] = B401C;
	BWrite[0x401D] = B401D;
	BWrite[0x4021] = B4021;
	BWrite[0x3000] = B3000;
	BWrite[0x3005] = B3005;
	BWrite[0x3006] = B3006;
	BWrite[0x3007] = B3007;
	}
}


int FCEUPPU_Loop(int skip) {
	/* Needed for Knight Rider, possibly others. */
	if (ppudead) {
		memset(XBuf, 0x80, 512 * 240);
		X6502_Run(scanlines_per_frame * (256 + 85));
		ppudead--;
	} else {
		X6502_Run(256 + 85);
		PPU_status |= 0x80;

		/* Not sure if this is correct.  According to Matt Conte and my own tests, it is.
		 * Timing is probably off, though.
		 * NOTE:  Not having this here breaks a Super Donkey Kong game.
		 */
		PPU[3] = PPUSPL = 0;

		/* I need to figure out the true nature and length of this delay. */
		X6502_Run(12);
		if (GameInfo->type == GIT_NSF)
			DoNSFFrame();
		else {
			if (VBlankON)
				TriggerNMI();
		}
		X6502_Run((scanlines_per_frame - 242) * (256 + 85) - 12);
		if (vblines) {
			if (!DMC_7bit || !skip_7bit_overclocking) {
				overclocked = 1;
				X6502_Run(vblines * (256 + 85) - 12);
				overclocked = 0;
			}
		}
		else if(vblankscanlines)
		{
						if (!DMC_7bit || !skip_7bit_overclocking) {
				overclocked = 1;
				X6502_Run(vblines * (256 + 85) - 12);
				overclocked = 0;
						}
		}
		PPU_status &= 0x1f;
		X6502_Run(256);

		{
			int x;

			if (ScreenON || SpriteON) {
				if (GameHBIRQHook && ((PPU[0] & 0x38) != 0x18))
					GameHBIRQHook();
				if (PPU_hook)
					for (x = 0; x < 42; x++) {
						PPU_hook(0x10000); PPU_hook(0);
					}
				if (GameHBIRQHook2)
					GameHBIRQHook2();
			}
			X6502_Run(85 - 16);
			if (ScreenON || SpriteON) {
				RefreshAddr = TempAddr;
				if (PPU_hook) PPU_hook(RefreshAddr & 0xffff);
				RefreshAddr2 = TempAddr2;
				if (PPU_hook) PPU_hook(RefreshAddr2 & 0xffff);
			}

			/* Clean this stuff up later. */
			spork = numsprites = 0;
			ResetRL(XBuf);

			X6502_Run(16 - kook);
			kook ^= 1;
		}
		if (GameInfo->type == GIT_NSF)
			X6502_Run((256 + 85) * normal_scanlines);
		#ifdef FRAMESKIP
		else if (skip) {
			int y;

			y = SPRAM[0];
			y++;

			PPU_status |= 0x20;	/* Fixes "Bee 52".  Does it break anything? */
			if (GameHBIRQHook) {
				X6502_Run(256);
				for (scanline = 0; scanline < 240; scanline++) {
					if (ScreenON || SpriteON)
						GameHBIRQHook();
					if (scanline == y && SpriteON) PPU_status |= 0x40;
					X6502_Run((scanline == 239) ? 85 : (256 + 85));
				}
			} else if (y < 240) {
				X6502_Run((256 + 85) * y);
				if (SpriteON) PPU_status |= 0x40;	/* Quick and very dirty hack. */
				X6502_Run((256 + 85) * (240 - y));
			} else
				X6502_Run((256 + 85) * 240);
		}
		#endif
		else {
			int x, max, maxref;

			deemp = PPU[1] >> 5;
			dmc7bfl = 0;
         /* manual samples can't play correctly with overclocking */
			if (DMC_7bit && skip_7bit_overclocking)
				totalscanlines = normal_scanlines;
			else
				totalscanlines = normal_scanlines + (overclock_state ? extrascanlines : 0);

			if (exscanlines)totalscanlines = 240 + exscanlines;
			for (scanline = 0; scanline < totalscanlines; ) {	/* scanline is incremented in  DoLine.  Evil. :/ */
				deempcnt[deemp]++;
				if ((PPUViewer) && (scanline == PPUViewScanline)) UpdatePPUView(1);
				DoLine();
				if (scanline < normal_scanlines || scanline == totalscanlines)
					overclocked = 0;
				else {

					overclocked = 1;
				}
			}

			DMC_7bit = 0;
			if (MMC5Hack && (ScreenON || SpriteON)) MMC5_hb(scanline);
			for (x = 1, max = 0, maxref = 0; x < 7; x++) {
				if (deempcnt[x] > max) {
					max = deempcnt[x];
					maxref = x;
				}
				deempcnt[x] = 0;
			}
			SetNESDeemph(maxref, 0);
		}
	}

	#ifdef FRAMESKIP
	if (skip) {
		FCEU_PutImageDummy();
		return(0);
	} else
	#endif
	{
		FCEU_PutImage();
		return(1);
	}
}

uint8 palm[128*3];
static uint16 TempAddrT, RefreshAddrT, TempAddrT2, RefreshAddrT2;

void FCEUPPU_LoadState(int version) {
		for(int i = 0; i<128; i++)
	{
		palo[i].r = palm[i*3];
		palo[i].g = palm[i*3+1];
		palo[i].b = palm[i*3+2];
	}
	WritePalette();
	TempAddr = TempAddrT;
	RefreshAddr = RefreshAddrT;
	TempAddr2 = TempAddrT2;
	RefreshAddr2 = RefreshAddrT2;
}

SFORMAT FCEUPPU_STATEINFO[] = {
	{ NTARAM, 0x800, "NTAR" },
	{ NTARAM2, 0x800, "NTR2" },
	{ PALRAM, 0x100, "PRAM" },
	{ palm, 128*3, "PALM" },
	{ SPRAM, 0x400, "SPRA" },
	{ extra_ppu, 0x10000, "EXPPU" },
	{ PPU, 0x4, "PPUR" },
	{ &kook, 1, "KOOK" },
	{ &stopclock, 1, "FLAGV" },
	{ &ppudead, 1, "DEAD" },
	{ &PPUSPL, 1, "PSPL" },
	{ &XOffset, 1, "XOFF" },
	{ &XOffset2, 1, "XOFFF" },
	{ &vtoggle, 1, "VTGL" },
	{ &vtoggle2, 1, "VTLL" },
	{ &RefreshAddrT, 2 | FCEUSTATE_RLSB, "RADD" },
	{ &TempAddrT, 2 | FCEUSTATE_RLSB, "TADD" },
	{ &RefreshAddrT2, 2 | FCEUSTATE_RLSB, "RADU" },
	{ &TempAddrT2, 2 | FCEUSTATE_RLSB, "TADU" },
	{ &VRAMBuffer, 1, "VBUF" },
	{ &PPUGenLatch, 1, "PGEN" },
	{ &okk, 1, "POKK" },
	{ &minus_line, 1, "MINL" },
	{ &colrnum, 1, "COLR" },
	{ &colr, 1, "CLRR" },
	{ &colg, 1, "CLRG" },
	{ &colb, 1, "CLRB" },
	{ &dmatgl, 1, "DTGL" },
	{ &dmaread, 1, "DRED" },
	{ &dmawrite, 1, "DWRT" },
	{ &dmalenght, 1, "DLNG" },
	{ 0 }
};


void FCEUPPU_SaveState(void) {
	for(int i = 0; i<128; i++)
	{
		palm[i*3] = palo[i].r;
		palm[i*3+1] = palo[i].g;
		palm[i*3+2] = palo[i].b;
	}
	TempAddrT = TempAddr;
	RefreshAddrT = RefreshAddr;
	TempAddrT2 = TempAddr2;
	RefreshAddrT2 = RefreshAddr2;
}