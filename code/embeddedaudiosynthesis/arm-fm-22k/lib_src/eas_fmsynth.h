/*----------------------------------------------------------------------------
 *
 * File:
 * eas_fmsynth.h
 *
 * Contents and purpose:
 * Implements the FM synthesizer functions.
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
 *   $Revision: 90 $
 *   $Date: 2006-07-11 20:18:13 -0700 (Tue, 11 Jul 2006) $
 *----------------------------------------------------------------------------
*/

#ifndef fmsynthH
#define fmsynthH

#include "eas_data.h"

#if defined (_FM_SYNTH)

/* FM envelope state */
typedef enum {
    eFMEnvelopeStateAttack = 0,
    eFMEnvelopeStateDecay,
    eFMEnvelopeStateSustain,
    eFMEnvelopeStateRelease,
    eFMEnvelopeStateMuted,
    eFMEnvelopeStateInvalid         /* should never be in this state! */
} E_FM_ENVELOPE_STATE;

/*------------------------------------
 * S_OPERATOR data structure
 *------------------------------------
*/
typedef struct s_operator_tag
{
    EAS_I16     pitch;              /* operator pitch in cents */
    EAS_U16     envGain;            /* envelope target */
    EAS_I16     baseGain;           /* patch gain (inc. vel & key scale) */
    EAS_U16     outputGain;         /* current output gain */
    EAS_U16     envRate;            /* calculated envelope rate */
    EAS_U8      envState;           /* envelope state */
    EAS_U8      pad;                /* pad to 16-bits */
} S_OPERATOR;
#endif

typedef struct s_fm_voice_tag
{
    S_OPERATOR          oper[4];        /* operator data */
    EAS_I16             voiceGain;      /* LFO + channel parameters */
    EAS_U16             lfoPhase;       /* LFO current phase */
    EAS_I16             lfoValue;       /* LFO current value */
    EAS_U16             lfoDelay;       /* keeps track of elapsed delay time */
    EAS_I8              pan;            /* stereo pan value (-64 to +64) */
    EAS_I8              pad;            /* reserved to maintain alignment */
} S_FM_VOICE;

#ifdef _FM_EDITOR
extern S_FM_REGION newPatch;
extern S_FM_REGION OriginalPatch;
#endif

extern EAS_U32 freqTable[];

#endif
