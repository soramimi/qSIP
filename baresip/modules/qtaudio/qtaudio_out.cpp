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
#include <QAudioOutput>
#include <QThread>
#include <memory>

#define DEBUG_MODULE "qtaudio"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

static const int sample_fq = 8000;
static double dtmf_levels[8] = {};

#include <QDebug>
#include <math.h>

static const double  PI2 = M_PI * 2;

static const int dtmf_fq[8] = { 697, 770, 852, 941, 1209, 1336, 1477, 1633 };

double goertzel(int size, int16_t const *data, int sample_fq, int detect_fq)
{
	if (size < 2) return 0;

	double omega = PI2 * detect_fq / sample_fq;
	double sine = sin(omega);
	double cosine = cos(omega);
	double coeff = cosine * 2;
	double q0 = 0;
	double q1 = 0;
	double q2 = 0;

	for (int i = 0; i < size; i++) {
		q0 = coeff * q1 - q2 + data[i];
		q2 = q1;
		q1 = q0;
	}

	double real = (q1 - q2 * cosine) / (size / 2.0);
	double imag = (q2 * sine) / (size / 2.0);

	return sqrt(real * real + imag * imag);
}

class QtAudioPlayer : public QThread {
private:
	int frame_size;

	auplay_write_h *source;
	void *arg;
	void *user_cookie;
	user_notify_fn user_notify;
	user_filter_fn user_output_filter;

	std::shared_ptr<QAudioOutput> audio_output_;
	QIODevice *audio_output_device_;
	QAudioOutput *audio_output()
	{
		return audio_output_.get();
	}
	QIODevice *audio_output_device()
	{
		return audio_output_device_;
	}
	int write_audio_output(char *ptr, int len)
	{
		if (ptr) {
			return audio_output_device()->write(ptr, len);
		} else {
			return audio_output()->bytesFree();
		}
	}

	int dtmf_value = 0;
	int dtmf_count = 0;

	const int MIN_LEVEL = 500;
	const int MIN_COUNT = 300;

	void detect_dtmf(int size, int16_t const *data)
	{
		for (int i = 0; i < 8; i++) {
			dtmf_levels[i] = goertzel(size, data, sample_fq, dtmf_fq[i]);
		}

		int v = 0;
		struct Tone {
			int index;
			int level;
			Tone(int i, int v)
				: index(i)
				, level(v)
			{
			}
		};
		Tone lo[] = { Tone(0, dtmf_levels[0]), Tone(1, dtmf_levels[1]), Tone(2, dtmf_levels[2]), Tone(3, dtmf_levels[3]) };
		Tone hi[] = { Tone(0, dtmf_levels[4]), Tone(1, dtmf_levels[5]), Tone(2, dtmf_levels[6]), Tone(3, dtmf_levels[7]) };
		std::sort(lo, lo + 4, [](Tone const &l, Tone const &r){ return l.level > r.level; });
		std::sort(hi, hi + 4, [](Tone const &l, Tone const &r){ return l.level > r.level; });
		if (lo[0].level > MIN_LEVEL && hi[0].level > MIN_LEVEL && lo[0].level > lo[1].level * 3 && hi[0].level > hi[1].level * 3) {
			int i = lo[0].index * 4 + hi[0].index;
			v = "123A456B789C*0#D"[i];
		}
		if (v != 0 && v == dtmf_value) {
			if (dtmf_count < MIN_COUNT) {
				dtmf_count += size;
				if (dtmf_count >= MIN_COUNT) {
					if (user_notify) {
						char tmp[100];
						sprintf(tmp, "<dtmf>%c", dtmf_value);
						user_notify(user_cookie, tmp, strlen(tmp));
					}
				}
			}
		} else {
			dtmf_value = v;
			dtmf_count = 0;
		}
	}

protected:
	void run()
	{
		QAudioDeviceInfo defdev = QAudioDeviceInfo::defaultOutputDevice();
		QAudioFormat format;
		format.setChannelCount(1);
		format.setSampleRate(8000);
		format.setSampleSize(16);
		format.setCodec("audio/pcm");
		format.setSampleType(QAudioFormat::SignedInt);
		format.setByteOrder(QAudioFormat::LittleEndian);
		audio_output_ = std::shared_ptr<QAudioOutput>(new QAudioOutput(defdev, format));
		audio_output_device_ = audio_output_->start();

		std::vector<char> buf(frame_size * 2);
		while (!isInterruptionRequested()) {
			int len = write_audio_output(nullptr, 0);
			if (len >= buf.size() && source((uint8_t *)&buf[0], buf.size(), arg)) {
				len = buf.size();
				write_audio_output(&buf[0], len);
				len /= 2;
				if (len > 0) {
					int16_t *ptr = (int16_t *)&buf[0];
					if (user_output_filter) {
						user_output_filter(user_cookie, ptr, len);
					}
					detect_dtmf(len, ptr);
				}
			}
			yieldCurrentThread();
		}
	}
public:
	~QtAudioPlayer()
	{
		requestInterruption();
		wait();
	}

	void go(auplay_write_h *src, void *arg, int frame_size, user_extra_data_t *user_data)
	{
		this->source = src;
		this->arg = arg;
		this->frame_size = frame_size;
		this->user_cookie = user_data ? user_data->cookie : nullptr;
		this->user_notify = user_data ? user_data->notify : nullptr;
		this->user_output_filter = user_data ? user_data->output_filter : nullptr;
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

int qtaudio_play_alloc(auplay_st **stp, struct auplay *ap, auplay_prm *prm, const char *device, auplay_write_h *wh, void *arg, user_extra_data_t *user_data)
{
	(void)device;

	if (!stp || !ap || !prm) return EINVAL;

	auplay_st *st = (auplay_st *)mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st) return ENOMEM;

	prm->fmt = AUFMT_S16LE;

	new(st) auplay_st();
	st->ap  = (struct auplay *)mem_ref(ap);
	st->obj.go(wh, arg, prm->frame_size, user_data);

	*stp = st;

	return 0;
}
