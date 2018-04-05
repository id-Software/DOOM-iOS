LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES = \
	lib_src/eas_chorus.c \
	lib_src/eas_chorusdata.c \
	lib_src/eas_data.c \
	lib_src/eas_dlssynth.c \
	lib_src/eas_flog.c \
	lib_src/eas_ima_tables.c \
	lib_src/eas_imaadpcm.c \
	lib_src/eas_imelody.c \
	lib_src/eas_imelodydata.c \
	lib_src/eas_math.c \
	lib_src/eas_mdls.c \
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
	lib_src/eas_wtengine.c \
	lib_src/eas_wtsynth.c \
	lib_src/eas_xmf.c \
	lib_src/eas_xmfdata.c \
	lib_src/wt_22khz.c \
	lib_src/jet.c \
	host_src/eas_config.c \
	host_src/eas_hostmm.c \
	host_src/eas_main.c \
	host_src/eas_report.c \
	host_src/eas_wave.c

# not using these modules
#	lib_src/eas_wavefile.c \
#	lib_src/eas_wavefiledata.c \

LOCAL_CFLAGS+= -O2 -D UNIFIED_DEBUG_MESSAGES -D EAS_WT_SYNTH \
	-D _IMELODY_PARSER -D _RTTTL_PARSER -D _OTA_PARSER \
	-D _XMF_PARSER -D NUM_OUTPUT_CHANNELS=2 \
	-D _SAMPLE_RATE_22050 -D MAX_SYNTH_VOICES=64 \
	-D _8_BIT_SAMPLES -D _FILTER_ENABLED \
	-D DLS_SYNTHESIZER \
	-D _REVERB_ENABLED
	
# not using these options
# -D _WAVE_PARSER
# -D _IMA_DECODER (needed for IMA-ADPCM wave files)
# -D _CHORUS_ENABLED
  	
LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/host_src \
	$(LOCAL_PATH)/lib_src

LOCAL_ARM_MODE := arm

LOCAL_MODULE := libsonivox

LOCAL_COPY_HEADERS_TO := libsonivox
LOCAL_COPY_HEADERS := \
	host_src/eas.h \
	host_src/eas_types.h \
	host_src/eas_reverb.h \
	host_src/jet.h

ifeq ($(TARGET_ARCH),arm)
LOCAL_SRC_FILES+= \
	lib_src/ARM-E_filter_gnu.s \
	lib_src/ARM-E_interpolate_loop_gnu.s \
	lib_src/ARM-E_interpolate_noloop_gnu.s \
	lib_src/ARM-E_mastergain_gnu.s \
	lib_src/ARM-E_voice_gain_gnu.s

asm_flags := \
	-I $(LOCAL_PATH)/lib_src \
	--defsym SAMPLE_RATE_22050=1 \
	--defsym STEREO_OUTPUT=1 \
	--defsym FILTER_ENABLED=1 \
	--defsym SAMPLES_8_BIT=1

LOCAL_CFLAGS+= -D NATIVE_EAS_KERNEL \
	$(foreach f,$(asm_flags),-Wa,"$(f)")

LOCAL_COPY_HEADERS += lib_src/ARM_synth_constants_gnu.inc
endif

LOCAL_SHARED_LIBRARIES := \
	libutils libcutils

LOCAL_LDLIBS := -lpthread

include $(BUILD_SHARED_LIBRARY)

