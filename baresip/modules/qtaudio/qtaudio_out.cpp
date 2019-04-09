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

	std::shared_ptr<QAudioOutput> output;
	QIODevice *device;

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
					qDebug() << (char)dtmf_value;
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
		output = std::shared_ptr<QAudioOutput>(new QAudioOutput(defdev, format));
        device = output->start();

        std::vector<char> buf(frame_size * 2);
		while (1) {
			if (isInterruptionRequested()) break;
			int n = output->bytesFree();
			if (n < (int)buf.size()) {
				QThread::yieldCurrentThread();
			} else if (source((uint8_t *)&buf[0], buf.size(), arg)) {
				device->write(&buf[0], buf.size());
				detect_dtmf(buf.size() / 2, (int16_t const *)&buf[0]);
			}
            QCoreApplication::processEvents();
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
