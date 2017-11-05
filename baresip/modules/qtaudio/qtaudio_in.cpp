#ifdef UNICODE
#undef UNICODE
#endif
/**
 * @file qtaudio/src.c Windows sound driver -- source
 *
 * Copyright (C) 2010 Creytiv.com
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


#define READ_BUFFERS   4
#define INC_RPOS(a) ((a) = (((a) + 1) % READ_BUFFERS))

struct ausrc_st {
	struct ausrc *as;      /* inheritance */
	struct dspbuf bufs[READ_BUFFERS];
	int pos;
	HWAVEIN wavein;
	volatile bool rdy;
	volatile bool processing;
//	size_t inuse;
	ausrc_read_h *rh;
	void *arg;

	user_filter_fn user_filter;
	void *user_cookie;
};


static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = (struct ausrc_st *)arg;
	int i;

	/* Mark the device for closing
	 */
	st->rdy = false;
	// pause if callback is processed at the moment
	// (hazard with st->rh, possible hangup in waveInReset)
	while (st->processing) {
    	Sleep(10);
	}

	st->rh = NULL;

	if (st->wavein) {
		waveInStop(st->wavein);
		// WIM_DATA may be sent
		waveInReset(st->wavein);

		for (i = 0; i < READ_BUFFERS; i++) {
			waveInUnprepareHeader(st->wavein, &st->bufs[i].wh, sizeof(WAVEHDR));
			mem_deref(st->bufs[i].mb);
		}
	}

	waveInClose(st->wavein);

	mem_deref(st->as);
}


static int add_wave_in(struct ausrc_st *st)
{
	struct dspbuf *db = &st->bufs[st->pos];
	WAVEHDR *wh = &db->wh;
	MMRESULT res;

	wh->lpData          = (LPSTR)db->mb->buf;
	wh->dwBufferLength  = db->mb->size;
	wh->dwBytesRecorded = 0;
	wh->dwFlags         = 0;
	wh->dwUser          = (DWORD_PTR)db->mb;

	if (st->wavein) {
		waveInPrepareHeader(st->wavein, wh, sizeof(*wh));
		res = waveInAddBuffer(st->wavein, wh, sizeof(*wh));
		if (res != MMSYSERR_NOERROR) {
			DEBUG_WARNING("add_wave_in: waveInAddBuffer fail: %08x\n", res);
			return ENOMEM;
		}
	}

	INC_RPOS(st->pos);

//	st->inuse++;

	return 0;
}


static void CALLBACK waveInCallback(HWAVEOUT hwo,
				    UINT uMsg,
				    DWORD_PTR dwInstance,
				    DWORD_PTR dwParam1,
				    DWORD_PTR dwParam2)
{
	struct ausrc_st *st = (struct ausrc_st *)dwInstance;
	WAVEHDR *wh = (WAVEHDR *)dwParam1;

	(void)hwo;
	(void)dwParam2;

	if (!st->rh) return;

	switch (uMsg) {

	case WIM_CLOSE:
		st->rdy = false;
		break;

	case WIM_OPEN:
		st->rdy = true;
		break;

	case WIM_DATA:
		st->processing = true;
		if (st->rdy) {
			if (st->wavein) {
				waveInUnprepareHeader(st->wavein, wh, sizeof(*wh));
			} else {
				int16_t *p = (int16_t *)wh->lpData;
				int n = wh->dwBytesRecorded / 2;
				for (int i = 0; i < n; i++) {
					p[i] = 0x8000;
				}
			}

			if (st->user_filter) {
				int16_t *p = (int16_t *)wh->lpData;
				int n = wh->dwBytesRecorded / 2;
				st->user_filter(st->user_cookie, p, n);
			}

			st->rh((uint8_t *)wh->lpData, wh->dwBytesRecorded, st->arg);

			//if (st->inuse < 3)
				add_wave_in(st);
			//st->inuse--;
		}
		st->processing = false;
		break;

	default:
		break;
	}
}

static unsigned int find_dev(const char* name) {
	WAVEINCAPS wic;
	unsigned int i;
	int nInDevices = waveInGetNumDevs();
	for (i=0; i<nInDevices; i++) {
		if (waveInGetDevCaps(i, &wic, sizeof(WAVEINCAPS))==MMSYSERR_NOERROR) {
			if (!strcmp(name, wic.szPname)) {
				return i;
			}
		}
	}
	return WAVE_MAPPER;
}

static int read_stream_open(unsigned int dev, struct ausrc_st *st, const struct ausrc_prm *prm)
{
	WAVEFORMATEX wfmt;
	MMRESULT res;
	int i, err = 0;

	/* Open an audio INPUT stream. */
	st->wavein = NULL;
	st->pos = 0;
	st->rdy = false;
	st->processing = false;

	for (i = 0; i < READ_BUFFERS; i++) {
		memset(&st->bufs[i].wh, 0, sizeof(WAVEHDR));
		st->bufs[i].mb = mbuf_alloc(2 * prm->frame_size);
		if (!st->bufs[i].mb)
			return ENOMEM;
	}

	wfmt.wFormatTag      = WAVE_FORMAT_PCM;
	wfmt.nChannels       = prm->ch;
	wfmt.nSamplesPerSec  = prm->srate;
	wfmt.wBitsPerSample  = 16;
	wfmt.nBlockAlign     = (prm->ch * wfmt.wBitsPerSample) / 8;
	wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;
	wfmt.cbSize          = 0;

	res = waveInOpen(&st->wavein, dev, &wfmt,
			  (DWORD_PTR) waveInCallback,
			  (DWORD_PTR) st,
			  CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT);
	if (res != MMSYSERR_NOERROR) {
		DEBUG_WARNING("waveInOpen: failed %d\n", err);
//		return EINVAL;
		st->wavein = NULL;
	}

	/* Prepare enough IN buffers to suite at least 50ms of data */
	for (i = 0; i < READ_BUFFERS; i++)
		err |= add_wave_in(st);

	if (st->wavein) {
		waveInStart(st->wavein);
	}

	return err;
}

int qtaudio_src_alloc(struct ausrc_st **stp, struct ausrc *as,
		      struct media_ctx **ctx,
		      struct ausrc_prm *prm, const char *device,
			  ausrc_read_h *rh, ausrc_error_h *errh, void *arg, struct user_extra_data_t *user_data)
{
	struct ausrc_st *st;
	int err;

	(void)ctx;
	(void)device;
	(void)errh;

	if (!stp || !as || !prm)
		return EINVAL;

	st = (struct ausrc_st *)mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->as  = (struct ausrc *)mem_ref(as);
	st->rh  = rh;
	st->arg = arg;
	st->user_filter = user_data ? user_data->filter : NULL;
	st->user_cookie = user_data ? user_data->cookie : NULL;

	prm->fmt = AUFMT_S16LE;

	err = read_stream_open(find_dev(device), st, prm);

	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}
