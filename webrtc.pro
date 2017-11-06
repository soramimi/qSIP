win32:Release:TARGET = webrtc
win32:Debug:TARGET = webrtcd
unix:TARGET = webrtc
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$PWD/webrtc

DESTDIR = $$PWD/_build

SOURCES += \
		webrtc/webrtc/modules/audio_processing/aec/echo_cancellation.c \
		webrtc/webrtc/modules/audio_processing/aec/aec_core.c \
		webrtc/webrtc/modules/audio_processing/aec/aec_rdft.c \
		webrtc/webrtc/modules/audio_processing/aec/aec_resampler.c \
		webrtc/webrtc/modules/audio_processing/utility/ring_buffer.c \
		webrtc/webrtc/modules/audio_processing/utility/delay_estimator_wrapper.c \
		webrtc/webrtc/modules/audio_processing/utility/delay_estimator.c \
		webrtc/webrtc/common_audio/signal_processing/randomization_functions.c \
		webrtc/webrtc/modules/audio_processing/ns/nsx_core.c \
		webrtc/webrtc/modules/audio_processing/ns/noise_suppression.c \
		webrtc/webrtc/modules/audio_processing/ns/noise_suppression_x.c \
		webrtc/webrtc/modules/audio_processing/ns/ns_core.c \
		webrtc/webrtc/common_audio/signal_processing/spl_init.c \
		webrtc/webrtc/common_audio/signal_processing/min_max_operations.c \
		webrtc/webrtc/common_audio/signal_processing/cross_correlation.c \
		webrtc/webrtc/common_audio/signal_processing/downsample_fast.c \
		webrtc/webrtc/common_audio/signal_processing/energy.c \
		webrtc/webrtc/common_audio/signal_processing/vector_scaling_operations.c \
		webrtc/webrtc/common_audio/signal_processing/real_fft.c \
		webrtc/webrtc/common_audio/signal_processing/complex_fft.c \
		webrtc/webrtc/common_audio/signal_processing/complex_bit_reverse.c \
		webrtc/webrtc/common_audio/signal_processing/copy_set_operations.c \
		webrtc/webrtc/common_audio/signal_processing/spl_sqrt_floor.c \
		webrtc/webrtc/common_audio/signal_processing/get_scaling_square.c

HEADERS += \
		webrtc/webrtc/modules/audio_processing/aec/include/echo_cancellation.h \
		webrtc/webrtc/modules/audio_processing/aec/aec_core.h \
		webrtc/webrtc/modules/audio_processing/aec/aec_core_internal.h \
		webrtc/webrtc/modules/audio_processing/aec/aec_rdft.h \
		webrtc/webrtc/modules/audio_processing/aec/aec_resampler.h \
		webrtc/webrtc/modules/audio_processing/aec/echo_cancellation_internal.h \
		webrtc/webrtc/modules/audio_processing/include/audio_processing.h \
		webrtc/webrtc/modules/audio_processing/include/mock_audio_processing.h \
		webrtc/webrtc/modules/audio_processing/audio_buffer.h \
		webrtc/webrtc/modules/audio_processing/audio_processing_impl.h \
		webrtc/webrtc/modules/audio_processing/echo_cancellation_impl.h \
		webrtc/webrtc/modules/audio_processing/echo_control_mobile_impl.h \
		webrtc/webrtc/modules/audio_processing/gain_control_impl.h \
		webrtc/webrtc/modules/audio_processing/high_pass_filter_impl.h \
		webrtc/webrtc/modules/audio_processing/level_estimator_impl.h \
		webrtc/webrtc/modules/audio_processing/noise_suppression_impl.h \
		webrtc/webrtc/modules/audio_processing/processing_component.h \
		webrtc/webrtc/modules/audio_processing/splitting_filter.h \
		webrtc/webrtc/modules/audio_processing/voice_detection_impl.h
