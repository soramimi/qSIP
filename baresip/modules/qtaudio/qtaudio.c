/**
 * @file qtaudio.c Qt sound driver
 *
 * Copyright (C) 2017 S.Fuchita
 */
#include <re.h>
#include <rem.h>
#include <windows.h>
#include <mmsystem.h>
#include <baresip.h>
#include "qtaudio.h"


#define DEBUG_MODULE "qtaudio"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static struct ausrc *ausrc;
static struct auplay *auplay;


static int ww_init(void)
{
	int play_dev_count, src_dev_count;
	int err;

	play_dev_count = waveOutGetNumDevs();
	src_dev_count = waveInGetNumDevs();

	re_printf("qtaudio: output devices: %d, input devices: %d\n",
		  play_dev_count, src_dev_count);

	err  = ausrc_register(&ausrc, "qtaudio", qtaudio_src_alloc);
	err |= auplay_register(&auplay, "qtaudio", qtaudio_play_alloc);

	return err;
}


static int ww_close(void)
{
	ausrc = mem_deref(ausrc);
	auplay = mem_deref(auplay);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(qtaudio) = {
	"qtaudio",
	"sound",
	ww_init,
	ww_close
};
