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
#include <baresip.h>
#include "qtaudio.h"

#include <QAudioOutput>
#include <QThread>
#include <memory>

#define DEBUG_MODULE "qtaudio"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

class QtAudioPlayer : public QThread {
private:
	int frame_size;

	auplay_write_h *source;
	void *arg;

	std::shared_ptr<QAudioOutput> output;
	QIODevice *device;

protected:
	void run()
	{
		std::vector<char> buf(frame_size * 2);
		while (1) {
			if (isInterruptionRequested()) break;
			int n = output->bytesFree();
			if (n < (int)buf.size()) {
				msleep(1);
			} else if (source((uint8_t *)&buf[0], buf.size(), arg)) {
				device->write(&buf[0], buf.size());
			}
		}
	}
public:
	~QtAudioPlayer()
	{
		requestInterruption();
		wait();
	}

	void go(auplay_write_h *src, void *arg, int frame_size)
	{
		this->source = src;
		this->arg = arg;
		this->frame_size = frame_size;
		QAudioFormat format;
		format.setByteOrder(QAudioFormat::LittleEndian);
		format.setChannelCount(1);
		format.setCodec("audio/pcm");
		format.setSampleRate(8000);
		format.setSampleSize(16);
		format.setSampleType(QAudioFormat::SignedInt);
		output = std::shared_ptr<QAudioOutput>(new QAudioOutput(format));
		device = output->start();
		start();
	}
};

struct auplay_st {
	struct auplay *ap;      /* inheritance */
	QtAudioPlayer obj;
};

static void auplay_destructor(void *arg)
{
	auplay_st *st = (auplay_st *)arg;
	mem_deref(st->ap);
	st->~auplay_st();
}

int qtaudio_play_alloc(auplay_st **stp, struct auplay *ap, auplay_prm *prm, const char *device, auplay_write_h *wh, void *arg, void *user_data)
{
	(void)device;
	(void)user_data;

	if (!stp || !ap || !prm) return EINVAL;

	auplay_st *st = (auplay_st *)mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st) return ENOMEM;

	prm->fmt = AUFMT_S16LE;

	new(st) auplay_st();
	st->ap  = (struct auplay *)mem_ref(ap);
	st->obj.go(wh, arg, prm->frame_size);

	*stp = st;

	return 0;
}
