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

class AudioSource {
private:
	ausrc_read_h *sink = nullptr;
	void *arg = nullptr;
	int frame_size = 0;
protected:

	void process(AudioIO *audio_io)
	{
		int len = frame_size * 2;
		char *buf = (char *)alloca(len);
		int n = audio_io->input(buf, len) / 2;
		if (n > 0) {
			void *ptr = buf;
			sink((uint8_t *)ptr, n * 2, arg);
		}
	}

	static void process_(void *cookie, void *audio_io)
	{
		((AudioSource *)cookie)->process((AudioIO *)audio_io);
	}

public:
	void go(ausrc_read_h *sink, void *arg, int frame_size, user_extra_data_t *user_data)
	{
		this->sink = sink;
		this->arg = arg;
		this->frame_size = frame_size;
		if (user_data) {
			user_data->callback_input_p = this;
			user_data->callback_input_f = process_;
		}
	}
};

struct ausrc_st {
	struct ausrc *as;
	AudioSource obj;
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

