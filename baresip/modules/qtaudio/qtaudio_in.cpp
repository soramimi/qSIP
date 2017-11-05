#ifdef UNICODE
#undef UNICODE
#endif
/**
 * @file qtaudio/src.c Windows sound driver -- source
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <new>
#include <re.h>
#include <rem.h>
#include <windows.h>
#include <mmsystem.h>
#include <baresip.h>
#include "qtaudio.h"

#define DEBUG_MODULE "qtaudio"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

#include <QAudioInput>

#define READ_BUFFERS   4
#define INC_RPOS(a) ((a) = (((a) + 1) % READ_BUFFERS))

class QtAudioSource {
public:
	struct dspbuf bufs[READ_BUFFERS];
	int pos;
	HWAVEIN wavein;
	volatile bool rdy;
	volatile bool processing;
	ausrc_read_h *rh;
	void *arg;
	user_filter_fn user_filter;
	void *user_cookie;

	QAudioInput input;

	int add_wave_in();
	static void CALLBACK waveInCallback_(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	int read_stream_open(unsigned int dev, const ausrc_prm *prm);
	void waveInCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
};

struct ausrc_st {
	struct ausrc *as;      /* inheritance */
	QtAudioSource obj;

};

static void ausrc_destructor(void *arg)
{
	ausrc_st *st = (ausrc_st *)arg;
	int i;

	/* Mark the device for closing
	 */
	st->obj.rdy = false;
	// pause if callback is processed at the moment
	// (hazard with st->rh, possible hangup in waveInReset)
	while (st->obj.processing) {
    	Sleep(10);
	}

	st->obj.rh = NULL;

	if (st->obj.wavein) {
		waveInStop(st->obj.wavein);
		// WIM_DATA may be sent
		waveInReset(st->obj.wavein);

		for (i = 0; i < READ_BUFFERS; i++) {
			waveInUnprepareHeader(st->obj.wavein, &st->obj.bufs[i].wh, sizeof(WAVEHDR));
			mem_deref(st->obj.bufs[i].mb);
		}
	}

	waveInClose(st->obj.wavein);

	mem_deref(st->as);

	st->~ausrc_st();
}

int QtAudioSource::add_wave_in()
{
	struct dspbuf *db = &bufs[pos];
	WAVEHDR *wh = &db->wh;
	MMRESULT res;

	wh->lpData          = (LPSTR)db->mb->buf;
	wh->dwBufferLength  = db->mb->size;
	wh->dwBytesRecorded = 0;
	wh->dwFlags         = 0;
	wh->dwUser          = (DWORD_PTR)db->mb;

	if (wavein) {
		waveInPrepareHeader(wavein, wh, sizeof(*wh));
		res = waveInAddBuffer(wavein, wh, sizeof(*wh));
		if (res != MMSYSERR_NOERROR) {
			DEBUG_WARNING("add_wave_in: waveInAddBuffer fail: %08x\n", res);
			return ENOMEM;
		}
	}

	INC_RPOS(pos);

	return 0;
}

void QtAudioSource::waveInCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	WAVEHDR *wh = (WAVEHDR *)dwParam1;

	(void)hwo;
	(void)dwParam2;

	if (!rh) return;

	switch (uMsg) {
	case WIM_CLOSE:
		rdy = false;
		break;

	case WIM_OPEN:
		rdy = true;
		break;

	case WIM_DATA:
		processing = true;
		if (rdy) {
			if (wavein) {
				waveInUnprepareHeader(wavein, wh, sizeof(*wh));
			} else {
				int16_t *p = (int16_t *)wh->lpData;
				int n = wh->dwBytesRecorded / 2;
				for (int i = 0; i < n; i++) {
					p[i] = 0;
				}
			}

			if (user_filter) {
				int16_t *p = (int16_t *)wh->lpData;
				int n = wh->dwBytesRecorded / 2;
				user_filter(user_cookie, p, n);
			}

			rh((uint8_t *)wh->lpData, wh->dwBytesRecorded, arg);

			add_wave_in();
		}
		processing = false;
		break;

	default:
		break;
	}
}

void CALLBACK QtAudioSource::waveInCallback_(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	QtAudioSource *obj = (QtAudioSource *)dwInstance;
	obj->waveInCallback(hwo, uMsg, dwParam1, dwParam2);
}

static unsigned int find_dev(const char *name)
{
	WAVEINCAPS wic;
	unsigned int i;
	unsigned int nInDevices = waveInGetNumDevs();
	for (i = 0; i < nInDevices; i++) {
		if (waveInGetDevCaps(i, &wic, sizeof(WAVEINCAPS))==MMSYSERR_NOERROR) {
			if (!strcmp(name, wic.szPname)) {
				return i;
			}
		}
	}
	return WAVE_MAPPER;
}

int QtAudioSource::read_stream_open(unsigned int dev, const struct ausrc_prm *prm)
{
	WAVEFORMATEX wfmt;
	MMRESULT res;
	int i, err = 0;

	/* Open an audio INPUT stream. */
	wavein = NULL;
	pos = 0;
	rdy = false;
	processing = false;

	for (i = 0; i < READ_BUFFERS; i++) {
		memset(&bufs[i].wh, 0, sizeof(WAVEHDR));
		bufs[i].mb = mbuf_alloc(2 * prm->frame_size);
		if (!bufs[i].mb)
			return ENOMEM;
	}

	wfmt.wFormatTag      = WAVE_FORMAT_PCM;
	wfmt.nChannels       = prm->ch;
	wfmt.nSamplesPerSec  = prm->srate;
	wfmt.wBitsPerSample  = 16;
	wfmt.nBlockAlign     = (prm->ch * wfmt.wBitsPerSample) / 8;
	wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;
	wfmt.cbSize          = 0;

	res = waveInOpen(&wavein, dev, &wfmt,
			  (DWORD_PTR) QtAudioSource::waveInCallback_,
			  (DWORD_PTR) this,
			  CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT);
	if (res != MMSYSERR_NOERROR) {
		DEBUG_WARNING("waveInOpen: failed %d\n", err);
//		return EINVAL;
		wavein = NULL;
	}

	/* Prepare enough IN buffers to suite at least 50ms of data */
	for (i = 0; i < READ_BUFFERS; i++)
		err |= add_wave_in();

	if (wavein) {
		waveInStart(wavein);
	}

	return err;
}

int qtaudio_src_alloc(ausrc_st **stp, struct ausrc *as, struct media_ctx **ctx, ausrc_prm *prm, const char *device, ausrc_read_h *rh, ausrc_error_h *errh, void *arg, user_extra_data_t *user_data)
{
	ausrc_st *st;
	int err;

	(void)ctx;
	(void)device;
	(void)errh;

	if (!stp || !as || !prm)
		return EINVAL;

	st = (ausrc_st *)mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	new(st) ausrc_st();

	st->as  = (struct ausrc *)mem_ref(as);
	st->obj.rh  = rh;
	st->obj.arg = arg;
	st->obj.user_filter = user_data ? user_data->filter : NULL;
	st->obj.user_cookie = user_data ? user_data->cookie : NULL;

	prm->fmt = AUFMT_S16LE;

	err = st->obj.read_stream_open(find_dev(device), prm);

	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}
