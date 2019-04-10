#ifdef UNICODE
#undef UNICODE
#endif

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

#include <QCoreApplication>
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
	void *user_cookie;
	user_notify_fn user_notify;
	user_filter_fn user_input_filter;

	std::shared_ptr<QAudioInput> audio_input_;
	QIODevice *audio_input_device_;
	QAudioInput *audio_input()
	{
		return audio_input_.get();
	}
	QIODevice *audio_input_device()
	{
		return audio_input_device_;
	}

	int read_audio_input(char *ptr, int len)
	{
		if (audio_input()->bytesReady() < len) {
			len = 0;
		} else {
			len = audio_input_device()->read(ptr, len);
		}
		return len;
	}

protected:
	void run()
	{
		QAudioDeviceInfo defdev = QAudioDeviceInfo::defaultInputDevice();
		QAudioFormat format;
		format.setChannelCount(1);
		format.setSampleRate(8000);
		format.setSampleSize(16);
		format.setCodec("audio/pcm");
		format.setSampleType(QAudioFormat::SignedInt);
		format.setByteOrder(QAudioFormat::LittleEndian);
		audio_input_ = std::shared_ptr<QAudioInput>(new QAudioInput(defdev, format));
		audio_input_device_ = audio_input_->start();

		std::vector<char> buf(frame_size * 2);
		while (!isInterruptionRequested()) {
			int len = read_audio_input(&buf[0], buf.size());
			len /= 2;
			if (len > 0) {
				void *ptr = &buf[0];
				if (user_input_filter) {
					user_input_filter(user_cookie, (int16_t *)ptr, len);
				}
				sink((uint8_t *)ptr, len * 2, arg);
			}
			QCoreApplication::processEvents();
			yieldCurrentThread();
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
		this->user_cookie = user_data ? user_data->cookie : nullptr;
		this->user_notify = user_data ? user_data->notify : nullptr;
		this->user_input_filter = user_data ? user_data->input_filter : nullptr;
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
