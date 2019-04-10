/**
 * @file qtaudio.c Qt sound driver
 *
 * Copyright (C) 2019 S.Fuchita
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "qtaudio.h"


#define DEBUG_MODULE "qtaudio"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static struct ausrc *ausrc;
static struct auplay *auplay;


static int qtaudio_init(void)
{
	int err;

	err  = ausrc_register(&ausrc, "qtaudio", qtaudio_src_alloc);
	err |= auplay_register(&auplay, "qtaudio", qtaudio_play_alloc);

	return err;
}


static int qtaudio_close(void)
{
	ausrc = (struct ausrc *)mem_deref(ausrc);
	auplay = (struct auplay *)mem_deref(auplay);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(qtaudio) = {
	"qtaudio",
	"sound",
	qtaudio_init,
	qtaudio_close
};
