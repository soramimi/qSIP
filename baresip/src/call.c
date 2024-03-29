/**
 * @file call.c  Call Control
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <baresip.h>
#include "core.h"


#define DEBUG_MODULE "call"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

/** Magic number */
#define MAGIC 0xca11ca11
#include "magic.h"


#ifndef RELEASE
#define CALL_DEBUG       1  /**< Enable call debugging */
#endif

#define FOREACH_STREAM						\
	for (le = call->streaml.head; le; le = le->next)

enum {
	PTIME           = 20,    /**< Packet time for audio               */
	LOCAL_TIMEOUT   = 120,   /**< Incoming call timeout in [seconds]  */
};


/** Call States */
enum state {
	STATE_IDLE = 0,
	STATE_INCOMING,
	STATE_OUTGOING,
	STATE_RINGING,
	STATE_EARLY,
	STATE_ESTABLISHED,
	STATE_TERMINATED
};

/** SIP Call Control object */
struct call {
	MAGIC_DECL                /**< Magic number for debugging           */
	struct le le;             /**< Linked list element                  */
	struct ua *ua;            /**< SIP User-agent                       */
	struct account *acc;      /**< Account (ref.)                       */
	struct sipsess *sess;     /**< SIP Session                          */
	struct sdp_session *sdp;  /**< SDP Session                          */
	struct sipsub *sub;       /**< Call transfer REFER subscription     */
	struct sipnot *not;       /**< REFER/NOTIFY client                  */
	struct list streaml;      /**< List of mediastreams (struct stream) */
	struct audio *audio;      /**< Audio stream                         */
#ifdef USE_VIDEO
	struct video *video;      /**< Video stream                         */
	struct bfcp *bfcp;        /**< BFCP Client                          */
#endif
	enum state state;         /**< Call state                           */
	char *local_uri;          /**< Local SIP uri                        */
	char *local_name;         /**< Local display name                   */
	char *peer_uri;           /**< Peer SIP Address                     */
	char *peer_name;          /**< Peer display name                    */
	char *alert_info;         /**< Alert-Info header value              */
	char *access_url;         /**< Access-URL header: URL               */
	char *initial_rx_invite;  /**< Intial received INVITE               */
	int access_url_mode;      /**< 0 = unknown, 1 = passive, 2 = active */
	struct tmr tmr_inv;       /**< Timer for incoming calls             */
	struct tmr tmr_dtmf;      /**< Timer for incoming DTMF events       */
	time_t time_start;        /**< Time when call started               */
	time_t time_stop;         /**< Time when call stopped               */
	bool got_offer;           /**< Got SDP Offer from Peer              */
	int answer_after;         /**< Call-info answer-after value; -1 if not present */
	struct mnat_sess *mnats;  /**< Media NAT session                    */
	bool mnat_wait;           /**< Waiting for MNAT to establish        */
	struct menc_sess *mencs;  /**< Media encryption session state       */
	int af;                   /**< Preferred Address Family             */
	uint16_t scode;           /**< Termination status code              */
	call_event_h *eh;         /**< Event handler                        */
	call_dtmf_h *dtmfh;       /**< DTMF handler                         */
	void *arg;                /**< Handler argument                     */
	uint32_t rtp_timeout_ms;  /**< RTP Timeout in [ms]                  */
};


static int send_invite(struct call *call);


static const char *state_name(enum state st)
{
	switch (st) {

	case STATE_IDLE:        return "IDLE";
	case STATE_INCOMING:    return "INCOMING";
	case STATE_OUTGOING:    return "OUTGOING";
	case STATE_RINGING:     return "RINGING";
	case STATE_EARLY:       return "EARLY";
	case STATE_ESTABLISHED: return "ESTABLISHED";
	case STATE_TERMINATED:  return "TERMINATED";
	default:                return "???";
	}
}


static void set_state(struct call *call, enum state st)
{
	DEBUG_INFO("State %s -> %s\n", state_name(call->state),
		   state_name(st));
	call->state = st;
}


static void call_stream_start(struct call *call, bool active, void *user_data)
{
	const struct sdp_format *sc;
	int err;

	/* Audio Stream */
	sc = sdp_media_rformat(stream_sdpmedia(audio_strm(call->audio)), NULL);
	if (sc) {
		struct aucodec *ac = sc->data;

		if (ac) {
			err  = audio_encoder_set(call->audio, sc->data, sc->pt, sc->params, user_data);
			if (err) {
				DEBUG_WARNING("call: start:"
					" audio_encoder_set error: %m\n", err);
			}
			err |= audio_decoder_set(call->audio, sc->data, sc->pt, sc->params, user_data);
			if (err) {
				DEBUG_WARNING("call: start:"
					" audio_decoder_set error: %m\n", err);
			}

			if (!err) {
				err = audio_start(call->audio, NULL);
			}
			if (err) {
				DEBUG_WARNING("audio stream: %m\n", err);
			}
		}
		else {
			(void)re_printf("no common audio-codecs..\n");
		}
	}
	else {
		(void)re_printf("call: audio stream is disabled..\n");
	}

#ifdef USE_VIDEO
	/* Video Stream */
	sc = sdp_media_rformat(stream_sdpmedia(video_strm(call->video)), NULL);
	if (sc) {
		err  = video_encoder_set(call->video, sc->data, sc->pt,
					 sc->params);
		err |= video_decoder_set(call->video, sc->data, sc->pt,
					 sc->rparams);
		if (!err) {
			err = video_start(call->video, call->peer_uri);
		}
		if (err) {
			DEBUG_WARNING("video stream: %m\n", err);
		}
	}
	else if (call->video) {
		(void)re_printf("video stream is disabled..\n");
	}

	if (call->bfcp) {
		err = bfcp_start(call->bfcp);
		if (err) {
			DEBUG_WARNING("bfcp_start() error: %m\n", err);
		}
	}
#endif

	if (active) {
		struct le *le;

		tmr_cancel(&call->tmr_inv);
		call->time_start = time(NULL);

		FOREACH_STREAM {
			stream_reset(le->data);
		}
	}
}


static void call_stream_stop(struct call *call)
{
	if (!call)
		return;

	call->time_stop = time(NULL);

	/* Audio */
	audio_stop(call->audio);

	/* Video */
#ifdef USE_VIDEO
	video_stop(call->video);
#endif

	tmr_cancel(&call->tmr_inv);
}


static void call_event_handler(struct call *call, enum call_event ev,
			       const char *fmt, ...)
{
	call_event_h *eh = call->eh;
	void *eh_arg = call->arg;
	char buf[256];
	va_list ap;

	if (!eh)
		return;

	va_start(ap, fmt);
	(void)re_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	eh(call, ev, buf, eh_arg);
}


static void invite_timeout(void *arg)
{
	struct call *call = arg;

	(void)re_printf("%s: Local timeout after %u seconds\n",
			call->peer_uri, LOCAL_TIMEOUT);

	call_event_handler(call, CALL_EVENT_CLOSED, "Local timeout");
}


/** Called when all media streams are established */
static void mnat_handler(int err, uint16_t scode, const char *reason,
			 void *arg)
{
	struct call *call = arg;
	MAGIC_CHECK(call);

	if (err) {
		DEBUG_WARNING("medianat '%s' failed: %m\n",
			      call->acc->mnatid, err);
		call_event_handler(call, CALL_EVENT_CLOSED, "%m", err);
		return;
	}
	else if (scode) {
		DEBUG_WARNING("medianat failed: %u %s\n", scode, reason);
		call_event_handler(call, CALL_EVENT_CLOSED, "%u %s",
				   scode, reason);
		return;
	}

	/* Re-INVITE */
	if (!call->mnat_wait) {
		DEBUG_NOTICE("MNAT Established: Send Re-INVITE\n");
		(void)call_modify(call);
		return;
	}

	call->mnat_wait = false;

	switch (call->state) {

	case STATE_OUTGOING:
		(void)send_invite(call);
		break;

	case STATE_INCOMING:
		call_event_handler(call, CALL_EVENT_INCOMING, call->peer_uri);
		break;

	default:
		break;
	}
}


static int update_media(struct call *call, struct user_extra_data_t *user_data)
{
	const struct sdp_format *sc;
	struct le *le;
	int err = 0;

	/* media attributes */
	audio_sdp_attr_decode(call->audio);

#ifdef USE_VIDEO
	if (call->video)
		video_sdp_attr_decode(call->video);
#endif

	/* Update each stream */
	FOREACH_STREAM {
		stream_update(le->data, call->local_uri);
	}

	if (call->acc->mnat && call->acc->mnat->updateh && call->mnats)
		err = call->acc->mnat->updateh(call->mnats);

	sc = sdp_media_rformat(stream_sdpmedia(audio_strm(call->audio)), NULL);
	if (sc) {
		struct aucodec *ac = sc->data;
		if (ac) {
			err  = audio_decoder_set(call->audio, sc->data, sc->pt, sc->params, user_data);
			err |= audio_encoder_set(call->audio, sc->data, sc->pt, sc->params, user_data);
		}
		else {
			(void)re_printf("no common audio-codecs..\n");
		}
	}
	else {
		(void)re_printf("audio stream is disabled..\n");
	}

#ifdef USE_VIDEO
	sc = sdp_media_rformat(stream_sdpmedia(video_strm(call->video)), NULL);
	if (sc) {
		err = video_encoder_set(call->video, sc->data,
					sc->pt, sc->params);
		if (err) {
			DEBUG_WARNING("video stream: %m\n", err);
		}
	}
	else if (call->video) {
		(void)re_printf("video stream is disabled..\n");
	}
#endif

	return err;
}


static void print_summary(const struct call *call)
{
	uint32_t dur = call_duration(call);
	if (!dur)
		return;

	(void)re_printf("%s: Call with %s terminated (duration: %H)\n",
			 call->local_uri, call->peer_uri,
			 fmt_human_time, &dur);
}


static void call_destructor(void *arg)
{
	struct call *call = arg;

	if (call->state != STATE_IDLE)
		print_summary(call);

	call_stream_stop(call);
	list_unlink(&call->le);
	tmr_cancel(&call->tmr_dtmf);

	mem_deref(call->sess);
	mem_deref(call->local_uri);
	mem_deref(call->local_name);
	mem_deref(call->peer_uri);
	mem_deref(call->peer_name);
	mem_deref(call->alert_info);
	mem_deref(call->access_url);
	mem_deref(call->initial_rx_invite);
	mem_deref(call->audio);
#ifdef USE_VIDEO
	mem_deref(call->video);
	mem_deref(call->bfcp);
#endif
	mem_deref(call->sdp);
	mem_deref(call->mnats);
	mem_deref(call->mencs);
	mem_deref(call->sub);
	mem_deref(call->not);
	mem_deref(call->acc);
}


static void audio_event_handler(int key, bool end, void *arg)
{
	struct call *call = arg;
	MAGIC_CHECK(call);

	(void)re_printf("received event: '%c' (end=%d)\n", key, end);

	if (call->dtmfh)
		call->dtmfh(call, end ? 0x00 : key, call->arg);
}


static void audio_error_handler(int err, const char *str, void *arg)
{
	struct call *call = arg;
	MAGIC_CHECK(call);

	if (err) {
		DEBUG_WARNING("Audio error: %m (%s)\n", err, str);
	}

	call_stream_stop(call);
	call_event_handler(call, CALL_EVENT_CLOSED, str);
}


static void menc_error_handler(int err, void *arg)
{
	struct call *call = arg;
	MAGIC_CHECK(call);

	DEBUG_WARNING("mediaenc error: %m\n", err);

	call_stream_stop(call);
	call_event_handler(call, CALL_EVENT_CLOSED, "mediaenc failed");
}


static void stream_error_handler(struct stream *strm, int err, void *arg)
{
	struct call *call = arg;
	MAGIC_CHECK(call);

	DEBUG_INFO("call: error in \"%s\" rtp stream (%m)\n",
		sdp_media_name(stream_sdpmedia(strm)), err);

	call->scode = 701;
	set_state(call, STATE_TERMINATED);

	call_stream_stop(call);
	call_event_handler(call, CALL_EVENT_CLOSED, "rtp stream error");
}
/**
 * Allocate a new Call state object
 *
 * @param callp       Pointer to allocated Call state object
 * @param cfg         Global configuration
 * @param lst         List of call objects
 * @param local_name  Local display name (optional)
 * @param local_uri   Local SIP uri
 * @param acc         Account parameters
 * @param ua          User-Agent
 * @param prm         Call parameters
 * @param msg         SIP message for incoming calls
 * @param xcall       Optional call to inherit properties from
 * @param eh          Call event handler
 * @param arg         Handler argument
 *
 * @return 0 if success, otherwise errorcode
 */
int call_alloc(struct call **callp, const struct config *cfg, struct list *lst,
	       const char *local_name, const char *local_uri,
	       struct account *acc, struct ua *ua, const struct call_prm *prm,
	       const struct sip_msg *msg, struct call *xcall,
	       call_event_h *eh, void *arg)
{
	struct call *call;
	struct le *le;
	enum vidmode vidmode = prm ? prm->vidmode : VIDMODE_OFF;
	bool use_video = true, got_offer = false;
	int label = 0;
	int err = 0;

	if (!cfg || !local_uri || !acc || !ua)
		return EINVAL;

	call = mem_zalloc(sizeof(*call), call_destructor);
	if (!call)
		return ENOMEM;

	MAGIC_INIT(call);

	tmr_init(&call->tmr_inv);

	call->acc    = mem_ref(acc);
	call->ua     = ua;
	call->state  = STATE_IDLE;
	call->eh     = eh;
	call->arg    = arg;
	call->af     = prm ? prm->af : AF_INET;
	call->answer_after = -1;

	err = str_dup(&call->local_uri, local_uri);
	if (local_name)
		err |= str_dup(&call->local_name, local_name);
	if (err)
		goto out;

	/* Init SDP info */
	err = sdp_session_alloc(&call->sdp, net_laddr_af(call->af));
	if (err)
		goto out;

	err = sdp_session_set_lattr(call->sdp, true,
				    "tool", "baresip " BARESIP_VERSION);
	if (err)
		goto out;

	/* Check for incoming SDP Offer */
	if (msg && mbuf_get_left(msg->mb))
		got_offer = true;

	/* Initialise media NAT handling */
	if (acc->mnat) {
		err = acc->mnat->sessh(&call->mnats, net_dnsc(),
				       acc->stun_host, acc->stun_port,
				       acc->stun_user, acc->stun_pass,
				       call->sdp, !got_offer,
				       mnat_handler, call);
		if (err) {
			DEBUG_WARNING("mnat session: %m\n", err);
			goto out;
		}
	}
	call->mnat_wait = true;

	/* Media encryption */
	if (acc->menc) {
		if (acc->menc->sessh) {
			err = acc->menc->sessh(&call->mencs, call->sdp,
						!got_offer,
						menc_error_handler, call);
			if (err) {
				DEBUG_WARNING("mediaenc session: %m\n", err);
				goto out;
			}
		}
	}

	/* Audio stream */
	err = audio_alloc(&call->audio, cfg, call,
			  call->sdp, ++label,
			  acc->mnat, call->mnats, acc->menc, call->mencs,
			  acc->ptime, account_aucodecl(call->acc),
			  audio_event_handler, audio_error_handler, call);
	if (err)
		goto out;

#ifdef USE_VIDEO
	/* We require at least one video codec, and at least one
	   video source or video display */
	use_video = (vidmode != VIDMODE_OFF)
		&& (list_head(account_vidcodecl(call->acc)) != NULL)
		&& (NULL != vidsrc_find(NULL) || NULL != vidisp_find(NULL));

	/* Video stream */
	if (use_video) {
 		err = video_alloc(&call->video, cfg,
				  call, call->sdp, ++label,
				  acc->mnat, call->mnats,
				  acc->menc, call->mencs,
				  "main",
				  account_vidcodecl(call->acc));
		if (err)
			goto out;
 	}

	if (str_isset(cfg->bfcp.proto)) {

		err = bfcp_alloc(&call->bfcp, call->sdp,
				 cfg->bfcp.proto, !got_offer,
				 acc->mnat, call->mnats);
		if (err)
			goto out;
	}
#else
	(void)use_video;
	(void)vidmode;
#endif

	/* inherit certain properties from original call */
	if (xcall) {
		call->not = mem_ref(xcall->not);
	}

	FOREACH_STREAM {
		struct stream *strm = le->data;
		stream_set_error_handler(strm, stream_error_handler, call);
	}

	if (cfg->avt.rtp_timeout) {
		call_enable_rtp_timeout(call, cfg->avt.rtp_timeout*1000);
	}
	/* NOTE: The new call must always be added to the tail of list,
	 *       which indicates the current call.
	 */
	list_append(lst, &call->le, call);

 out:
	if (err)
		mem_deref(call);
	else if (callp)
		*callp = call;

	return err;
}


int call_connect(struct call *call, const struct pl *paddr)
{
	struct sip_addr addr;
	int err;

	if (!call || !paddr)
		return EINVAL;

	(void)re_printf("connecting to '%r'..\n", paddr);

	if (0 == sip_addr_decode(&addr, paddr)) {
		err = pl_strdup(&call->peer_uri, &addr.auri);
	}
	else {
		err = pl_strdup(&call->peer_uri, paddr);
	}
	if (err)
		return err;

	set_state(call, STATE_OUTGOING);
	call_event_handler(call, CALL_EVENT_OUTGOING, call->peer_uri);	

	/* If we are using asyncronous medianat like STUN/TURN, then
	 * wait until completed before sending the INVITE */
	if (!call->acc->mnat)
		err = send_invite(call);

	return err;
}


/**
 * Update the current call by sending Re-INVITE or UPDATE
 *
 * @param call Call object
 *
 * @return 0 if success, otherwise errorcode
 */
int call_modify(struct call *call)
{
	struct mbuf *desc;
	int err;

	if (!call)
		return EINVAL;

	err = call_sdp_get(call, &desc, true);
	if (!err)
		err = sipsess_modify(call->sess, desc);

	mem_deref(desc);

	return err;
}


int call_hangup(struct call *call, uint16_t scode, const char *reason)
{
	int err = 0;

	if (!call)
		return EINVAL;

	switch (call->state) {

	case STATE_INCOMING:
		if (scode < 400) {
			scode = 486;
			reason = "Rejected";
		}
		(void)re_printf("rejecting incoming call from %s (%u %s)\n",
				call->peer_uri, scode, reason);
		(void)sipsess_reject(call->sess, scode, reason, NULL);
		break;

	default:
		(void)re_printf("terminate call with %s\n", call->peer_uri);
		call->sess = mem_deref(call->sess);
		break;
	}

	set_state(call, STATE_TERMINATED);

	call_stream_stop(call);

	return err;
}


int call_progress(struct call *call)
{
	struct mbuf *desc;
	int err;

	if (!call)
		return EINVAL;

	err = call_sdp_get(call, &desc, false);
	if (err)
		return err;

	err = sipsess_progress(call->sess, 183, "Session Progress",
			       desc, "Allow: %s\r\n", uag_allowed_methods());

	if (!err)
		call_stream_start(call, false, NULL);

	mem_deref(desc);

	return 0;
}


int call_answer(struct call *call, uint16_t scode, const char *audio_mod, const char *audio_dev, struct user_extra_data_t *user_data)
{
	struct mbuf *desc;
	int err;
	
	if (!call || !call->sess)
		return EINVAL;

	if (STATE_INCOMING != call->state) {
		return 0;
	}

	(void)re_printf("answering call from %s with %u\n",
			call->peer_uri, scode);

	if (audio_mod && audio_dev && audio_mod[0] != '\0') {
		audio_set_rx_device(call->audio, audio_mod, audio_dev);
	}

	if (call->got_offer) {

		err = update_media(call, user_data);
		if (err)
			return err;
	}

	err = sdp_encode(&desc, call->sdp, !call->got_offer);
	if (err)
		return err;

	err = sipsess_answer(call->sess, scode, "Answering", desc,
                             "Allow: %s\r\n", uag_allowed_methods());

	mem_deref(desc);

	return err;
}


/**
 * Check if the current call has an active audio stream
 *
 * @param call  Call object
 *
 * @return True if active stream, otherwise false
 */
bool call_has_audio(const struct call *call)
{
	if (!call)
		return false;

	return sdp_media_has_media(stream_sdpmedia(audio_strm(call->audio)));
}


/**
 * Check if the current call has an active video stream
 *
 * @param call  Call object
 *
 * @return True if active stream, otherwise false
 */
bool call_has_video(const struct call *call)
{
	if (!call)
		return false;

#ifdef USE_VIDEO
	return sdp_media_has_media(stream_sdpmedia(video_strm(call->video)));
#else
	return false;
#endif
}


/**
 * Put the current call on hold/resume
 *
 * @param call  Call object
 * @param hold  True to hold, false to resume
 *
 * @return 0 if success, otherwise errorcode
 */
int call_hold(struct call *call, bool hold)
{
	struct le *le;

	if (!call || !call->sess)
		return EINVAL;

	(void)re_printf("%s %s\n", hold ? "hold" : "resume", call->peer_uri);

	FOREACH_STREAM
		stream_hold(le->data, hold);

	return call_modify(call);
}


int call_sdp_get(const struct call *call, struct mbuf **descp, bool offer)
{
	return sdp_encode(descp, call->sdp, offer);
}


const char *call_peeruri(const struct call *call)
{
	return call ? call->peer_uri : NULL;
}


const char *call_localuri(const struct call *call)
{
	return call ? call->local_uri : NULL;
}


/**
 * Get the name of the peer
 *
 * @param call  Call object
 *
 * @return Peer name
 */
const char *call_peername(const struct call *call)
{
	return call ? call->peer_name : NULL;
}

const char *call_alert_info(const struct call *call)
{
	return call ? call->alert_info : NULL;
}

const char   *call_access_url(const struct call *call)
{
	return call ? call->access_url : NULL;
}

const char   *call_initial_rx_invite(const struct call *call)
{
    return call ? call->initial_rx_invite : NULL;
}

int call_access_url_mode(const struct call *call)
{
    return call ? call->access_url_mode : 0;
}

int call_debug(struct re_printf *pf, const struct call *call)
{
	int err;

	if (!call)
		return 0;

	err = re_hprintf(pf, "===== Call debug (%s) =====\n",
			 state_name(call->state));

	/* SIP Session debug */
	err |= re_hprintf(pf,
			  " local_uri: %s <%s>\n"
			  " peer_uri:  %s <%s>\n"
			  " af=%s\n",
			  call->local_name, call->local_uri,
			  call->peer_name, call->peer_uri,
			  net_af2name(call->af));

	/* SDP debug */
	err |= sdp_session_debug(pf, call->sdp);

	return err;
}


static int print_duration(struct re_printf *pf, const struct call *call)
{
	const uint32_t dur = call_duration(call);
	const uint32_t sec = dur%60%60;
	const uint32_t min = dur/60%60;
	const uint32_t hrs = dur/60/60;

	return re_hprintf(pf, "%u:%02u:%02u", hrs, min, sec);
}


int call_status(struct re_printf *pf, const struct call *call)
{
	struct le *le;
	int err;

	if (!call)
		return EINVAL;

	switch (call->state) {

	case STATE_EARLY:
	case STATE_ESTABLISHED:
		break;
	default:
		return 0;
	}

	err = re_hprintf(pf, "\r[%H]", print_duration, call);

	FOREACH_STREAM
		err |= stream_print(pf, le->data);

	err |= re_hprintf(pf, " (bit/s)");

#ifdef USE_VIDEO
	if (call->video)
		err |= video_print(pf, call->video);
#endif

	return err;
}


int call_jbuf_stat(struct re_printf *pf, const struct call *call)
{
	struct le *le;
	int err = 0;

	if (!call)
		return EINVAL;

	FOREACH_STREAM
		err |= stream_jbuf_stat(pf, le->data);

	return err;
}


int call_info(struct re_printf *pf, const struct call *call)
{
	if (!call)
		return 0;

	return re_hprintf(pf, "%H  %8s  %s", print_duration, call,
			  state_name(call->state), call->peer_uri);
}


/**
 * Send a DTMF digit to the peer
 *
 * @param call  Call object
 * @param key   DTMF digit to send (0x00 for key release)
 *
 * @return 0 if success, otherwise errorcode
 */
int call_send_digit(struct call *call, char key)
{
	if (!call)
		return EINVAL;

	return audio_send_digit(call->audio, key);
}


struct ua *call_get_ua(const struct call *call)
{
	return call ? call->ua : NULL;
}


static int auth_handler(char **username, char **password,
			const char *realm, void *arg)
{
	struct account *acc = arg;
	return account_auth(acc, username, password, realm);
}


static int sipsess_offer_handler(struct mbuf **descp,
				 const struct sip_msg *msg, void *arg)
{
	const bool got_offer = mbuf_get_left(msg->mb);
	struct call *call = arg;
	int err;

	MAGIC_CHECK(call);

	DEBUG_NOTICE("got re-INVITE%s\n", got_offer ? " (SDP Offer)" : "");

	if (got_offer) {

		/* Decode SDP Offer */
		err = sdp_decode(call->sdp, msg->mb, true);
		if (err)
			return err;

		err = update_media(call, NULL);
		if (err)
			return err;
	}

	/* Encode SDP Answer */
	return sdp_encode(descp, call->sdp, !got_offer);
}


static int sipsess_answer_handler(const struct sip_msg *msg, void *arg, void *user_data)
{
	struct call *call = arg;
	int err;

	MAGIC_CHECK(call);

	(void)sdp_decode_multipart(&msg->ctype, msg->mb);

	err = sdp_decode(call->sdp, msg->mb, false);
	if (err) {
		DEBUG_WARNING("answer: sdp_decode: %m\n", err);
		return err;
	}

	err = update_media(call, (struct user_extra_data_t *)user_data);
	if (err)
		return err;

	return 0;
}


static void sipsess_estab_handler(const struct sip_msg *msg, void *arg)
{
	struct call *call = arg;

	MAGIC_CHECK(call);

	(void)msg;

	if (call->state == STATE_ESTABLISHED)
		return;

	set_state(call, STATE_ESTABLISHED);

	call_stream_start(call, true, NULL);

	if (call->rtp_timeout_ms) {

		struct le *le;

		FOREACH_STREAM {
			struct stream *strm = le->data;
			stream_enable_rtp_timeout(strm, call->rtp_timeout_ms);
		}
	}

	/* the transferor will hangup this call */
	if (call->not) {
		(void)call_notify_sipfrag(call, 200, "OK");
	}

	/* must be done last, the handler might deref this call */
	call_event_handler(call, CALL_EVENT_ESTABLISHED, call->peer_uri);
}


#ifdef USE_VIDEO
static void call_handle_info_req(struct call *call, const struct sip_msg *req)
{
	struct pl body;
	bool pfu;
	int err;

	(void)call;

	pl_set_mbuf(&body, req->mb);

	err = mctrl_handle_media_control(&body, &pfu);
	if (err)
		return;

	if (pfu) {
		video_update_picture(call->video);
	}
}
#endif


static void dtmfend_handler(void *arg)
{
	struct call *call = arg;

	if (call->dtmfh)
		call->dtmfh(call, 0x00, call->arg);
}


static void sipsess_info_handler(struct sip *sip, const struct sip_msg *msg,
				 void *arg)
{
	struct call *call = arg;

	if (!pl_strcasecmp(&msg->ctype, "application/dtmf-relay")) {

		struct pl body, sig, dur;
		int err;

		pl_set_mbuf(&body, msg->mb);

		err  = re_regex(body.p, body.l, "Signal=[0-9]+", &sig);
		err |= re_regex(body.p, body.l, "Duration=[0-9]+", &dur);

		if (err) {
			(void)sip_reply(sip, msg, 400, "Bad Request");
		}
		else {
			char s = pl_u32(&sig);
			uint32_t duration = pl_u32(&dur);

			if (s == 10) s = '*';
			else if (s == 11) s = '#';
			else s += '0';

			(void)re_printf("received DTMF: '%c' (duration=%r)\n",
					s, &dur);

			(void)sip_reply(sip, msg, 200, "OK");

			if (call->dtmfh) {
				tmr_start(&call->tmr_dtmf, duration,
					  dtmfend_handler, call);
				call->dtmfh(call, s, call->arg);
			}
		}
	}
#ifdef USE_VIDEO
	else if (!pl_strcasecmp(&msg->ctype,
				"application/media_control+xml")) {
		call_handle_info_req(call, msg);
		(void)sip_reply(sip, msg, 200, "OK");
	}
#endif
	else {
		(void)sip_reply(sip, msg, 488, "Not Acceptable Here");
	}
}


static void sipnot_close_handler(int err, const struct sip_msg *msg,
				 void *arg)
{
	struct call *call = arg;

	if (err)
		(void)re_printf("notification closed: %m\n", err);
	else if (msg)
		(void)re_printf("notification closed: %u %r\n",
				msg->scode, &msg->reason);

	call->not = mem_deref(call->not);
}


static void sipsess_refer_handler(struct sip *sip, const struct sip_msg *msg,
				  void *arg)
{
	struct call *call = arg;
	const struct sip_hdr *hdr;
	int err;

	/* get the transfer target */
	hdr = sip_msg_hdr(msg, SIP_HDR_REFER_TO);
	if (!hdr) {
		DEBUG_WARNING("bad REFER request from %r\n", &msg->from.auri);
		(void)sip_reply(sip, msg, 400, "Missing Refer-To header");
		return;
	}

	/* The REFER creates an implicit subscription.
	 * Reply 202 to the REFER request
	 */
	call->not = mem_deref(call->not);
	err = sipevent_accept(&call->not, uag_sipevent_sock(), msg,
			      sipsess_dialog(call->sess), NULL,
			      202, "Accepted", 60, 60, 60,
			      ua_cuser(call->ua), "message/sipfrag",
			      auth_handler, call->acc, true,
			      sipnot_close_handler, call,
	                      "Allow: %s\r\n", uag_allowed_methods());
	if (err) {
		DEBUG_WARNING("refer: sipevent_accept failed: %m\n", err);
		return;
	}

	(void)call_notify_sipfrag(call, 100, "Trying");

	call_event_handler(call, CALL_EVENT_TRANSFER, "%r", &hdr->val);
}


static void sipsess_close_handler(int err, const struct sip_msg *msg,
				  void *arg)
{
	struct call *call = arg;
	char reason[128] = "";

	MAGIC_CHECK(call);

	if (err) {
		(void)re_printf("%s: session closed: %m\n",
				call->peer_uri, err);

		if (call->not) {
			(void)call_notify_sipfrag(call, 500, "%m", err);
		}
	}
	else if (msg) {

		call->scode = msg->scode;

		(void)re_snprintf(reason, sizeof(reason), "%u %r",
				  msg->scode, &msg->reason);

		(void)re_printf("%s: session closed: %u %r\n",
				call->peer_uri, msg->scode, &msg->reason);

		if (call->not) {
			(void)call_notify_sipfrag(call, msg->scode,
						  "%r", &msg->reason);
		}
	}
	else {
		(void)re_printf("%s: session closed\n", call->peer_uri);
	}

	call_stream_stop(call);
	call_event_handler(call, CALL_EVENT_CLOSED, reason);
}


int call_accept(struct call *call, struct sipsess_sock *sess_sock,
		const struct sip_msg *msg)
{
	const struct sip_hdr * hdr_call_info;
	bool got_offer;
	int err;

	if (!call || !msg)
		return EINVAL;

	got_offer = (mbuf_get_left(msg->mb) > 0);

	err = pl_strdup(&call->peer_uri, &msg->from.auri);
	if (err)
		return err;

	if (pl_isset(&msg->from.dname)) {
		err = pl_strdup(&call->peer_name, &msg->from.dname);
		if (err)
			return err;
	}

	call->answer_after = msg->call_info.answer_after;

	if (msg->alert_info.info.l > 0) {
		err = pl_strdup(&call->alert_info, &msg->alert_info.info);
	} else {
    	call->alert_info = NULL;
	}
	if (err)
		return err;

	if (msg->access_url.url.l > 0) {
		err = pl_strdup(&call->access_url, &msg->access_url.url);
    } else {
		call->access_url = NULL;
	}
	if (err)
		return err;
	call->access_url_mode = msg->access_url.mode;

	if (got_offer) {

		err = sdp_decode(call->sdp, msg->mb, true);
		if (err)
			return err;

		call->got_offer = true;
	}

	err = sipsess_accept(&call->sess, sess_sock, msg, 180, "Ringing",
			     ua_cuser(call->ua), "application/sdp", NULL,
			     auth_handler, call->acc, true,
			     sipsess_offer_handler, sipsess_answer_handler,
			     sipsess_estab_handler, sipsess_info_handler,
			     sipsess_refer_handler, sipsess_close_handler,
			     call, "Allow: %s\r\n", uag_allowed_methods());
	if (err) {
		DEBUG_WARNING("sipsess_accept: %m\n", err);
		return err;
	}

	// clone initial INVITE message to allow manual processing (extracting custom header lines from script) later
	if (call->initial_rx_invite == NULL) {
		call->initial_rx_invite = mem_alloc(msg->mb_msg.size+1, NULL);
		if (!call->initial_rx_invite)
			return ENOMEM;
		memcpy(call->initial_rx_invite, mbuf_buf(&msg->mb_msg), msg->mb_msg.size);
		call->initial_rx_invite[msg->mb_msg.size] = '\0';
    }

	set_state(call, STATE_INCOMING);

	/* New call */
	tmr_start(&call->tmr_inv, LOCAL_TIMEOUT*1000, invite_timeout, call);

	if (!call->acc->mnat)
		call_event_handler(call, CALL_EVENT_INCOMING, call->peer_uri);

	return err;
}


static void sipsess_progr_handler(const struct sip_msg *msg, void *arg, void *user_data)
{
	struct call *call = arg;
	bool media;

	MAGIC_CHECK(call);

	(void)re_printf("SIP Progress: %u %r (%r)\n",
			msg->scode, &msg->reason, &msg->ctype);

	if (msg->scode == 100) {
		call_event_handler(call, CALL_EVENT_TRYING, call->peer_uri);
	}

	if (msg->scode <= 100)
		return;

	/* check for 18x and content-type
	 *
	 * 1. start media-stream if application/sdp
	 * 2. play local ringback tone if not
	 *
	 * we must also handle changes to/from 180 and 183,
	 * so we reset the media-stream/ringback each time.
	 */
	if (!pl_strcasecmp(&msg->ctype, "application/sdp")
	    && mbuf_get_left(msg->mb)
	    && !sdp_decode(call->sdp, msg->mb, false)) {
		media = true;
	}
	else if (!sdp_decode_multipart(&msg->ctype, msg->mb) &&
		 !sdp_decode(call->sdp, msg->mb, false)) {
		media = true;
	}
	else
		media = false;

	switch (msg->scode) {

	case 180:
		set_state(call, STATE_RINGING);
		break;

	case 183:
		set_state(call, STATE_EARLY);
		break;
	}

	if (media)
		call_event_handler(call, CALL_EVENT_PROGRESS, call->peer_uri);
	else
		call_event_handler(call, CALL_EVENT_RINGING, call->peer_uri);

	call_stream_stop(call);

	if (media)
		call_stream_start(call, false, user_data);
}


static int send_invite(struct call *call)
{
	const char *routev[1];
	struct mbuf *desc;
	int err;

	routev[0] = ua_outbound(call->ua);

	err = call_sdp_get(call, &desc, true);
	if (err)
		return err;

	err = sipsess_connect(&call->sess, uag_sipsess_sock(),
			      call->peer_uri,
			      call->local_name,
			      call->local_uri,
			      ua_cuser(call->ua),
			      routev[0] ? routev : NULL,
			      routev[0] ? 1 : 0,
			      "application/sdp", desc,
			      auth_handler, call->acc, true,
			      sipsess_offer_handler, sipsess_answer_handler,
			      sipsess_progr_handler, sipsess_estab_handler,
			      sipsess_info_handler, sipsess_refer_handler,
			      sipsess_close_handler, call,
			      "Allow: %s\r\n%H", uag_allowed_methods(),
			      ua_print_supported, call->ua);
	if (err) {
		DEBUG_WARNING("sipsess_connect: %m\n", err);
	}

	mem_deref(desc);

	return err;
}


/**
 * Get the current call duration in seconds
 *
 * @param call  Call object
 *
 * @return Duration in seconds
 */
uint32_t call_duration(const struct call *call)
{
	if (!call || !call->time_start)
		return 0;

	return (uint32_t)(time(NULL) - call->time_start);
}


/**
 * Get the audio object for the current call
 *
 * @param call  Call object
 *
 * @return Audio object
 */
struct audio *call_audio(const struct call *call)
{
	return call ? call->audio : NULL;
}


/**
 * Get the video object for the current call
 *
 * @param call  Call object
 *
 * @return Video object
 */
struct video *call_video(const struct call *call)
{
#ifdef USE_VIDEO
	return call ? call->video : NULL;
#else
	(void)call;
	return NULL;
#endif
}


/**
 * Get the list of media streams for the current call
 *
 * @param call  Call object
 *
 * @return List of media streams
 */
struct list *call_streaml(const struct call *call)
{
	return call ? (struct list *)&call->streaml : NULL;
}


int call_reset_transp(struct call *call)
{
	if (!call)
		return EINVAL;

	sdp_session_set_laddr(call->sdp, net_laddr_af(call->af));

	return call_modify(call);
}


int call_notify_sipfrag(struct call *call, uint16_t scode,
			const char *reason, ...)
{
	struct mbuf *mb;
	va_list ap;
	int err;

	if (!call)
		return EINVAL;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	va_start(ap, reason);
	(void)mbuf_printf(mb, "SIP/2.0 %u %v\n", scode, reason, &ap);
	va_end(ap);

	mb->pos = 0;

	if (scode >= 200) {
		err = sipevent_notify(call->not, mb, SIPEVENT_TERMINATED,
				      SIPEVENT_NORESOURCE, 0);

		call->not = mem_deref(call->not);
	}
	else {
		err = sipevent_notify(call->not, mb, SIPEVENT_ACTIVE,
				      SIPEVENT_NORESOURCE, 0);
	}

	mem_deref(mb);

	return err;
}


static void sipsub_notify_handler(struct sip *sip, const struct sip_msg *msg,
				  void *arg)
{
	struct call *call = arg;
	struct pl scode, reason;
	uint32_t sc;

	if (re_regex((char *)mbuf_buf(msg->mb), mbuf_get_left(msg->mb),
		     "SIP/2.0 [0-9]+ [^\r\n]+", &scode, &reason)) {
		(void)sip_reply(sip, msg, 400, "Bad sipfrag");
		return;
	}

	(void)sip_reply(sip, msg, 200, "OK");

	sc = pl_u32(&scode);

	if (sc >= 300) {
		DEBUG_WARNING("call transfer failed: %u %r\n", sc, &reason);
	}
	else if (sc >= 200) {
		call_event_handler(call, CALL_EVENT_CLOSED, "Call transfered");
	}
}


static void sipsub_close_handler(int err, const struct sip_msg *msg,
				 const struct sipevent_substate *substate,
				 void *arg)
{
	struct call *call = arg;

	(void)substate;

	call->sub = mem_deref(call->sub);

	if (err) {
		(void)re_printf("subscription closed: %m\n", err);
	}
	else if (msg && msg->scode >= 300) {
		(void)re_printf("call transfer failed: %u %r\n",
				msg->scode, &msg->reason);
	}
}


static int normalize_uri(char **out, const char *uri, const struct uri *luri)
{
	struct uri uri2;
	struct pl pl;
	int err;

	if (!out || !uri || !luri)
		return EINVAL;

	pl_set_str(&pl, uri);

	if (0 == uri_decode(&uri2, &pl)) {

		err = str_dup(out, uri);
	}
	else {
		uri2 = *luri;

		uri2.user     = pl;
		uri2.password = pl_null;
		uri2.params   = pl_null;

		err = re_sdprintf(out, "%H", uri_encode, &uri2);
	}

	return err;
}


/**
 * Transfer the call to a target SIP uri
 *
 * @param call  Call object
 * @param uri   Target SIP uri
 *
 * @return 0 if success, otherwise errorcode
 */
int call_transfer(struct call *call, const char *uri)
{
	char *nuri;
	int err;

	if (!call || !uri)
		return EINVAL;

	err = normalize_uri(&nuri, uri, &call->acc->luri);
	if (err)
		return err;

	(void)re_printf("transferring call to %s\n", nuri);

	call->sub = mem_deref(call->sub);
	err = sipevent_drefer(&call->sub, uag_sipevent_sock(),
			      sipsess_dialog(call->sess), ua_cuser(call->ua),
			      auth_handler, call->acc, true,
			      sipsub_notify_handler, sipsub_close_handler,
			      call,
			      "Refer-To: %s\r\n", nuri);
	if (err) {
		DEBUG_WARNING("sipevent_drefer: %m\n", err);
	}

	mem_deref(nuri);

	return err;
}


int call_af(const struct call *call)
{
	return call ? call->af : AF_UNSPEC;
}


uint16_t call_scode(const struct call *call)
{
	return call ? call->scode : 0;
}

int call_answer_after(const struct call *call)
{
	return call ? call->answer_after : -1;
}

void call_set_handlers(struct call *call, call_event_h *eh,
		       call_dtmf_h *dtmfh, void *arg)
{
	if (!call)
		return;

	if (eh)
		call->eh    = eh;

	if (dtmfh)
		call->dtmfh = dtmfh;

	if (arg)
		call->arg   = arg;
}

void call_enable_rtp_timeout(struct call *call, uint32_t timeout_ms)
{
	if (!call)
		return;

	call->rtp_timeout_ms = timeout_ms;
}
