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
	$(CC) -c -O2 -o $@ -I lib_src -I host_src -D NUM_OUTPUT_CHANNELS=2 -D _SAMPLE_RATE_22050 -D MAX_SYNTH_VOICES=16 -D EAS_FM_SYNTH -D _IMELODY_PARSER -D _RTTTL_PARSER -D _OTA_PARSER -D _WAVE_PARSER -D _REVERB_ENABLED -D _CHORUS_ENABLED -D _IMA_DECODER $<

%.o: %.s
	$(AS) -o $@ -EL -mcpu=arm946e-s -mfpu=softfpa $<

OBJS = eas_mididata.o eas_pan.o eas_wavefiledata.o eas_smfdata.o eas_imelody.o eas_math.o eas_fmengine.o eas_chorusdata.o eas_ima_tables.o eas_ota.o eas_rtttldata.o eas_imelodydata.o eas_fmtables.o eas_public.o eas_rtttl.o eas_reverb.o eas_fmsynth.o eas_midi.o eas_otadata.o eas_mixbuf.o eas_fmsndlib.o eas_imaadpcm.o eas_smf.o eas_chorus.o eas_pcm.o eas_mixer.o eas_wavefile.o eas_pcmdata.o eas_data.o eas_reverbdata.o eas_voicemgt.o

arm-fm-22k.a: $(OBJS)
	$(AR) rc lib$@ $(OBJS)

