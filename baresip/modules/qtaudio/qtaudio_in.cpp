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
#include <baresip.h>
#include "qtaudio.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <QAudioInput>
#include <QDebug>
#include <QThread>
#include <memory>

#define DEBUG_MODULE "qtaudio"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

class QtAudioSource : public QThread {
private:
	int frame_size;

	ausrc_read_h *sink;
	void *arg;
	user_filter_fn user_filter;
	void *user_cookie;

	std::shared_ptr<QAudioInput> input;
	QIODevice *device;
protected:
	void run()
	{
		std::vector<char> buf(frame_size * 2);
		while (1) {
			if (isInterruptionRequested()) break;

			int len = buf.size();
			if (input->bytesReady() < len) {
				msleep(1);
			} else {
				char *p = &buf[0];
				len = device->read(p, len);

				if (user_filter) {
					int16_t *p = (int16_t *)p;
					int n = len / 2;
					user_filter(user_cookie, p, n);
				}

				sink((uint8_t *)p, len, arg);
			}
		}
	}
public:
	~QtAudioSource()
	{
		requestInterruption();
		wait();
	}

	void go(ausrc_read_h *sink, void *arg, int frame_size, user_extra_data_t *user_data)
	{
		this->sink = sink;
		this->arg = arg;
		this->frame_size = frame_size;
		this->user_filter = user_data ? user_data->filter : nullptr;
		this->user_cookie = user_data ? user_data->cookie : nullptr;
		QAudioFormat format;
		format.setByteOrder(QAudioFormat::LittleEndian);
		format.setChannelCount(1);
		format.setCodec("audio/pcm");
		format.setSampleRate(8000);
		format.setSampleSize(16);
		format.setSampleType(QAudioFormat::SignedInt);
		input = std::shared_ptr<QAudioInput>(new QAudioInput(format));
		device = input->start();
		start();
	}
};

struct ausrc_st {
	struct ausrc *as;      /* inheritance */
	QtAudioSource obj;

};

static void ausrc_destructor(void *arg)
{
	ausrc_st *st = (ausrc_st *)arg;
	mem_deref(st->as);
	st->~ausrc_st();
}

int qtaudio_src_alloc(ausrc_st **stp, struct ausrc *as, struct media_ctx **ctx, ausrc_prm *prm, const char *device, ausrc_read_h *rh, ausrc_error_h *errh, void *arg, user_extra_data_t *user_data)
{
	(void)ctx;
	(void)device;
	(void)errh;

	if (!stp || !as || !prm) return EINVAL;

	ausrc_st *st = (ausrc_st *)mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st) return ENOMEM;

	prm->fmt = AUFMT_S16LE;

	new(st) ausrc_st();
	st->as  = (struct ausrc *)mem_ref(as);
	st->obj.go(rh, arg, prm->frame_size, user_data);

	*stp = st;

	return 0;
}
