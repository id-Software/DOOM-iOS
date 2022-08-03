/*----------------------------------------------------------------------------
 *
 * File:
 * fmsynth.c
 *
 * Contents and purpose:
 * Implements the high-level FM synthesizer functions.
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
 *   $Revision: 795 $
 *   $Date: 2007-08-01 00:14:45 -0700 (Wed, 01 Aug 2007) $
 *----------------------------------------------------------------------------
*/

// includes
#include "eas_host.h"
#include "eas_report.h"

#include "eas_data.h"
#include "eas_synth_protos.h"
#include "eas_audioconst.h"
#include "eas_fmengine.h"
#include "eas_math.h"

/* option sanity check */
#ifdef _REVERB
#error "No reverb for FM synthesizer"
#endif
#ifdef _CHORUS
#error "No chorus for FM synthesizer"
#endif

/*
 * WARNING: These macros can cause unwanted side effects. Use them
 * with care. For example, min(x++,y++) will cause either x or y to be
 * incremented twice.
 */
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

/* pivot point for keyboard scalars */
#define EG_SCALE_PIVOT_POINT 64
#define KEY_SCALE_PIVOT_POINT 36

/* This number is the negative of the frequency of the note (in cents) of
 * the sine wave played at unity. The number can be calculated as follows:
 *
 * MAGIC_NUMBER = 1200 * log(base2) (SINE_TABLE_SIZE * 8.175798916 / SAMPLE_RATE)
 *
 * 8.17578 is a reference to the frequency of MIDI note 0
 */
#if defined (_SAMPLE_RATE_8000)
#define MAGIC_NUMBER 1279
#elif   defined (_SAMPLE_RATE_16000)
#define MAGIC_NUMBER 79
#elif   defined (_SAMPLE_RATE_20000)
#define MAGIC_NUMBER -308
#elif   defined (_SAMPLE_RATE_22050)
#define MAGIC_NUMBER -477
#elif   defined (_SAMPLE_RATE_24000)
#define MAGIC_NUMBER -623
#elif defined (_SAMPLE_RATE_32000)
#define MAGIC_NUMBER -1121
#elif defined (_SAMPLE_RATE_44100)
#define MAGIC_NUMBER -1677
#elif defined (_SAMPLE_RATE_48000)
#define MAGIC_NUMBER -1823
#endif

/* externs */
extern const EAS_I16 fmControlTable[128];
extern const EAS_U16 fmRateTable[256];
extern const EAS_U16 fmAttackTable[16];
extern const EAS_U8 fmDecayTable[16];
extern const EAS_U8 fmReleaseTable[16];
extern const EAS_U8 fmScaleTable[16];

/* local prototypes */
/*lint -esym(715, pVoiceMgr) standard synthesizer interface - pVoiceMgr not used */
static EAS_RESULT FM_Initialize (S_VOICE_MGR *pVoiceMgr) { return EAS_SUCCESS; }
static EAS_RESULT FM_StartVoice (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, EAS_I32 voiceNum, EAS_U16 regionIndex);
static EAS_BOOL FM_UpdateVoice (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, EAS_I32 voiceNum, EAS_I32 *pMixBuffer, EAS_I32 numSamples);
static void FM_ReleaseVoice (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, EAS_I32 voiceNum);
static void FM_MuteVoice (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, EAS_I32 voiceNum);
static void FM_SustainPedal (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, S_SYNTH_CHANNEL *pChannel, EAS_I32 voiceNum);
static void FM_UpdateChannel (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, EAS_U8 channel);


/*----------------------------------------------------------------------------
 * Synthesizer interface
 *----------------------------------------------------------------------------
*/
const S_SYNTH_INTERFACE fmSynth =
{
    FM_Initialize,
    FM_StartVoice,
    FM_UpdateVoice,
    FM_ReleaseVoice,
    FM_MuteVoice,
    FM_SustainPedal,
    FM_UpdateChannel
};

#ifdef FM_OFFBOARD
const S_FRAME_INTERFACE fmFrameInterface =
{
    FM_StartFrame,
    FM_EndFrame
};
#endif

/*----------------------------------------------------------------------------
 * inline functions
 *----------------------------------------------------------------------------
 */
EAS_INLINE S_FM_VOICE *GetFMVoicePtr (S_VOICE_MGR *pVoiceMgr, EAS_INT voiceNum)
{
    return &pVoiceMgr->fmVoices[voiceNum];
}
EAS_INLINE S_SYNTH_CHANNEL *GetChannelPtr (S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice)
{
    return &pSynth->channels[pVoice->channel & 15];
}
EAS_INLINE const S_FM_REGION *GetFMRegionPtr (S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice)
{
#ifdef _SECONDARY_SYNTH
    return &pSynth->pEAS->pFMRegions[pVoice->regionIndex & REGION_INDEX_MASK];
#else
    return &pSynth->pEAS->pFMRegions[pVoice->regionIndex];
#endif
}

/*----------------------------------------------------------------------------
 * FM_SynthIsOutputOperator
 *----------------------------------------------------------------------------
 * Purpose:
 * Returns true if the operator is a direct output and not muted
 *
 * Inputs:
 *
 * Outputs:
 * Returns boolean
 *----------------------------------------------------------------------------
*/
static EAS_BOOL FM_SynthIsOutputOperator (const S_FM_REGION *pRegion, EAS_INT operIndex)
{

    /* see if voice is muted */
    if ((pRegion->oper[operIndex].gain & 0xfc) == 0)
        return 0;

    /* check based on mode */
    switch (pRegion->region.keyGroupAndFlags & 7)
    {

        /* mode 0 - all operators are external */
        case 0:
            return EAS_TRUE;

        /* mode 1 - operators 1-3 are external */
        case 1:
            if (operIndex != 0)
                return EAS_TRUE;
        break;

        /* mode 2 - operators 1 & 3 are external */
        case 2:
            if ((operIndex == 1) || (operIndex == 3))
                return EAS_TRUE;
            break;

        /* mode 2 - operators 1 & 2 are external */
        case 3:
            if ((operIndex == 1) || (operIndex == 2))
                return EAS_TRUE;
            break;

        /* modes 4 & 5 - operator 1 is external */
        case 4:
        case 5:
            if (operIndex == 1)
                return EAS_TRUE;
            break;

        default:
            { /* dpp: EAS_ReportEx(_EAS_SEVERITY_FATAL,"Invalid voice mode: %d",
                pRegion->region.keyGroupAndFlags & 7); */ }
    }

    return EAS_FALSE;
}

/*----------------------------------------------------------------------------
 * FM_CalcEGRate()
 *----------------------------------------------------------------------------
 * Purpose:
 *
 * Inputs:
 * nKeyNumber - MIDI note
 * nLogRate - logarithmic scale rate from patch data
 * nKeyScale - key scaling factor for this EG
 *
 * Outputs:
 * 16-bit linear multiplier
 *----------------------------------------------------------------------------
*/

static EAS_U16 FM_CalcEGRate (EAS_U8 nKeyNumber, EAS_U8 nLogRate, EAS_U8 nEGScale)
{
    EAS_I32 temp;

    /* incorporate key scaling on release rate */
    temp = (EAS_I32) nLogRate << 7;
    temp += ((EAS_I32) nKeyNumber - EG_SCALE_PIVOT_POINT) * (EAS_I32) nEGScale;

    /* saturate */
    temp = max(temp, 0);
    temp = min(temp, 32767);

    /* look up in rate table */
    /*lint -e{704} use shift for performance */
    return fmRateTable[temp >> 8];
}

/*----------------------------------------------------------------------------
 * FM_ReleaseVoice()
 *----------------------------------------------------------------------------
 * Purpose:
 * The selected voice is being released.
 *
 * Inputs:
 * psEASData - pointer to S_EAS_DATA
 * pVoice - pointer to voice to release
 *
 * Outputs:
 * None
 *----------------------------------------------------------------------------
*/
static void FM_ReleaseVoice (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, EAS_I32 voiceNum)
{
    EAS_INT operIndex;
    const S_FM_REGION *pRegion;
    S_FM_VOICE *pFMVoice;

    /* check to see if voice responds to NOTE-OFF's */
    pRegion = GetFMRegionPtr(pSynth, pVoice);
    if (pRegion->region.keyGroupAndFlags & REGION_FLAG_ONE_SHOT)
        return;

    /* set all envelopes to release state */
    pFMVoice = GetFMVoicePtr(pVoiceMgr, voiceNum);
    for (operIndex = 0; operIndex < 4; operIndex++)
    {
        pFMVoice->oper[operIndex].envState = eFMEnvelopeStateRelease;

        /* incorporate key scaling on release rate */
        pFMVoice->oper[operIndex].envRate = FM_CalcEGRate(
                pVoice->note,
                fmReleaseTable[pRegion->oper[operIndex].velocityRelease & 0x0f],
                fmScaleTable[pRegion->oper[operIndex].egKeyScale >> 4]);
    } /* end for (operIndex = 0; operIndex < 4; operIndex++) */
}

/*----------------------------------------------------------------------------
 * FM_MuteVoice()
 *----------------------------------------------------------------------------
 * Purpose:
 * The selected voice is being muted.
 *
 * Inputs:
 * pVoice - pointer to voice to release
 *
 * Outputs:
 * None
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, pSynth) standard interface, pVoiceMgr not used */
static void FM_MuteVoice (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, EAS_I32 voiceNum)
{
    S_FM_VOICE *pFMVoice;

    /* clear deferred action flags */
    pVoice->voiceFlags &=
        ~(VOICE_FLAG_DEFER_MIDI_NOTE_OFF |
        VOICE_FLAG_SUSTAIN_PEDAL_DEFER_NOTE_OFF |
        VOICE_FLAG_DEFER_MUTE);

    /* set all envelopes to muted state */
    pFMVoice = GetFMVoicePtr(pVoiceMgr, voiceNum);
    pFMVoice->oper[0].envState = eFMEnvelopeStateMuted;
    pFMVoice->oper[1].envState = eFMEnvelopeStateMuted;
    pFMVoice->oper[2].envState = eFMEnvelopeStateMuted;
    pFMVoice->oper[3].envState = eFMEnvelopeStateMuted;
}

/*----------------------------------------------------------------------------
 * FM_SustainPedal()
 *----------------------------------------------------------------------------
 * Purpose:
 * The selected voice is held due to sustain pedal
 *
 * Inputs:
 * pVoice - pointer to voice to sustain
 *
 * Outputs:
 * None
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, pChannel) standard interface, pVoiceMgr not used */
static void FM_SustainPedal (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, S_SYNTH_CHANNEL *pChannel, EAS_I32 voiceNum)
{
    const S_FM_REGION *pRegion;
    S_FM_VOICE *pFMVoice;
    EAS_INT operIndex;

    pRegion = GetFMRegionPtr(pSynth, pVoice);
    pFMVoice = GetFMVoicePtr(pVoiceMgr, voiceNum);

    /* check to see if any envelopes are above the sustain level */
    for (operIndex = 0; operIndex < 4; operIndex++)
    {

        /* if level control or envelope gain is zero, skip this envelope */
        if (((pRegion->oper[operIndex].gain & 0xfc) == 0) ||
            (pFMVoice->oper[operIndex].envGain == 0))
        {
            continue;
        }

        /* if the envelope gain is above the sustain level, we need to catch this voice */
        if (pFMVoice->oper[operIndex].envGain >= ((EAS_U16) (pRegion->oper[operIndex].sustain & 0xfc) << 7))
        {

            /* reset envelope to decay state */
            pFMVoice->oper[operIndex].envState = eFMEnvelopeStateDecay;

            pFMVoice->oper[operIndex].envRate = FM_CalcEGRate(
                    pVoice->note,
                    fmDecayTable[pRegion->oper[operIndex].attackDecay & 0x0f],
                    fmScaleTable[pRegion->oper[operIndex].egKeyScale >> 4]);

            /* set voice to decay state */
            pVoice->voiceState = eVoiceStatePlay;

            /* set sustain flag */
            pVoice->voiceFlags |= VOICE_FLAG_SUSTAIN_PEDAL_DEFER_NOTE_OFF;
        }
    } /* end for (operIndex = 0; operIndex < 4; operIndex++) */
}

/*----------------------------------------------------------------------------
 * FM_StartVoice()
 *----------------------------------------------------------------------------
 * Purpose:
 * Assign the region for the given instrument using the midi key number
 * and the RPN2 (coarse tuning) value. By using RPN2 as part of the
 * region selection process, we reduce the amount a given sample has
 * to be transposed by selecting the closest recorded root instead.
 *
 * This routine is the second half of SynthAssignRegion().
 * If the region was successfully found by SynthFindRegionIndex(),
 * then assign the region's parameters to the voice.
 *
 * Setup and initialize the following voice parameters:
 * m_nRegionIndex
 *
 * Inputs:
 * pVoice - ptr to the voice we have assigned for this channel
 * nRegionIndex - index of the region
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 * success - could find and assign the region for this voice's note otherwise
 * failure - could not find nor assign the region for this voice's note
 *
 * Side Effects:
 * psSynthObject->m_sVoice[].m_nRegionIndex is assigned
 * psSynthObject->m_sVoice[] parameters are assigned
 *----------------------------------------------------------------------------
*/
static EAS_RESULT FM_StartVoice (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, EAS_I32 voiceNum, EAS_U16 regionIndex)
{
    S_FM_VOICE *pFMVoice;
    S_SYNTH_CHANNEL *pChannel;
    const S_FM_REGION *pRegion;
    EAS_I32 temp;
    EAS_INT operIndex;

    /* establish pointers to data */
    pVoice->regionIndex = regionIndex;
    pFMVoice = GetFMVoicePtr(pVoiceMgr, voiceNum);
    pChannel = GetChannelPtr(pSynth, pVoice);
    pRegion = GetFMRegionPtr(pSynth, pVoice);

    /* update static channel parameters */
    if (pChannel->channelFlags & CHANNEL_FLAG_UPDATE_CHANNEL_PARAMETERS)
        FM_UpdateChannel(pVoiceMgr, pSynth, pVoice->channel & 15);

    /* init the LFO */
    pFMVoice->lfoValue = 0;
    pFMVoice->lfoPhase = 0;
    pFMVoice->lfoDelay = (EAS_U16) (fmScaleTable[pRegion->lfoFreqDelay & 0x0f] >> 1);

#if (NUM_OUTPUT_CHANNELS == 2)
    /* calculate pan gain values only if stereo output */
    /* set up panning only at note start */
    temp = (EAS_I32) pChannel->pan - 64;
    temp += (EAS_I32) pRegion->pan;
    if (temp < -64)
        temp = -64;
    if (temp > 64)
        temp = 64;
    pFMVoice->pan = (EAS_I8) temp;
#endif /* #if (NUM_OUTPUT_CHANNELS == 2) */

    /* no samples have been synthesized for this note yet */
    pVoice->voiceFlags = VOICE_FLAG_NO_SAMPLES_SYNTHESIZED_YET;

    /* initialize gain value for anti-zipper filter */
    pFMVoice->voiceGain = (EAS_I16) EAS_LogToLinear16(pChannel->staticGain);
    pFMVoice->voiceGain = (EAS_I16) FMUL_15x15(pFMVoice->voiceGain, pSynth->masterVolume);

    /* initialize the operators */
    for (operIndex = 0; operIndex < 4; operIndex++)
    {

        /* establish operator output gain level */
        /*lint -e{701} <use shift for performance> */
        pFMVoice->oper[operIndex].outputGain = EAS_LogToLinear16(((EAS_I16) (pRegion->oper[operIndex].gain & 0xfc) - 0xfc) << 7);

        /* check for linear velocity flag */
        /*lint -e{703} <use shift for performance> */
        if (pRegion->oper[operIndex].flags & FM_OPER_FLAG_LINEAR_VELOCITY)
            temp = (EAS_I32) (pVoice->velocity - 127) << 5;
        else
            temp = (EAS_I32) fmControlTable[pVoice->velocity];

        /* scale velocity */
        /*lint -e{704} use shift for performance */
        temp = (temp * (EAS_I32)(pRegion->oper[operIndex].velocityRelease & 0xf0)) >> 7;

        /* include key scalar */
        temp -= ((EAS_I32) pVoice->note - KEY_SCALE_PIVOT_POINT) * (EAS_I32) fmScaleTable[pRegion->oper[operIndex].egKeyScale & 0x0f];

        /* saturate */
        temp = min(temp, 0);
        temp = max(temp, -32768);

        /* save static gain parameters */
        pFMVoice->oper[operIndex].baseGain = (EAS_I16) EAS_LogToLinear16(temp);

        /* incorporate key scaling on decay rate */
        pFMVoice->oper[operIndex].envRate = FM_CalcEGRate(
            pVoice->note,
            fmDecayTable[pRegion->oper[operIndex].attackDecay & 0x0f],
            fmScaleTable[pRegion->oper[operIndex].egKeyScale >> 4]);

        /* if zero attack time, max out envelope and jump to decay state */
        if ((pRegion->oper[operIndex].attackDecay & 0xf0) == 0xf0)
        {

            /* start out envelope at max */
            pFMVoice->oper[operIndex].envGain = 0x7fff;

            /* set envelope to decay state */
            pFMVoice->oper[operIndex].envState = eFMEnvelopeStateDecay;
        }

        /* start envelope at zero and start in attack state */
        else
        {
            pFMVoice->oper[operIndex].envGain = 0;
            pFMVoice->oper[operIndex].envState = eFMEnvelopeStateAttack;
        }
    }

    return EAS_SUCCESS;
}

/*----------------------------------------------------------------------------
 * FM_UpdateChannel()
 *----------------------------------------------------------------------------
 * Purpose:
 * Calculate and assign static channel parameters
 * These values only need to be updated if one of the controller values
 * for this channel changes.
 * Called when CHANNEL_FLAG_UPDATE_CHANNEL_PARAMETERS flag is set.
 *
 * Inputs:
 * nChannel - channel to update
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 *
 * Side Effects:
 * - the given channel's static gain and static pitch are updated
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, pVoiceMgr) standard interface, pVoiceMgr not used */
static void FM_UpdateChannel (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, EAS_U8 channel)
{
    S_SYNTH_CHANNEL *pChannel;
    EAS_I32 temp;

    pChannel = &pSynth->channels[channel];

    /* convert CC7 volume controller to log scale */
    temp = fmControlTable[pChannel->volume];

    /* incorporate CC11 expression controller */
    temp += fmControlTable[pChannel->expression];

    /* saturate */
    pChannel->staticGain = (EAS_I16) max(temp,-32768);

    /* calculate pitch bend */
    /*lint -e{703} <avoid multiply for performance>*/
    temp = (((EAS_I32)(pChannel->pitchBend) << 2) - 32768);

    temp = FMUL_15x15(temp, pChannel->pitchBendSensitivity);

    /* include "magic number" compensation for sample rate and lookup table size */
    temp += MAGIC_NUMBER;

    /* if this is not a drum channel, then add in the per-channel tuning */
    if (!(pChannel->channelFlags & CHANNEL_FLAG_RHYTHM_CHANNEL))
        temp += (pChannel->finePitch + (pChannel->coarsePitch * 100));

    /* save static pitch */
    pChannel->staticPitch = temp;

    /* Calculate LFO modulation depth */
    /* mod wheel to LFO depth */
    temp = FMUL_15x15(DEFAULT_LFO_MOD_WHEEL_TO_PITCH_CENTS,
    pChannel->modWheel << (NUM_EG1_FRAC_BITS -7));

    /* channel pressure to LFO depth */
    pChannel->lfoAmt = (EAS_I16) (temp +
    FMUL_15x15(DEFAULT_LFO_CHANNEL_PRESSURE_TO_PITCH_CENTS,
    pChannel->channelPressure << (NUM_EG1_FRAC_BITS -7)));

    /* clear update flag */
    pChannel->channelFlags &= ~CHANNEL_FLAG_UPDATE_CHANNEL_PARAMETERS;
    return;
}

/*----------------------------------------------------------------------------
 * FM_UpdateLFO()
 *----------------------------------------------------------------------------
 * Purpose:
 * Calculate the LFO for the given voice
 *
 * Inputs:
 * pVoice - ptr to the voice whose LFO we want to update
 * psEASData - pointer to overall EAS data structure - used for debug only
 *
 * Outputs:
 *
 * Side Effects:
 * - updates LFO values for the given voice
 *----------------------------------------------------------------------------
*/
static void FM_UpdateLFO (S_FM_VOICE *pFMVoice, const S_FM_REGION *pRegion)
{

    /* increment the LFO phase if the delay time has elapsed */
    if (!pFMVoice->lfoDelay)
    {
        /*lint -e{701} <use shift for performance> */
        pFMVoice->lfoPhase = pFMVoice->lfoPhase + (EAS_U16) (-fmControlTable[((15 - (pRegion->lfoFreqDelay >> 4)) << 3) + 4]);

        /* square wave LFO? */
        if (pRegion->region.keyGroupAndFlags & REGION_FLAG_SQUARE_WAVE)
            pFMVoice->lfoValue = (EAS_I16)(pFMVoice->lfoPhase & 0x8000 ? -32767 : 32767);

        /* trick to get a triangle wave out of a sawtooth */
        else
        {
            pFMVoice->lfoValue = (EAS_I16) (pFMVoice->lfoPhase << 1);
            /*lint -e{502} <shortcut to turn sawtooth into sine wave> */
            if ((pFMVoice->lfoPhase > 0x3fff) && (pFMVoice->lfoPhase < 0xC000))
                pFMVoice->lfoValue = ~pFMVoice->lfoValue;
        }
    }

    /* still in delay */
    else
        pFMVoice->lfoDelay--;

    return;
}

/*----------------------------------------------------------------------------
 * FM_UpdateEG()
 *----------------------------------------------------------------------------
 * Purpose:
 * Calculate the synthesis parameters for an operator to be used during
 * the next buffer
 *
 * Inputs:
 * pVoice - pointer to the voice being updated
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 *
 * Side Effects:
 *
 *----------------------------------------------------------------------------
*/
static EAS_BOOL FM_UpdateEG (S_SYNTH_VOICE *pVoice, S_OPERATOR *pOper, const S_FM_OPER *pOperData, EAS_BOOL oneShot)
{
    EAS_U32 temp;
    EAS_BOOL done;

    /* set flag assuming the envelope is not done */
    done = EAS_FALSE;

    /* take appropriate action based on state */
    switch (pOper->envState)
    {

        case eFMEnvelopeStateAttack:

            /* the envelope is linear during the attack, so add the value */
            temp = pOper->envGain + fmAttackTable[pOperData->attackDecay >> 4];

            /* check for end of attack */
            if (temp >= 0x7fff)
            {
                pOper->envGain = 0x7fff;
                pOper->envState = eFMEnvelopeStateDecay;
            }
            else
                pOper->envGain = (EAS_U16) temp;
            break;

        case eFMEnvelopeStateDecay:

            /* decay is exponential, multiply by decay rate */
            pOper->envGain = (EAS_U16) FMUL_15x15(pOper->envGain, pOper->envRate);

            /* check for sustain level reached */
            temp = (EAS_U32) (pOperData->sustain & 0xfc) << 7;
            if (pOper->envGain <= (EAS_U16) temp)
            {
                /* if this is a one-shot patch, go directly to release phase */
                if (oneShot)
                {
                    pOper->envRate = FM_CalcEGRate(
                    pVoice->note,
                    fmReleaseTable[pOperData->velocityRelease & 0x0f],
                    fmScaleTable[pOperData->egKeyScale >> 4]);
                    pOper->envState = eFMEnvelopeStateRelease;
                }

                /* normal sustaining type */
                else
                {
                    pOper->envGain = (EAS_U16) temp;
                    pOper->envState = eFMEnvelopeStateSustain;
                }
            }
            break;

        case eFMEnvelopeStateSustain:
            pOper->envGain = (EAS_U16)((EAS_U16)(pOperData->sustain & 0xfc) << 7);
            break;

        case eFMEnvelopeStateRelease:

            /* release is exponential, multiply by release rate */
            pOper->envGain = (EAS_U16) FMUL_15x15(pOper->envGain, pOper->envRate);

            /* fully released */
            if (pOper->envGain == 0)
            {
                pOper->envGain = 0;
                pOper->envState = eFMEnvelopeStateMuted;
                done = EAS_TRUE;
            }
            break;

        case eFMEnvelopeStateMuted:
            pOper->envGain = 0;
            done = EAS_TRUE;
            break;
        default:
            { /* dpp: EAS_ReportEx(_EAS_SEVERITY_FATAL,"Invalid operator state: %d", pOper->envState); */ }
    } /* end switch (pOper->m_eState) */

    return done;
}

/*----------------------------------------------------------------------------
 * FM_UpdateDynamic()
 *----------------------------------------------------------------------------
 * Purpose:
 * Calculate the synthesis parameters for this voice that will be used to
 * synthesize the next buffer
 *
 * Inputs:
 * pVoice - pointer to the voice being updated
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 * Returns EAS_TRUE if voice will be fully ramped to zero at the end of
 * the next synthesized buffer.
 *
 * Side Effects:
 *
 *----------------------------------------------------------------------------
*/
static EAS_BOOL FM_UpdateDynamic (S_SYNTH_VOICE *pVoice, S_FM_VOICE *pFMVoice, const S_FM_REGION *pRegion, S_SYNTH_CHANNEL *pChannel)
{
    EAS_I32 temp;
    EAS_I32 pitch;
    EAS_I32 lfoPitch;
    EAS_INT operIndex;
    EAS_BOOL done;

    /* increment LFO phase */
    FM_UpdateLFO(pFMVoice, pRegion);

    /* base pitch in cents */
    pitch = pVoice->note * 100;

    /* LFO amount includes LFO depth from programming + channel dynamics */
    temp = (fmScaleTable[pRegion->vibTrem >> 4] >> 1) + pChannel->lfoAmt;

    /* multiply by LFO output to get final pitch modulation */
    lfoPitch = FMUL_15x15(pFMVoice->lfoValue, temp);

    /* flag to indicate this voice is done */
    done = EAS_TRUE;

    /* iterate through operators to establish parameters */
    for (operIndex = 0; operIndex < 4; operIndex++)
    {
        EAS_BOOL bTemp;

        /* set base phase increment for each operator */
        temp = pRegion->oper[operIndex].tuning +
        pChannel->staticPitch;

        /* add vibrato effect unless it is disabled for this operator */
        if ((pRegion->oper[operIndex].flags & FM_OPER_FLAG_NO_VIBRATO) == 0)
            temp += lfoPitch;

        /* if note is monotonic, bias to MIDI note 60 */
        if (pRegion->oper[operIndex].flags & FM_OPER_FLAG_MONOTONE)
            temp += 6000;
        else
            temp += pitch;
        pFMVoice->oper[operIndex].pitch = (EAS_I16) temp;

        /* calculate envelope, returns true if done */
        bTemp = FM_UpdateEG(pVoice, &pFMVoice->oper[operIndex], &pRegion->oper[operIndex], pRegion->region.keyGroupAndFlags & REGION_FLAG_ONE_SHOT);

        /* check if all output envelopes have completed */
        if (FM_SynthIsOutputOperator(pRegion, operIndex))
            done = done && bTemp;
    }

    return done;
}

/*----------------------------------------------------------------------------
 * FM_UpdateVoice()
 *----------------------------------------------------------------------------
 * Purpose:
 * Synthesize a block of samples for the given voice.
 *
 * Inputs:
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 * number of samples actually written to buffer
 *
 * Side Effects:
 * - samples are added to the presently free buffer
 *
 *----------------------------------------------------------------------------
*/
static EAS_BOOL FM_UpdateVoice (S_VOICE_MGR *pVoiceMgr, S_SYNTH *pSynth, S_SYNTH_VOICE *pVoice, EAS_I32 voiceNum, EAS_I32 *pMixBuffer, EAS_I32 numSamples)
{
    S_SYNTH_CHANNEL *pChannel;
    const S_FM_REGION *pRegion;
    S_FM_VOICE *pFMVoice;
    S_FM_VOICE_CONFIG vCfg;
    S_FM_VOICE_FRAME vFrame;
    EAS_I32 temp;
    EAS_INT oper;
    EAS_U16 voiceGainTarget;
    EAS_BOOL done;

    /* setup some pointers */
    pChannel = GetChannelPtr(pSynth, pVoice);
    pRegion = GetFMRegionPtr(pSynth, pVoice);
    pFMVoice = GetFMVoicePtr(pVoiceMgr, voiceNum);

    /* if the voice is just starting, get the voice configuration data */
    if (pVoice->voiceFlags & VOICE_FLAG_NO_SAMPLES_SYNTHESIZED_YET)
    {

        /* split architecture may limit the number of voice starts */
#ifdef MAX_VOICE_STARTS
        if (pVoiceMgr->numVoiceStarts == MAX_VOICE_STARTS)
            return EAS_FALSE;
        pVoiceMgr->numVoiceStarts++;
#endif

        /* get initial parameters */
        vCfg.feedback = pRegion->feedback;
        vCfg.voiceGain = (EAS_U16) pFMVoice->voiceGain;

#if (NUM_OUTPUT_CHANNELS == 2)
        vCfg.pan = pFMVoice->pan;
#endif

        /* get voice mode */
        vCfg.flags = pRegion->region.keyGroupAndFlags & 7;

        /* get operator parameters */
        for (oper = 0; oper < 4; oper++)
        {
            /* calculate initial gain */
            vCfg.gain[oper] = (EAS_U16) FMUL_15x15(pFMVoice->oper[oper].baseGain, pFMVoice->oper[oper].envGain);
            vCfg.outputGain[oper] = pFMVoice->oper[oper].outputGain;

            /* copy noise waveform flag */
            if (pRegion->oper[oper].flags & FM_OPER_FLAG_NOISE)
                vCfg.flags |= (EAS_U8) (FLAG_FM_ENG_VOICE_OP1_NOISE << oper);
        }

#ifdef FM_OFFBOARD
        FM_ConfigVoice(voiceNum, &vCfg, pVoiceMgr->pFrameBuffer);
#else
        FM_ConfigVoice(voiceNum, &vCfg, NULL);
#endif

        /* clear startup flag */
        pVoice->voiceFlags &= ~VOICE_FLAG_NO_SAMPLES_SYNTHESIZED_YET;
    }

    /* calculate new synthesis parameters */
    done = FM_UpdateDynamic(pVoice, pFMVoice, pRegion, pChannel);

    /* calculate LFO gain modulation */
    /*lint -e{702} <use shift for performance> */
    temp = ((fmScaleTable[pRegion->vibTrem & 0x0f] >> 1) * pFMVoice->lfoValue) >> FM_LFO_GAIN_SHIFT;

    /* include channel gain */
    temp += pChannel->staticGain;

    /* -32768 or lower is infinite attenuation */
    if (temp < -32767)
        voiceGainTarget = 0;

    /* translate to linear gain multiplier */
    else
        voiceGainTarget = EAS_LogToLinear16(temp);

    /* include synth master volume */
    voiceGainTarget = (EAS_U16) FMUL_15x15(voiceGainTarget, pSynth->masterVolume);

    /* save target values for this frame */
    vFrame.voiceGain = voiceGainTarget;

    /* assume voice output is zero */
    pVoice->gain = 0;

    /* save operator targets for this frame */
    for (oper = 0; oper < 4; oper++)
    {
        vFrame.gain[oper] = (EAS_U16) FMUL_15x15(pFMVoice->oper[oper].baseGain, pFMVoice->oper[oper].envGain);
        vFrame.pitch[oper] = pFMVoice->oper[oper].pitch;

        /* use the highest output envelope level as the gain for the voice stealing algorithm */
        if (FM_SynthIsOutputOperator(pRegion, oper))
            pVoice->gain = max(pVoice->gain, (EAS_I16) vFrame.gain[oper]);
    }

    /* consider voice gain multiplier in calculating gain for stealing algorithm */
    pVoice->gain = (EAS_I16) FMUL_15x15(voiceGainTarget, pVoice->gain);

    /* synthesize samples */
#ifdef FM_OFFBOARD
    FM_ProcessVoice(voiceNum, &vFrame, numSamples, pVoiceMgr->operMixBuffer, pVoiceMgr->voiceBuffer, pMixBuffer, pVoiceMgr->pFrameBuffer);
#else
    FM_ProcessVoice(voiceNum, &vFrame, numSamples, pVoiceMgr->operMixBuffer, pVoiceMgr->voiceBuffer, pMixBuffer, NULL);
#endif

    return done;
}

