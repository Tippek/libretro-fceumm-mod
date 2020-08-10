	uint8 *S2 = PALRAM;
	uint32 pixdata2;
	uint32 lel2;
static uint32 attl;
uint8 *C2;
register uint8 cc2;
uint32 vadr2;

#ifndef PPUT_MMC5SP
	register uint8 zz2;
#else
	uint8 xs2, ys2;
	xs2 = X1;
	ys2 = ((scanline >> 3) + MMC5HackSPScroll) & 0x1F;
	if (ys2 >= 0x1E) ys2 -= 0x1E;
#endif

if (X1 >= 2) {


	pixdata2 = ppulut1[(pshift2[0] >> (8 - XOffset2)) & 0xFF] | ppulut2[(pshift2[1] >> (8 - XOffset2)) & 0xFF];
if(vt03_mode)pixdata2 = ppulut1[(pshift2[0] >> (8 - XOffset2)) & 0xFF] | ppulut2[(pshift2[1] >> (8 - XOffset2)) & 0xFF] | ppulut4[(pshift2[2] >> (8 - XOffset2)) & 0xFF] | ppulut5[(pshift2[3] >> (8 - XOffset2)) & 0xFF];
lel2 = pixdata2;
lel2 = ppulut3[XOffset2 | (atlatch2 << 3)];
if(!wscre) 	  lel2 = ppulut3[XOffset | (atlatch2 << 3)];
if(wscre)  lel2 = ppulut6[XOffset| (atlatch2 << 3)];
}

uint8 *C;
register uint8 cc;
uint16 vadr;

#ifndef PPUT_MMC5SP
	register uint8 zz;
#else
	uint8 xs, ys;
	xs = X1;
	ys = ((scanline >> 3) + MMC5HackSPScroll) & 0x1F;
	if (ys >= 0x1E) ys -= 0x1E;
#endif

if (X1 >= 2) {
	uint8 *S = PALRAM;
	uint32 pixdata;
	uint32 lel;

	pixdata = ppulut1[(pshift[0] >> (8 - XOffset)) & 0xFF] | ppulut2[(pshift[1] >> (8 - XOffset)) & 0xFF];
if(vt03_mode)
{
pixdata = ppulut1[(pshift[0] >> (8 - XOffset)) & 0xFF] | ppulut2[(pshift[1] >> (8 - XOffset)) & 0xFF] | ppulut4[(pshift[2] >> (8 - XOffset)) & 0xFF] | ppulut5[(pshift[3] >> (8 - XOffset)) & 0xFF];
lel = ppulut3[XOffset | (atlatch << 3)];

if(!wscre) 	  lel = ppulut3[XOffset | (atlatch << 3)];
if(wscre)  lel = ppulut6[XOffset| (atlatch << 3)];
}
	for(int x =(X1*8-16); x<(X1*8-16+8); x++)
{
	
	priora_bg[x] = 0;
}
if(vt03_mode&&pixdata)
{
//pixdata |= ppulut3[XOffset];
	if(pixdata&0xF)
	{
		P[0] = S[pixdata & 0xF | (((lel>>2)&3)<<4)];
		priora_bg[X1*8-16+0] = 1;
	}
		else
		{			
		P[0] = S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
		
		}
	pixdata >>= 4;
	lel >>=4;	
	pixdata2 >>= 4;
	lel2 >>=4;	
	if(pixdata&0xF)
	{
		P[1] =  S[pixdata & 0xF | (((lel>>2)&3)<<4)];
		priora_bg[X1*8-16+1] = 1;
	}
		else 
		{
			P[1] = S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
			
		}
	pixdata >>= 4;		
	lel >>=4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(pixdata&0xF)
	{
		P[2] =  S[pixdata & 0xF | (((lel>>2)&3)<<4)];
		priora_bg[X1*8-16+2] = 1;
	}
	else 
	{
		
		P[2] = S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	}
	pixdata >>= 4;		
	lel >>=4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(pixdata&0xF)
	{
		P[3] =  S[pixdata & 0xF | (((lel>>2)&3)<<4)];
		priora_bg[X1*8-16+3] = 1;
	}
	else
	{
		
		P[3] = S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	}
	pixdata >>= 4;		
	lel >>=4;
	pixdata2 >>= 4;
	lel2 >>=4;
if(pixdata&0xF)
	{
		P[4] =  S[pixdata & 0xF | (((lel>>2)&3)<<4)];
		priora_bg[X1*8-16+4] = 1;
	}
	else 
	{
		P[4] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];	
		
	}
	pixdata >>= 4;		
	lel >>=4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(pixdata&0xF)
	{
		priora_bg[X1*8-16+5] = 1;
		P[5] =  S[pixdata & 0xF | (((lel>>2)&3)<<4)];
	}else 
	{
		P[5] = S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];	
		
	}
	pixdata >>= 4;		
	lel >>=4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(pixdata&0xF)
	{
		priora_bg[X1*8-16+6] = 1;
		P[6] =  S[pixdata & 0xF | (((lel>>2)&3)<<4)];
	}		else 
	{
		
		P[6] = S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	}
	pixdata >>= 4;		
	lel >>=4;
	pixdata2 >>= 4;
	lel2 >>=4;
if(pixdata&0xF)
	{
		priora_bg[X1*8-16+7] = 1;
		P[7] =  S[pixdata & 0xF | (((lel>>2)&3)<<4)];	
	}else 
	{
		
		P[7] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	}
	}
	else
	{
	if(vt03_mode)
	{
	//pixdata = 0;
	if(!pixdata2)P[0] = S[0];else P[0] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	pixdata >>= 4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(!pixdata2)P[1] = S[0];else P[1] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	pixdata >>= 4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(!pixdata2)P[2] = S[0];else P[2] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	pixdata >>= 4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(!pixdata2)P[3] = S[0];else P[3] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	pixdata >>= 4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(!pixdata2)P[4] = S[0];else P[4] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	pixdata >>= 4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(!pixdata2)P[5] = S[0];else P[5] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	pixdata >>= 4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(!pixdata2)P[6] = S[0];else P[6] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	pixdata >>= 4;
	pixdata2 >>= 4;
	lel2 >>=4;
	if(!pixdata2)P[7] = S[0];else P[7] =  S2[pixdata2 & 0xF | (((lel2>>2)&3)<<4)];
	
	
	}
	else
	{ 
	if(!wscre) 	  pixdata |= ppulut3[XOffset | (atlatch << 3)];
	uint32 pixdat =0;// ppulut3[XOffset| (atlatch << 3)];
if(wscre)  pixdat = ppulut6[XOffset| (atlatch << 3)];
if (atrib & 0x20) {
				if (atrib & 0x40) {
						P[7] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+7] = 1;
					pixdata >>= 4;
					P[6] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+6] = 1;
					pixdata >>= 4;
					P[5] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+5] = 1;
					pixdata >>= 4;
					P[4] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+4] = 1;
					pixdata >>= 4;
					P[3] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+3] = 1;
					pixdata >>= 4;
					P[2] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+2] = 1;
					pixdata >>= 4;
					P[1] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+1] = 1;
					pixdata >>= 4;
					P[0] = S[pixdata];
					if((pixdata & 0xF))  priora_bg[X1*8-16+0] = 1;

					

				} else {
					P[0] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+0] = 1;
					pixdata >>= 4;         
					P[1] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+1] = 1;
					pixdata >>= 4;         
					P[2] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+2] = 1;
					pixdata >>= 4;         
					P[3] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+3] = 1;
					pixdata >>= 4;         
					P[4] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+4] = 1;
					pixdata >>= 4;         
					P[5] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+5] = 1;
					pixdata >>= 4;         
					P[6] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+6] = 1;
					pixdata >>= 4;         
					P[7] = S[pixdata];
					if((pixdata & 0xF))  priora_bg[X1*8-16+7] = 1;

				}
			} else {
				if (atrib & 0x40) {
					P[7] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+7] = 1;
					pixdata >>= 4;
					P[6] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+6] = 1;
					pixdata >>= 4;
					P[5] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+5] = 1;
					pixdata >>= 4;
					P[4] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+4] = 1;
					pixdata >>= 4;
					P[3] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+3] = 1;
					pixdata >>= 4;
					P[2] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+2] = 1;
					pixdata >>= 4;
					P[1] = S[pixdata & 0xF];
					if((pixdata & 0xF))  priora_bg[X1*8-16+1] = 1;
					pixdata >>= 4;
					P[0] = S[pixdata];
					if((pixdata & 0xF))  priora_bg[X1*8-16+0] = 1;

				} else {
					P[0] = S[(pixdata & 0xF) | (((pixdat)&0xF)<<2)];
					if(((pixdata & 0x3) |(((pixdat)&0x3)<<2)) )  priora_bg[X1*8-16+0] = 1;
					pixdata >>= 4;
					pixdat >>=4; 
					P[1] = S[(pixdata & 0xF) | (((pixdat)&0xF)<<2)];
					if(((pixdata & 0x3) |(((pixdat)&0x3)<<2)) )   priora_bg[X1*8-16+1] = 1;
					pixdata >>= 4;
					pixdat >>= 4;
					P[2] = S[(pixdata & 0xF) | (((pixdat)&0xF)<<2)];
					if(((pixdata & 0x3) |(((pixdat)&0x3)<<2)) )   priora_bg[X1*8-16+2] = 1;
					pixdata >>= 4;
					pixdat >>= 4;
					P[3] = S[(pixdata & 0xF) | (((pixdat)&0xF)<<2)];
					if(((pixdata & 0x3) |(((pixdat)&0x3)<<2)) )  priora_bg[X1*8-16+3] = 1;
					pixdata >>= 4;
					pixdat >>= 4;
					P[4] = S[(pixdata & 0xF) | (((pixdat)&0xF)<<2)];
					if(((pixdata & 0x3) |(((pixdat)&0x3)<<2)) )   priora_bg[X1*8-16+4] = 1;
					pixdata >>= 4;
					pixdat >>= 4;
					P[5] = S[(pixdata & 0xF) | (((pixdat)&0xF)<<2)];
					if(((pixdata & 0x3) |(((pixdat)&0x3)<<2)) )  priora_bg[X1*8-16+5] = 1;
					pixdata >>= 4;
					pixdat >>= 4;
					P[6] = S[(pixdata & 0xF) | (((pixdat)&0xF)<<2)];
					if(((pixdata & 0x3) |(((pixdat)&0x3)<<2)) )   priora_bg[X1*8-16+6] = 1;
					pixdata >>= 4;
					pixdat >>= 4;
					P[7] = S[ (pixdata ) | (((pixdat)&0xF)<<2)];
					if(((pixdata & 0x3) |(((pixdat)&0x3)<<2)) )  priora_bg[X1*8-16+7] = 1;
			}	
				}
	
	}

}
}
if (X1 >= 2)
	P +=8;
if(vt03_mode)
{
#ifdef PPUT_MMC5SP
	vadr2 = (MMC5HackExNTARAMPtr[xs2 | (ys2 << 5)] << 4);
#else
	zz2 = RefreshAddr2 & 0x1F;
	C2 = vnapage2[(RefreshAddr2 >> 10) & 3];
	vadr2 = (C2[RefreshAddr2 & 0x3ff] << 4) + vofs2;	// Fetch name table byte.
	if(vt03_mode)vadr2 = (C2[RefreshAddr2 & 0x3ff] << 5) + vofs2;	// Fetch name table byte.
#endif

#ifdef PPUT_HOOK
	PPU_hook(0x2000 | (RefreshAddr2 & 0xfff));
#endif

#ifdef PPUT_MMC5SP
	cc2 = MMC5HackExNTARAMPtr[0x3c0 + (xs2 >> 2) + ((ys2 & 0x1C) << 1)];
	cc2 = ((cc2 >> ((xs2 & 2) + ((ys2 & 0x2) << 1))) & 3);
#else
	#ifdef PPUT_MMC5CHR1
		cc2 = (MMC5HackExNTARAMPtr[RefreshAddr2 & 0x3ff] & 0xC0) >> 6;
	#else
		cc2 = C2[0x3c0 + (zz2 >> 2) + ((RefreshAddr2 & 0x380) >> 4)];	// Fetch attribute table byte.
		cc2 = ((cc2 >> ((zz2 & 2) + ((RefreshAddr2 & 0x40) >> 4))) & 3);
		if(wscre)cc2 = NTATRIB2[RefreshAddr2&0x7FF]&0x0F;
	#endif
#endif
}
if(wscre)
{
atlatch2 >>= 4;
atlatch2 |= cc2 << 4;
}
else
{
atlatch2 >>= 2;
atlatch2 |= cc2 << 2;
}

	if(vt03_mode)
	{
pshift2[0] <<= 8;
pshift2[1] <<= 8;
pshift2[2] <<= 8;
pshift2[3] <<= 8;

#ifdef PPUT_MMC5SP
	C2 = MMC5HackVROMPTR + vadr2;
	C2 += ((MMC5HackSPPage & 0x3f & MMC5HackVROMMask) << 12);
#else
	#ifdef PPUT_MMC5CHR1
		C2 = MMC5HackVROMPTR;
		C2 += (((MMC5HackExNTARAMPtr[RefreshAddr2 & 0x3ff]) & 0x3f & MMC5HackVROMMask) << 12) + (vadr2 & 0xfff);
		C2 += (MMC50x5130 & 0x3) << 18; //11-jun-2009 for kuja_killer
	#elif defined(PPUT_MMC5)
		C2 = MMC5BGVRAMADR(vadr2);
	#else
		//if(vadr2<0x4000)C2 = (uint8*)VRAMADR(vadr2);
		C2 = &extra_ppu[vadr2+0x6000];
	#endif
#endif

#ifdef PPUT_HOOK
	PPU_hook(vadr2);
#endif

#ifdef PPU_BGFETCH
	if (RefreshAddr2 & 1) {
	
		pshift2[0] |= C2[8];
		pshift2[1] |= C2[8];
	//	pshift2[2] |= C2[16];
	//	pshift2[3] |= C2[24];
	} else {
	
		pshift2[0] |= C2[0];
		pshift2[1] |= C2[0];
	//	pshift2[2] |= C2[16];
	//	pshift2[3] |= C2[24];
	}
#else
	
	pshift2[0] |= C2[0];
	pshift2[2] |= C2[16];

	pshift2[1] |= C2[8];
	pshift2[3] |= C2[24];
#endif
}

//#ifdef PPUT_HOOK
//	PPU_hook(0x2000 | (RefreshAddr2 & 0xfff));
//#endif
#ifdef PPUT_MMC5SP
	vadr = (MMC5HackExNTARAMPtr[xs | (ys << 5)] << 4) + (vofs & 7);
#else
	zz = RefreshAddr & 0x1F;
	C = vnapage[(RefreshAddr >> 10) & 3];
	vadr = (C[RefreshAddr & 0x3ff] << 4) + vofs;	// Fetch name table byte.
	if(vt03_mode)vadr = (C[RefreshAddr & 0x3ff] << 5) + vofs;	// Fetch name table byte.
	if(wscre&&(NTATRIB[RefreshAddr&0x7FF]&0x10))vadr = (C[RefreshAddr & 0x3ff] << 5) + 0xC000;	// Fetch name table byte.
#endif

#ifdef PPUT_HOOK
	PPU_hook(0x2000 | (RefreshAddr & 0xfff));
#endif

#ifdef PPUT_MMC5SP
	cc = MMC5HackExNTARAMPtr[0x3c0 + (xs >> 2) + ((ys & 0x1C) << 1)];
	cc = ((cc >> ((xs & 2) + ((ys & 0x2) << 1))) & 3);
#else
	#ifdef PPUT_MMC5CHR1
		cc = (MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff] & 0xC0) >> 6;
	#else
		cc = C[0x3c0 + (zz >> 2) + ((RefreshAddr & 0x380) >> 4)];	// Fetch attribute table byte.
		cc = ((cc >> ((zz & 2) + ((RefreshAddr & 0x40) >> 4))) & 3);
		if(wscre)cc = NTATRIB[RefreshAddr&0x7FF]&0x0F;
	#endif
#endif
if(wscre)
{
atlatch >>= 4;
atlatch |= cc << 4;
}
else
{
atlatch >>= 2;
atlatch |= cc << 2;
}
atrib = NTATRIB[RefreshAddr&0x7FF];
pshift[0] <<= 8;
pshift[1] <<= 8;
pshift[2] <<= 8;
pshift[3] <<= 8;

#ifdef PPUT_MMC5SP
	C = MMC5HackVROMPTR + vadr;
	C += ((MMC5HackSPPage & 0x3f & MMC5HackVROMMask) << 12);
#else
	#ifdef PPUT_MMC5CHR1
		C = MMC5HackVROMPTR;
		C += (((MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff]) & 0x3f & MMC5HackVROMMask) << 12) + (vadr & 0xfff);
		C += (MMC50x5130 & 0x3) << 18; //11-jun-2009 for kuja_killer
	#elif defined(PPUT_MMC5)
		C = MMC5BGVRAMADR(vadr);
	#else
		
				if(vadr<0x4000)
				{
						if(chrambank_V[vadr>>8])
						C = &chrramm[(chrambank_V[vadr>>8]<<8)|(vadr&0xFF)];
							else
				C = VRAMADR(vadr);
				}
				else
				{
				if(vt03_mmc3_flag && vadr <0xA000) C = VRAMADR(vadr);
				else C = &extra_ppu[vadr];
				}
	#endif
#endif

#ifdef PPUT_HOOK
	PPU_hook(vadr);
#endif

#ifdef PPU_BGFETCH
	if (RefreshAddr & 1) {
		
		pshift[0] |= C[8];
		pshift[1] |= C[8];
		pshift[2] |= C[16];
		pshift[3] |= C[24];
	} else {
		
		pshift[0] |= C[0];
		pshift[1] |= C[0];
		pshift[2] |= C[16];
		pshift[3] |= C[24];
	}
#else

	pshift[0] |= C[0];
	pshift[2] |= C[16];
	
	pshift[1] |= C[8];
	pshift[3] |= C[24];
#endif

if ((RefreshAddr & 0x1f) == 0x1f)
	RefreshAddr ^= 0x41F;
else
	RefreshAddr++;

if ((RefreshAddr2 & 0x1f) == 0x1f)
	RefreshAddr2 ^= 0x41F;
else
	RefreshAddr2++;
#ifdef PPUT_HOOK

	PPU_hook(0x2000 | (RefreshAddr & 0xfff));
#endif

