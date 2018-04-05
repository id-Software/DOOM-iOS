LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES = \
	lib_src/eas_chorus.c \
	lib_src/eas_chorusdata.c \
	lib_src/eas_data.c \
	lib_src/eas_fmengine.c \
	lib_src/eas_fmsndlib.c \
	lib_src/eas_fmsynth.c \
	lib_src/eas_fmtables.c \
	lib_src/eas_ima_tables.c \
	lib_src/eas_imaadpcm.c \
	lib_src/eas_imelody.c \
	lib_src/eas_imelodydata.c \
	lib_src/eas_math.c \
	lib_src/eas_midi.c \
	lib_src/eas_mididata.c \
	lib_src/eas_mixbuf.c \
	lib_src/eas_mixer.c \
	lib_src/eas_ota.c \
	lib_src/eas_otadata.c \
	lib_src/eas_pan.c \
	lib_src/eas_pcm.c \
	lib_src/eas_pcmdata.c \
	lib_src/eas_public.c \
	lib_src/eas_reverb.c \
	lib_src/eas_reverbdata.c \
	lib_src/eas_rtttl.c \
	lib_src/eas_rtttldata.c \
	lib_src/eas_smf.c \
	lib_src/eas_smfdata.c \
	lib_src/eas_voicemgt.c \
	lib_src/eas_wavefile.c \
	lib_src/eas_wavefiledata.c \
	host_src/eas_config.c \
	host_src/eas_hostmm.c \
	host_src/eas_main.c \
	host_src/eas_report.c \
	host_src/eas_wave.c

LOCAL_CFLAGS+= -O2 -D NUM_OUTPUT_CHANNELS=2 \
	-D _SAMPLE_RATE_22050 -D EAS_FM_SYNTH \
	-D MAX_SYNTH_VOICES=16 -D _IMELODY_PARSER \
	-D _RTTTL_PARSER -D _OTA_PARSER \
	-D _WAVE_PARSER -D _REVERB_ENABLED \
	-D _CHORUS_ENABLED -D _IMA_DECODER \
	-D UNIFIED_DEBUG_MESSAGES

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/host_src/ \
	$(LOCAL_PATH)/lib_src/

LOCAL_ARM_MODE := arm

LOCAL_MODULE := libsonivox

LOCAL_COPY_HEADERS_TO := libsonivox
LOCAL_COPY_HEADERS := \
	host_src/eas.h \
	host_src/eas_types.h

include $(BUILD_SHARED_LIBRARY)
