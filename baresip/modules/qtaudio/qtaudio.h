/**
 * @file qtaudio.h Qt sound driver -- internal api
 *
 * Copyright (C) 2017 S.fuchita
 */

#ifdef __cplusplus
extern "C" {
#endif

int qtaudio_src_alloc(struct ausrc_st **stp, struct ausrc *as,
		      struct media_ctx **ctx,
		      struct ausrc_prm *prm, const char *device,
		      ausrc_read_h *rh, ausrc_error_h *errh, void *arg, struct user_extra_data_t *user_data);
int qtaudio_play_alloc(struct auplay_st **stp, struct auplay *ap,
		       struct auplay_prm *prm, const char *device,
			   auplay_write_h *wh, void *arg, struct user_extra_data_t *user_data);

#ifdef __cplusplus
}
#endif
