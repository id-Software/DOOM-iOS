/*----------------------------------------------------------------------------
 *
 * File:
 * eas_fmengine.h
 *
 * Contents and purpose:
 * Declarations, interfaces, and prototypes for FM synthesize low-level.
 *
 * Copyright Sonic Network Inc. 2004

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *----------------------------------------------------------------------------
 * Revision Control:
 *   $Revision: 664 $
 *   $Date: 2007-04-25 13:11:22 -0700 (Wed, 25 Apr 2007) $
 *----------------------------------------------------------------------------
*/

/* sentinel */
#ifndef _FMENGINE_H
#define _FMENGINE_H

/* check for split architecture */
#if defined (EAS_SPLIT_HYBRID_SYNTH) || defined(EAS_SPLIT_FM_SYNTH)
#define FM_OFFBOARD
#endif

/* output level to mix buffer (3 = -24dB) */
#define FM_GAIN_SHIFT 3
#define FM_MONO_GAIN_SHIFT 9

/* voice output level for stereo 15 = +6dB */
#define FM_STEREO_PRE_GAIN_SHIFT 11
#define FM_STEREO_POST_GAIN_SHIFT 10

/* modulator input level shift (21 = -30dB) */
#define FM_MODULATOR_INPUT_SHIFT 21

/* feedback control level shift (7 = 0dB) */
#define FM_FEEDBACK_SHIFT 7

/* synth final output level */
#define SYNTH_POST_GAIN_SHIFT 14

/* LFO modulation to gain control */
#define FM_LFO_GAIN_SHIFT 12

/* sine table is always a power of 2 - saves cycles in inner loop */
#define SINE_TABLE_SIZE_IN_BITS 11
#define SINE_TABLE_SIZE 2048

/* operator structure for FM engine */
typedef struct
{
    EAS_U32     phase;              /* current waveform phase */
    EAS_U16     gain;               /* current internal gain */
    EAS_U16     outputGain;         /* current output gain */
} S_FM_ENG_OPER;

typedef struct
{
    S_FM_ENG_OPER   oper[4];        /* operator data */
    EAS_I16         op1Out;         /* op1 output for feedback loop */
    EAS_I16         op3Out;         /* op3 output for feedback loop */
    EAS_U16         voiceGain;      /* LFO + channel parameters */
#if (NUM_OUTPUT_CHANNELS == 2)
    EAS_U16         gainLeft;       /* left gain multiplier */
    EAS_U16         gainRight;      /* right gain multiplier */
#endif
    EAS_U8          flags;          /* mode bits and noise waveform flags */
    EAS_U8          feedback;       /* feedback for Op1 and Op3 */
} S_FM_ENG_VOICE;

typedef struct
{
    EAS_U16         gain[4];        /* initial operator gain value */
    EAS_U16         outputGain[4];  /* initial operator output gain value */
    EAS_U16         voiceGain;      /* initial voice gain */
    EAS_U8          flags;          /* mode bits and noise waveform flags */
    EAS_U8          feedback;       /* feedback for Op1 and Op3 */
#if (NUM_OUTPUT_CHANNELS == 2)
    EAS_I8          pan;            /* pan value +/-64 */
#endif
} S_FM_VOICE_CONFIG;

typedef struct
{
    EAS_U16         gain[4];        /* new operator gain value */
    EAS_I16         pitch[4];       /* new pitch value */
    EAS_U16         voiceGain;      /* new voice gain */
} S_FM_VOICE_FRAME;

/* bit definitions for S_FM_ENG_VOICE.flags */
#define FLAG_FM_ENG_VOICE_OP1_NOISE     0x10    /* operator 1 source is PRNG */
#define FLAG_FM_ENG_VOICE_OP2_NOISE     0x20    /* operator 2 source is PRNG */
#define FLAG_FM_ENG_VOICE_OP3_NOISE     0x40    /* operator 3 source is PRNG */
#define FLAG_FM_ENG_VOICE_OP4_NOISE     0x80    /* operator 4 source is PRNG */

#ifdef FM_OFFBOARD
extern EAS_BOOL FM_StartFrame (EAS_FRAME_BUFFER_HANDLE pFrameBuffer);
extern EAS_BOOL FM_EndFrame (EAS_FRAME_BUFFER_HANDLE pFrameBuffe, EAS_I32 *pMixBuffer, EAS_I16 masterGain);
#endif

/* FM engine prototypes */
extern void FM_ConfigVoice (EAS_I32 voiceNum, S_FM_VOICE_CONFIG *vCfg, EAS_FRAME_BUFFER_HANDLE pFrameBuffer);
extern void FM_ProcessVoice (EAS_I32 voiceNum, S_FM_VOICE_FRAME *pFrame, EAS_I32 numSamplesToAdd, EAS_PCM *pTempBuffer, EAS_PCM *pBuffer, EAS_I32 *pMixBuffer, EAS_FRAME_BUFFER_HANDLE pFrameBuffer);

#endif
/* #ifndef _FMENGINE_H */

