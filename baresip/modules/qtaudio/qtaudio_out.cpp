#ifdef UNICODE
#undef UNICODE
#endif
/**
 * @file qtaudio/play.c Windows sound driver -- playback
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

#include <QAudioOutput>

#define WRITE_BUFFERS  4
#define INC_WPOS(a) ((a) = (((a) + 1) % WRITE_BUFFERS))

class QtAudioPlayer {
public:
	struct dspbuf bufs[WRITE_BUFFERS];
	int pos;
	HWAVEOUT waveout;
	bool rdy;
	bool closed;
	size_t inuse;
	auplay_write_h *writer;
	void *arg;

	QAudioOutput output;

	int dsp_write();
	static void CALLBACK waveOutCallback_(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	int write_stream_open(unsigned int dev, const auplay_prm *prm);
	void waveOutCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
};

struct auplay_st {
	struct auplay *ap;      /* inheritance */
	QtAudioPlayer obj;
};

static void auplay_destructor(void *arg)
{
	auplay_st *st = (auplay_st *)arg;
	int i;

	st->obj.writer = NULL;

	/* Mark the device for closing, and wait for all the
	 * buffers to be returned by the driver
	 */
	st->obj.rdy = false;
	while (st->obj.inuse > 0)
		Sleep(50);

	if (st->obj.waveout) {
		waveOutReset(st->obj.waveout);
	}

	for (i = 0; i < WRITE_BUFFERS; i++) {
		if (st->obj.waveout) {
			waveOutUnprepareHeader(st->obj.waveout, &st->obj.bufs[i].wh, sizeof(WAVEHDR));
		}
		mem_deref(st->obj.bufs[i].mb);
	}

	if (st->obj.waveout) {
		waveOutClose(st->obj.waveout);
	}

	for (i=0; i<100; i++) {
		if (st->obj.closed) break;
		Sleep(10);
    }

	mem_deref(st->ap);

	st->~auplay_st();
}

int QtAudioPlayer::dsp_write()
{
	MMRESULT res;
	WAVEHDR *wh;
	struct mbuf *mb;

	if (!rdy)
		return EINVAL;

	wh = &bufs[pos].wh;
	if (wh->dwFlags & WHDR_PREPARED) {
		return EINVAL;
	}
	mb = bufs[pos].mb;
	wh->lpData = (LPSTR)mb->buf;

	if (writer) {
		writer(mb->buf, mb->size, arg);
	}

	wh->dwBufferLength = mb->size;
	wh->dwFlags = 0;
	wh->dwUser = (DWORD_PTR) mb;

	if (waveout) {
		waveOutPrepareHeader(waveout, wh, sizeof(*wh));
	}

	INC_WPOS(pos);

	if (waveout) {
		res = waveOutWrite(waveout, wh, sizeof(*wh));
		if (res != MMSYSERR_NOERROR)
			DEBUG_WARNING("dsp_write: waveOutWrite: failed: %08x\n", res);
		else
			inuse++;
	}

	return 0;
}

void QtAudioPlayer::waveOutCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	WAVEHDR *wh = (WAVEHDR *)dwParam1;

	(void)hwo;
	(void)dwParam2;
	switch (uMsg) {

	case WOM_OPEN:
		rdy = true;
		break;

	case WOM_DONE:
		/*LOCK();*/
		waveOutUnprepareHeader(waveout, wh, sizeof(*wh));
		/*UNLOCK();*/
		inuse--;
		dsp_write();
		break;

	case WOM_CLOSE:
		rdy = false;
		closed = true;
		break;

	default:
		break;
	}
}

void CALLBACK QtAudioPlayer::waveOutCallback_(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	QtAudioPlayer *obj = (QtAudioPlayer *)dwInstance;
	obj->waveOutCallback(hwo, uMsg, dwParam1, dwParam2);
}

static unsigned int find_dev(const char *dev)
{
	WAVEOUTCAPS wic;
	unsigned int i;
	unsigned int nInDevices = waveOutGetNumDevs();
	for (i = 0; i < (int)nInDevices; i++) {
		if (waveOutGetDevCaps(i, &wic, sizeof(WAVEOUTCAPS))==MMSYSERR_NOERROR) {
			if (!strcmp(dev, wic.szPname)) {
				return i;
			}
		}
	}
	return WAVE_MAPPER;
}

int QtAudioPlayer::write_stream_open(unsigned int dev, const struct auplay_prm *prm)
{
	WAVEFORMATEX wfmt;
	MMRESULT res;
	int i;

	/* Open an audio I/O stream. */
	waveout = NULL;
	pos = 0;
	rdy = false;
	closed = false;

	for (i = 0; i < WRITE_BUFFERS; i++) {
		memset(&bufs[i].wh, 0, sizeof(WAVEHDR));
		bufs[i].mb = mbuf_alloc(2 * prm->frame_size);
	}

	wfmt.wFormatTag      = WAVE_FORMAT_PCM;
	wfmt.nChannels       = prm->ch;
	wfmt.nSamplesPerSec  = prm->srate;
	wfmt.wBitsPerSample  = 16;
	wfmt.nBlockAlign     = (prm->ch * wfmt.wBitsPerSample) / 8;
	wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;
	wfmt.cbSize          = 0;

	res = waveOutOpen(&waveout, dev, &wfmt,
			  (DWORD_PTR) QtAudioPlayer::waveOutCallback_,
			  (DWORD_PTR) this,
			  CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT);
	if (res != MMSYSERR_NOERROR) {
		DEBUG_WARNING("waveOutOpen: failed %d\n", res);
//		return EINVAL;
		waveout = NULL;
	}

	return 0;
}

int qtaudio_play_alloc(auplay_st **stp, struct auplay *ap, auplay_prm *prm, const char *device, auplay_write_h *wh, void *arg, void *user_data)
{
	auplay_st *st;
	int i, err;
	(void)device;
	(void)user_data;

	if (!stp || !ap || !prm)
		return EINVAL;

	st = (auplay_st *)mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st)
		return ENOMEM;

	new(st) auplay_st();

	st->ap  = (struct auplay *)mem_ref(ap);
	st->obj.writer  = wh;
	st->obj.arg = arg;

	prm->fmt = AUFMT_S16LE;

	err = st->obj.write_stream_open(find_dev(device), prm);
	if (err)
		goto out;

	/* The write runs at 100ms intervals
	 * prepare enough buffers to suite its needs
	 */
	for (i = 0; i < 5; i++)
		st->obj.dsp_write();

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}
