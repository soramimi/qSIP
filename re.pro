win32:Release:TARGET = re
win32:Debug:TARGET = red
unix:TARGET = re
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$PWD/re/include

QMAKE_CFLAGS += -DHAVE_SELECT
unix:QMAKE_CFLAGS += -DHAVE_GETIFADDRS -DHAVE_SIGNAL

DESTDIR = $$PWD/_build

SOURCES += \
        re/src/base64/b64.c \
        re/src/bfcp/attr.c \
        re/src/bfcp/msg.c \
        re/src/conf/conf.c \
        re/src/crc32/crc32.c \
        re/src/dbg/dbg.c \
        re/src/dns/client.c \
        re/src/dns/cstr.c \
        re/src/dns/dname.c \
        re/src/dns/rrlist.c \
        re/src/dns/ns.c \
        re/src/dns/rr.c \
        re/src/dns/dns_hdr.c \
        re/src/fmt/time.c \
        re/src/fmt/ch.c \
        re/src/fmt/hexdump.c \
        re/src/fmt/pl.c \
        re/src/fmt/print.c \
        re/src/fmt/prm.c \
        re/src/fmt/regex.c \
        re/src/fmt/str.c \
        re/src/hash/hash.c \
        re/src/hash/func.c \
        re/src/hmac/hmac_sha1.c \
        re/src/httpauth/digest.c \
        re/src/httpauth/basic.c \
        re/src/ice/util.c \
        re/src/ice/cand.c \
        re/src/ice/candpair.c \
        re/src/ice/chklist.c \
        re/src/ice/comp.c \
        re/src/ice/connchk.c \
        re/src/ice/gather.c \
        re/src/ice/ice.c \
        re/src/ice/icem.c \
        re/src/ice/icesdp.c \
        re/src/ice/icestr.c \
        re/src/ice/stunsrv.c \
        re/src/jbuf/jbuf.c \
        re/src/list/list.c \
        re/src/main/method.c \
        re/src/main/init.c \
        re/src/main/main.c \
        re/src/mbuf/mbuf.c \
        re/src/md5/wrap.c \
        re/src/md5/md5.c \
        re/src/mem/mem.c \
        re/src/natbd/natstr.c \
        re/src/natbd/filtering.c \
        re/src/natbd/genalg.c \
        re/src/natbd/hairpinning.c \
        re/src/natbd/lifetime.c \
        re/src/natbd/mapping.c \
        re/src/net/sockopt.c \
        re/src/net/if.c \
        re/src/net/net.c \
        re/src/net/netstr.c \
        re/src/net/rt.c \
        re/src/rtp/source.c \
        re/src/rtp/fb.c \
        re/src/rtp/member.c \
        re/src/rtp/ntp.c \
        re/src/rtp/pkt.c \
        re/src/rtp/rtcp.c \
        re/src/rtp/rtp.c \
        re/src/rtp/rtp_rr.c \
        re/src/rtp/sdes.c \
        re/src/rtp/sess.c \
        re/src/sa/sa.c \
        re/src/sa/ntop.c \
        re/src/sa/pton.c \
        re/src/sdp/session.c \
        re/src/sdp/format.c \
        re/src/sdp/media.c \
        re/src/sdp/sdp_attr.c \
        re/src/sdp/sdp_msg.c \
        re/src/sdp/sdp_str.c \
        re/src/sha/sha1.c \
        re/src/sip/via.c \
        re/src/sip/addr.c \
        re/src/sip/auth.c \
        re/src/sip/cseq.c \
        re/src/sip/ctrans.c \
        re/src/sip/dialog.c \
        re/src/sip/keepalive.c \
        re/src/sip/keepalive_udp.c \
        re/src/sip/param.c \
        re/src/sip/reply.c \
        re/src/sip/request.c \
        re/src/sip/sip.c \
        re/src/sip/sip_msg.c \
        re/src/sip/sip_transp.c \
        re/src/sip/strans.c \
        re/src/sipreg/reg.c \
        re/src/sipsess/sipsess_request.c \
        re/src/sipsess/accept.c \
        re/src/sipsess/ack.c \
        re/src/sipsess/close.c \
        re/src/sipsess/connect.c \
        re/src/sipsess/info.c \
        re/src/sipsess/listen.c \
        re/src/sipsess/modify.c \
        re/src/sipsess/sip_sess.c \
        re/src/sipsess/sipsess_reply.c \
        re/src/stun/stun.c \
        re/src/stun/stun_addr.c \
        re/src/stun/stun_attr.c \
        re/src/stun/stun_hdr.c \
        re/src/stun/stun_ind.c \
        re/src/stun/stun_msg.c \
        re/src/stun/stun_rep.c \
        re/src/stun/stun_req.c \
        re/src/sys/sys.c \
        re/src/sys/daemon.c \
        re/src/sys/endian.c \
        re/src/sys/rand.c \
        re/src/tcp/tcp_high.c \
        re/src/tcp/tcp.c \
        re/src/telev/telev.c \
        re/src/tmr/tmr.c \
        re/src/udp/udp.c \
        re/src/uri/uric.c \
        re/src/uri/ucmp.c \
        re/src/uri/uri.c \
        re/src/mod/mod.c \
        re/src/net/net_sock.c \
        re/src/sys/sleep.c \
        re/src/sipevent/subscribe.c \
        re/src/sipevent/notify.c \
        re/src/sipevent/sipevent_listen.c \
        re/src/sipevent/sipevent_msg.c \
        re/src/bfcp/conn.c \
        re/src/bfcp/bfcp_request.c \
        re/src/bfcp/bfcp_reply.c \
        re/src/fmt/str_error.c \
        re/src/sa/printaddr.c \
        re/src/stun/stunstr.c \
        re/src/stun/stun_keepalive.c \
        re/src/stun/stun_ctrans.c \
        re/src/stun/dnsdisc.c \
        re/src/sip/sip_call_info.c \
        re/src/sip/sip_alert_info.c \
        re/src/sys/sys_time.c \
        re/src/sip/sip_access_url.c

win32:SOURCES += \
		re/src/dns/win32/srv.c \
		re/src/mqueue/win32/pipe.c \
		re/src/net/win32/wif.c \
		re/src/mod/win32/dll.c \
		re/src/lock/win32/win32_lock.c

unix:SOURCES += \
		re/src/lock/lock.c \
		re/src/lock/rwlock.c \
		re/src/net/ifaddrs.c \
		re/src/net/posix/pif.c \
		re/src/mod/dl.c



HEADERS += \
        re/include/re_uri.h \
        re/include/re.h \
        re/include/re_base64.h \
        re/include/re_bfcp.h \
        re/include/re_bitv.h \
        re/include/re_conf.h \
        re/include/re_crc32.h \
        re/include/re_dbg.h \
        re/include/re_dns.h \
        re/include/re_fmt.h \
        re/include/re_hash.h \
        re/include/re_hmac.h \
        re/include/re_httpauth.h \
        re/include/re_ice.h \
        re/include/re_jbuf.h \
        re/include/re_list.h \
        re/include/re_lock.h \
        re/include/re_main.h \
        re/include/re_mbuf.h \
        re/include/re_md5.h \
        re/include/re_mem.h \
        re/include/re_mod.h \
        re/include/re_mqueue.h \
        re/include/re_natbd.h \
        re/include/re_net.h \
        re/include/re_rtp.h \
        re/include/re_sa.h \
        re/include/re_sdp.h \
        re/include/re_sha.h \
        re/include/re_sip.h \
        re/include/re_sipevent.h \
        re/include/re_sipreg.h \
        re/include/re_sipsess.h \
        re/include/re_stun.h \
        re/include/re_sys.h \
        re/include/re_tcp.h \
        re/include/re_telev.h \
        re/include/re_tls.h \
        re/include/re_tmr.h \
        re/include/re_turn.h \
        re/include/re_types.h \
        re/include/re_udp.h \
        re/src/sip/sip.h \
        re/src/sdp/sdp.h

