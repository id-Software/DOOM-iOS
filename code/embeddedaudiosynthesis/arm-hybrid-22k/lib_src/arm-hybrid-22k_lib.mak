#
# Auto-generated sample makefile
#
# This makefile is intended for use with GNU make.
# Set the paths to the tools (CC, AR, LD, etc.)
#

vpath %.c lib_src

CC = C:\Program Files\GNUARM\bin\arm-elf-gcc.exe
AS = C:\Program Files\GNUARM\bin\arm-elf-as.exe
LD = C:\Program Files\GNUARM\bin\arm-elf-gcc.exe
AR = C:\Program Files\GNUARM\bin\arm-elf-ar.exe

%.o: %.c
	$(CC) -c -O2 -o $@ -I lib_src -I host_src -D NUM_OUTPUT_CHANNELS=2 -D _SAMPLE_RATE_22050 -D MAX_SYNTH_VOICES=32 -D NUM_PRIMARY_VOICES=8 -D EAS_HYBRID_SYNTH -D _8_BIT_SAMPLES -D _FILTER_ENABLED -D _IMELODY_PARSER -D _RTTTL_PARSER -D _OTA_PARSER -D _WAVE_PARSER -D _REVERB_ENABLED -D _CHORUS_ENABLED -D _IMA_DECODER -D NATIVE_EAS_KERNEL $<

%.o: %.s
	$(AS) -o $@ -EL -mcpu=arm946e-s -mfpu=softfpa -I lib_src --defsym CHECK_STACK=0 --defsym REVERB=0 --defsym CHORUS=0 --defsym STEREO_OUTPUT=1 --defsym SAMPLE_RATE_22050=1 --defsym SAMPLES_8_BIT=1 --defsym FILTER_ENABLED=1 $<

OBJS = eas_mididata.o eas_pan.o eas_wavefiledata.o eas_wavefile.o eas_smfdata.o eas_imelody.o eas_math.o eas_fmengine.o ARM-E_filter_gnu.o eas_chorusdata.o eas_wtengine.o eas_ota.o eas_rtttldata.o eas_reverbdata.o eas_public.o ARM-E_interpolate_loop_gnu.o eas_rtttl.o eas_reverb.o ARM-E_voice_gain_gnu.o eas_fmsynth.o eas_midi.o eas_otadata.o eas_wtsynth.o eas_mixbuf.o eas_imaadpcm.o hybrid_22khz_mcu.o eas_smf.o eas_chorus.o eas_pcm.o eas_mixer.o eas_data.o eas_imelodydata.o eas_pcmdata.o eas_ima_tables.o eas_fmtables.o ARM-E_interpolate_noloop_gnu.o ARM-E_mastergain_gnu.o eas_voicemgt.o

arm-hybrid-22k.a: $(OBJS)
	$(AR) rc lib$@ $(OBJS)

