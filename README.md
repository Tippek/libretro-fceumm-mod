

MODIFIED FCEUMM-CORE
Fork. Original: https://github.com/libretro/libretro-fceumm. Features from fceux-mod.
Feature	Description
Extra RAM	0x0000-0x2000
256 sprites on screen	reworked OAM, spr_y, spr_y,….,spr_no, spr_no, 0x400 bytes, easy for romhackers
Post-render overclock	Native with additional dmc fix, but fix can be disable
Pre-render overclock	Native
Stop increase timings	$401C register, #$00 — stop, otherwise — run; 
Extra PPURAM	0x8000 — 0x9FFF for extra sprites(see below) or second page for VT03 gfx(instead 0x1000-0x1fff);                           0xA000 - 0xBFFF for extra sprites with VT03 gfx;          0xC000 — 0xDFFF for extra sprites 16x8 with VT03 gfx;
CPU overclock	All opcodes take only 1 cycle, for debugging in fact
New Register	Z, similar X and Y
New opcodes	TAZ 0x02 TZA 0x12 PLZ 0x7A PHZ 0x80 INZ 0xFA DEZ 0xFB LDZ 0xA3 imm LDZ 0xA7 indX STZ 0x82 indX DMA 0xF2 imm — xA DVA 0xF3 imm - /A LDZ 0x22 abs STZ 0x92 abs LDZ 0xAB zpX STZ 0x83 zpX STA 0x87 zpZ STX 0x89 zpZ STY 0x8B zpZ LDA 0xB7 zpZ LDX 0xBF zpZ LDY 0x42 zpZ STZ 0x97 absY LDZ 0xB2 absX STZ 0x93 absX STZ 0x8F zpY LDZ 0xAF zpY DEC 0xCF abz ORA 0xDF abz AND 0xDB abz ADC 0xC3 abz SBC 0xD3 abz INC 0xEF abz LDX 0x32 abz LDY 0x52 abz LDA 0xBB abz STX 0x9C abz STY 0x9E abz STA 0x9B abz CPZ 0x07 imm BRA 0x0B rel DAV 0x0C imm - %A INX 0x0F imm INY 0x04 imm LDZ 0xB3 absY
Extra Palettes for sprites	0x3F00-0x3FFF, using 0xC bit in sprite attribute
Extra Sprites	Located in additional PPURAM, using 0x10 bit(512 sprites)
MP3, Ogg player	not presents in fceux, because there is no reason for extra DLL files, presents in mednafen
Second NT	$3000, $3005-$3007, use only with VT03 gfx
128 colors in palette(instead 64)	Only VT03 gfx
VT03 gfx	Only mappers without chr banks/mmc3 and simple mappers, based on VT03 specs
Change RGB color in palette	$401B registers, 4 bytes: No, R. G. B
12 players	only fami4 extension
More than 256 chr banks(mmc3)	-
DMA operations	$4018 register, 7 bytes: 1 — low read address, 2 — hi read address, 3 — low write address, 4 — hi read address, 5 — low lenght, 6 — hi lenght, 7 — mode(0 — from ROM to RAM, 1 — from VRAM/VROM to VRAM, 2 — from ROM to VRAM)
Widescreen	 works only with Vertical Mirroring, using 0x08 bit sprite attribute OR special RAM place($4021 registers like a $4014)
64x32 NMT(Genesis like)	$2000-$27FF for tile number, $2800-$2FFF — tile attributes(16 palettes , unfinished)
DMC memory from SRAM, only AxROM	SRAM $5000-$7FFF, DMC starts from $5000 instead $C000

# FCE Ultra mappers modified
FCEU "mappers modified" is an unofficial build of FCEU Ultra by CaH4e3, which supports a lot of new mappers including some obscure mappers such as one for unlicensed NES ROM's.
