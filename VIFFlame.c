// Licensed under the MIT License
// Ty (Fobes) Lamontagne 2023

// A Flame animation using textures send through PATH2
// and with not that much dependence on the SDK

#include "flame_clut.h"
#include "flame_tex.h"

#include <stdio.h>
#include <stdlib.h>

#include <kernel.h>
#include <graph.h>
#include <dma.h>
#include <dma_tags.h>
#include <vif_codes.h>
#include <gif_tags.h>
#include <gif_registers.h>
#include <gs_gp.h>
#include <gs_psm.h>

u32 g_fbptr;
u32 g_zbptr;
void init_gs()
{
	// Allocate the framebuffer and zbuffer
	g_fbptr = graph_vram_allocate(640, 448, GS_PSM_24, GRAPH_ALIGN_PAGE);
	g_zbptr = graph_vram_allocate(640, 448, GS_PSMZ_16, GRAPH_ALIGN_PAGE);

	// Rely on graph to set up privileged registers
	graph_initialize(g_fbptr, 640, 448, GS_PSM_24, 0, 0);

	// Set up the GS registers with our desired draw context
	// You could use libdraw to do this

	qword_t *draw_context_pk = aligned_alloc(16, sizeof(qword_t) * 100);
	qword_t *q = draw_context_pk;

	PACK_GIFTAG(q, GIF_SET_TAG(14, 1, GIF_PRE_DISABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1),
				GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_FRAME(g_fbptr >> 11, 640 >> 6, GS_PSM_24, 0), GS_REG_FRAME);
	q++;
	PACK_GIFTAG(q, GS_SET_TEST(1, 7, 0, 2, 0, 0, 1, 1), GS_REG_TEST);
	q++;
	PACK_GIFTAG(q, GS_SET_PABE(0), GS_REG_PABE);
	q++;
	PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 0, 1, 128), GS_REG_ALPHA);
	q++;
	PACK_GIFTAG(q, GS_SET_ZBUF(g_zbptr >> 11, GS_PSMZ_16, 0), GS_REG_ZBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET);
	q++;
	PACK_GIFTAG(q, GS_SET_DTHE(0), GS_REG_DTHE);
	q++;
	PACK_GIFTAG(q, GS_SET_PRMODECONT(1), GS_REG_PRMODECONT);
	q++;
	PACK_GIFTAG(q, GS_SET_SCISSOR(0, 640 - 1, 0, 448 - 1), GS_REG_SCISSOR);
	q++;
	PACK_GIFTAG(q, GS_SET_CLAMP(0, 0, 0, 0, 0, 0), GS_REG_CLAMP);
	q++;
	PACK_GIFTAG(q, GS_SET_SCANMSK(0), GS_REG_SCANMSK);
	q++;
	PACK_GIFTAG(q, GS_SET_TEXA(0x00, 0, 0x7F), GS_REG_TEXA);
	q++;
	PACK_GIFTAG(q, GS_SET_FBA(0), GS_REG_FBA);
	q++;
	PACK_GIFTAG(q, GS_SET_COLCLAMP(GS_ENABLE), GS_REG_COLCLAMP);
	q++;

	dma_channel_send_normal(DMA_CHANNEL_GIF, draw_context_pk, q - draw_context_pk, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	free(draw_context_pk);
}

u32 g_clutptr = 0;
u32 g_texptr = 0;
void upload_texture(s32 include_itex, s32 itex_index)
{
	qword_t *vif_packet = aligned_alloc(16, sizeof(qword_t) * 40);
	qword_t *q = vif_packet;
	DMATAG_CNT(q, 7, 0, 0, 0); // Tell the DMAC to send the next 7 QW to the VIF
	q++;
	PACK_VIFTAG(q, VIF_CMD_NOP, VIF_CMD_NOP, VIF_CMD_NOP, (VIF_CMD_DIRECT << 24) | 8); // Tell the VIF to send the next 8 QW to the GIF
	q++;
	PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_BITBLTBUF(0, 0, 0, g_clutptr >> 6, 1, GS_PSM_16), GS_REG_BITBLTBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXREG(16, 1), GS_REG_TRXREG);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXDIR(0), GS_REG_TRXDIR);
	q++;
	PACK_GIFTAG(q, GIF_SET_TAG(2, 0, 0, 0, GIF_FLG_IMAGE, 0), 0);
	q++;
	DMATAG_REF(q, 2, (uiptr)flame_clut, 0, 0, 0); // "attach" the clut data to this DMA chain
	q++;

	if (include_itex)
	{
		DMATAG_CNT(q, 7, 0, 0, 0); // Tell the DMAC to send the next 7 QW to the VIF
		q++;
		PACK_VIFTAG(q, VIF_CMD_NOP, VIF_CMD_NOP, VIF_CMD_NOP, (VIF_CMD_DIRECT << 24) | 38); // Tell the VIF to send the next 38 QW to the GIF
		q++;
		PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_BITBLTBUF(0, 0, 0, g_texptr >> 6, 1, GS_PSM_4), GS_REG_BITBLTBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXREG(32, 32), GS_REG_TRXREG);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXDIR(0), GS_REG_TRXDIR);
		q++;
		PACK_GIFTAG(q, GIF_SET_TAG(32, 0, 0, 0, GIF_FLG_IMAGE, 0), 0);
		q++;
		DMATAG_REF(q, 32, (uiptr)flame_textures[itex_index], 0, 0, 0); // "attach" the texture data to this DMA chain
		q++;
	}
	// I don't believe this is required, but I'll keep it here anyways
	// This just flushes the GS texture cache
	DMATAG_CNT(q, 5, 0, 0, 0); // Tell the DMAC to send the next 5 QW to the VIF
	q++;
	PACK_VIFTAG(q, VIF_CMD_NOP, VIF_CMD_NOP, VIF_CMD_NOP, (VIF_CMD_DIRECT << 24) | 2); // Tell the VIF to send the next 2 QW to the GIF
	q++;
	PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_TEXFLUSH(1), GS_REG_TEXFLUSH);
	q++;
	PACK_VIFTAG(q, VIF_CMD_NOP, VIF_CMD_NOP, VIF_CMD_NOP, VIF_CMD_NOP);
	q++;
	PACK_VIFTAG(q, VIF_CMD_NOP, VIF_CMD_NOP, VIF_CMD_NOP, VIF_CMD_NOP);
	q++;

	FlushCache(0);
	dma_channel_send_chain(DMA_CHANNEL_VIF1, vif_packet, q - vif_packet, 0, 0);
	dma_channel_wait(DMA_CHANNEL_VIF1, 0);
	free(vif_packet);
}

// Rotates the pallete from indices 1 to 6
void rotate_pallete()
{
	u16 *flameClut = (u16 *)UNCACHED_SEG(flame_clut);
	u16 temp = flameClut[1];
	for (s32 i = 1; i < 6; i++)
		flameClut[i] = flameClut[i + 1];
	flameClut[6] = temp;
}

s32 vsync_sema_id = 0;
s32 vsync_handler(s32 cause)
{
	iSignalSema(vsync_sema_id);
	ExitHandler();
	return 0;
}

int main(int argc, char *argv[])
{
	printf("VIFFlame Starting.\n");

	init_gs();

	// Allocate vram for the clut and texture
	// note: I'm not sure why allocating a 16 bit format for the clut doesn't work
	// I think it's an issue with graphs allocation strategy
	g_clutptr = graph_vram_allocate(16, 1, GS_PSM_32, GRAPH_ALIGN_BLOCK);
	g_texptr = graph_vram_allocate(32, 32, GS_PSM_4, GRAPH_ALIGN_BLOCK);
	ee_sema_t vsync_sema;
	vsync_sema.init_count = 0;
	vsync_sema.max_count = 1;
	vsync_sema.option = 0;
	vsync_sema.attr = 0;
	vsync_sema_id = CreateSema(&vsync_sema);

	AddIntcHandler(INTC_VBLANK_S, vsync_handler, 0);
	EnableIntc(INTC_VBLANK_S);

	upload_texture(1, 0);

	qword_t *draw_packet = aligned_alloc(16, sizeof(qword_t) * 10);

	u32 frame_count = 0;
	u32 itex_index = 0;
	do
	{
		qword_t *q = draw_packet;
		PACK_GIFTAG(q, GIF_SET_TAG(9, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1),
					GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_RGBAQ(0x11, 0x11, 0x00, 0x7f, 0x00), GS_REG_RGBAQ);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0 << 4, 0 << 4, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(640 << 4, 448 << 4, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0), GS_REG_PRIM);
		q++;
		PACK_GIFTAG(q, GS_SET_TEX0(g_texptr / 64, 1, GS_PSM_4, 5, 5, 1, 1, g_clutptr / 64, GS_PSM_16, 1, 0, 1), GS_REG_TEX0);
		q++;
		PACK_GIFTAG(q, GS_SET_UV(0, 0), GS_REG_UV);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_UV(32 << 4, 32 << 4), GS_REG_UV);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(640 << 4, 448 << 4, 0), GS_REG_XYZ2);
		q++;

		FlushCache(0);
		dma_channel_send_normal(DMA_CHANNEL_GIF, draw_packet, q - draw_packet, 0, 0);
		WaitSema(vsync_sema_id);

		// Rotates the pallete every other frame
		// Rotates the texture every 5 frames
		s32 rotate_clut = frame_count % 2 == 0;
		s32 rotate_itex = frame_count % 5 == 0;

		if(rotate_clut)
			rotate_pallete();

		if(rotate_itex && itex_index == 4)
		{
			itex_index = 0;
		}

		if(rotate_clut || rotate_itex)
			upload_texture(rotate_itex, rotate_itex ? itex_index++ : 0);

		frame_count++;
	} while (1);

	free(draw_packet);
	SleepThread();
	return 0;
}
