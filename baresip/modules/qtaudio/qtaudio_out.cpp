#ifdef UNICODE
#undef UNICODE
#endif

#include <new>
#include <re.h>
#include <rem.h>
#include <baresip.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#define DEBUG_MODULE "qtaudio"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

#include <vector>
#include "qtaudio.h"

class AudioPlayer {
private:
	auplay_write_h *source = nullptr;
	void *arg = nullptr;
	int frame_size = 0;
protected:

	void process(AudioIO *audio_io)
	{
		int len = frame_size * 2;
		char *buf = (char *)alloca(len);
		int n = audio_io->output(nullptr, 0);
		if (n >= len && source((uint8_t *)buf, len, arg)) {
			audio_io->output(&buf[0], len);
		}
	}

	static void process_(void *cookie, void *audio_io)
	{
		((AudioPlayer *)cookie)->process((AudioIO *)audio_io);
	}

public:

	void go(auplay_write_h *src, void *arg, int frame_size, user_extra_data_t *user_data)
	{
		this->source = src;
		this->arg = arg;
		this->frame_size = frame_size;
		if (user_data) {
			user_data->callback_output_p = this;
			user_data->callback_output_f = process_;
		}
	}
};

struct auplay_st {
	struct auplay *ap;
	AudioPlayer obj;
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

