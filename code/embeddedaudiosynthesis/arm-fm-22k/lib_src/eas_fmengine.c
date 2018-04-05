/*----------------------------------------------------------------------------
 *
 * File:
 * eas_fmengine.c
 *
 * Contents and purpose:
 * Implements the low-level FM synthesizer functions.
 *
 * Copyright Sonic Network Inc. 2004, 2005

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

/* includes */
#include "eas_types.h"
#include "eas_math.h"
#include "eas_audioconst.h"
#include "eas_fmengine.h"

#if defined(EAS_FM_SYNTH) || defined(EAS_HYBRID_SYNTH) || defined(EAS_SPLIT_HYBRID_SYNTH) || defined(EAS_SPLIT_FM_SYNTH)
#include "eas_data.h"
#endif

/* externals */
extern const EAS_I16 sineTable[];
extern const EAS_U8 fmScaleTable[16];

// saturation constants for 32-bit to 16-bit conversion
#define _EAS_MAX_OUTPUT 32767
#define _EAS_MIN_OUTPUT -32767

static S_FM_ENG_VOICE voices[NUM_FM_VOICES];

/* local prototypes */
void FM_SynthMixVoice (S_FM_ENG_VOICE *p,  EAS_U16 gainTarget, EAS_I32 numSamplesToAdd, EAS_PCM *pInputBuffer, EAS_I32 *pBuffer);

/* used in development environment */
#if defined(_SATURATION_MONITOR)
static EAS_BOOL bSaturated = EAS_FALSE;

/*----------------------------------------------------------------------------
 * FM_CheckSaturation()
 *----------------------------------------------------------------------------
 * Purpose:
 * Allows the sound development tool to check for saturation at the voice
 * level. Useful for tuning the level controls.
 *
 * Inputs:
 *
 * Outputs:
 * Returns true if saturation has occurred since the last time the function
 * was called.
 *
 * Side Effects:
 * Resets the saturation flag
 *----------------------------------------------------------------------------
*/
EAS_BOOL FM_CheckSaturation ()
{
    EAS_BOOL bTemp;
    bTemp = bSaturated;
    bSaturated = EAS_FALSE;
    return bTemp;
}
#endif

/*----------------------------------------------------------------------------
 * FM_Saturate()
 *----------------------------------------------------------------------------
 * Purpose:
 * This inline function saturates a 32-bit number to 16-bits
 *
 * Inputs:
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 * Returns a 16-bit integer
 *----------------------------------------------------------------------------
*/
EAS_INLINE EAS_I16 FM_Saturate (EAS_I32 nValue)
{
    if (nValue > _EAS_MAX_OUTPUT)
    {
#if defined(_SATURATION_MONITOR)
        bSaturated = EAS_TRUE;
#endif
        return _EAS_MAX_OUTPUT;
    }
    if (nValue < _EAS_MIN_OUTPUT)
    {
#if defined(_SATURATION_MONITOR)
        bSaturated = EAS_TRUE;
#endif
        return _EAS_MIN_OUTPUT;
    }
    return (EAS_I16) nValue;
}

/*----------------------------------------------------------------------------
 * FM_Noise()
 *----------------------------------------------------------------------------
 * Purpose:
 * A 31-bit low-cost linear congruential PRNG algorithm used to
 * generate noise.
 *
 * Inputs:
 * pnSeed - pointer to 32-bit PRNG seed
 *
 * Outputs:
 * Returns a 16-bit integer
 *----------------------------------------------------------------------------
*/
EAS_INLINE EAS_I16 FM_Noise (EAS_U32 *pnSeed)
{
    *pnSeed = *pnSeed * 214013L + 2531011L;
    return (EAS_I16) ((*pnSeed >> 15) & 0xffff);
}

/*----------------------------------------------------------------------------
 * FM_PhaseInc()
 *----------------------------------------------------------------------------
 * Purpose:
 * Transform pitch cents to linear phase increment
 *
 * Inputs:
 * nCents -     measured in cents
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 * nResult - int.frac result (where frac has NUM_DENTS_FRAC_BITS)
 *
 * Side Effects:
 *
 *----------------------------------------------------------------------------
*/
static EAS_I32 FM_PhaseInc (EAS_I32 nCents)
{
    EAS_I32 nDents;
    EAS_I32 nExponentInt, nExponentFrac;
    EAS_I32 nTemp1, nTemp2;
    EAS_I32 nResult;

    /* convert cents to dents */
    nDents = FMUL_15x15(nCents, CENTS_TO_DENTS);
    nExponentInt = GET_DENTS_INT_PART(nDents) + (32 - SINE_TABLE_SIZE_IN_BITS - NUM_EG1_FRAC_BITS);
    nExponentFrac = GET_DENTS_FRAC_PART(nDents);

    /* implement 2^(fracPart) as a power series */
    nTemp1 = GN2_TO_X2 + MULT_DENTS_COEF(nExponentFrac, GN2_TO_X3);
    nTemp2 = GN2_TO_X1 + MULT_DENTS_COEF(nExponentFrac, nTemp1);
    nTemp1 = GN2_TO_X0 + MULT_DENTS_COEF(nExponentFrac, nTemp2);

    /*
    implement 2^(intPart) as
    a left shift for intPart >= 0 or
    a left shift for intPart <  0
    */
    if (nExponentInt >= 0)
    {
        /* left shift for positive exponents */
        /*lint -e{703} <avoid multiply for performance>*/
        nResult = nTemp1 << nExponentInt;
    }
    else
    {
        /* right shift for negative exponents */
        nExponentInt = -nExponentInt;
        nResult = nTemp1 >> nExponentInt;
    }

    return nResult;
}

#if (NUM_OUTPUT_CHANNELS == 2)
/*----------------------------------------------------------------------------
 * FM_CalculatePan()
 *----------------------------------------------------------------------------
 * Purpose:
 * Assign the left and right gain values corresponding to the given pan value.
 *
 * Inputs:
 * psVoice - ptr to the voice we have assigned for this channel
 * psArticulation - ptr to this voice's articulation
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 *
 * Side Effects:
 * the given voice's m_nGainLeft and m_nGainRight are assigned
 *----------------------------------------------------------------------------
*/
static void FM_CalculatePan (EAS_I16 pan, EAS_U16 *pGainLeft, EAS_U16 *pGainRight)
{
    EAS_I32 nTemp;
    EAS_INT nNetAngle;

    /*
    Implement the following
    sin(x) = (2-4*c)*x^2 + c + x
    cos(x) = (2-4*c)*x^2 + c - x

      where  c = 1/sqrt(2)
    using the a0 + x*(a1 + x*a2) approach
    */

    /*
    Get the Midi CC10 pan value for this voice's channel
    convert the pan value to an "angle" representation suitable for
    our sin, cos calculator. This representation is NOT necessarily the same
    as the transform in the GM manuals because of our sin, cos calculator.
    "angle" = (CC10 - 64)/128
    */
    /*lint -e{703} <avoid multiply for performance reasons>*/
    nNetAngle = ((EAS_I32) pan) << (NUM_EG1_FRAC_BITS -7);

    /* calculate sin */
    nTemp = EG1_ONE + FMUL_15x15(COEFF_PAN_G2, nNetAngle);
    nTemp = COEFF_PAN_G0 + FMUL_15x15(nTemp, nNetAngle);

    if (nTemp > SYNTH_FULL_SCALE_EG1_GAIN)
        nTemp = SYNTH_FULL_SCALE_EG1_GAIN;
    else if (nTemp < 0)
        nTemp = 0;

    *pGainRight = (EAS_U16) nTemp;

    /* calculate cos */
    nTemp = -EG1_ONE + FMUL_15x15(COEFF_PAN_G2, nNetAngle);
    nTemp = COEFF_PAN_G0 + FMUL_15x15(nTemp, nNetAngle);

    if (nTemp > SYNTH_FULL_SCALE_EG1_GAIN)
        nTemp = SYNTH_FULL_SCALE_EG1_GAIN;
    else if (nTemp < 0)
        nTemp = 0;

    *pGainLeft = (EAS_U16) nTemp;
}
#endif /* #if (NUM_OUTPUT_CHANNELS == 2) */

/*----------------------------------------------------------------------------
 * FM_Operator()
 *----------------------------------------------------------------------------
 * Purpose:
 * Synthesizes a buffer of samples based on passed parameters.
 *
 * Inputs:
 * nNumSamplesToAdd - number of samples to synthesize
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 *
 * Side Effects:
 *
 *----------------------------------------------------------------------------
*/
void FM_Operator (
        S_FM_ENG_OPER *p,
        EAS_I32 numSamplesToAdd,
        EAS_PCM *pBuffer,
        EAS_PCM *pModBuffer,
        EAS_BOOL mix,
        EAS_U16 gainTarget,
        EAS_I16 pitch,
        EAS_U8 feedback,
        EAS_I16 *pLastOutput)
{
    EAS_I32 gain;
    EAS_I32 gainInc;
    EAS_U32 phase;
    EAS_U32 phaseInc;
    EAS_U32 phaseTemp;
    EAS_I32 temp;
    EAS_I32 temp2;

    /* establish local gain variable */
    gain = (EAS_I32) p->gain << 16;

    /* calculate gain increment */
    /*lint -e{703} use shift for performance */
    gainInc = ((EAS_I32) gainTarget - (EAS_I32) p->gain) << (16 - SYNTH_UPDATE_PERIOD_IN_BITS);

    /* establish local phase variables */
    phase = p->phase;

    /* calculate the new phase increment */
    phaseInc = (EAS_U32) FM_PhaseInc(pitch);

    /* restore final output from previous frame for feedback loop */
    if (pLastOutput)
        temp = *pLastOutput;
    else
        temp = 0;

    /* generate a buffer of samples */
    while (numSamplesToAdd--)
    {

        /* incorporate modulation */
        if (pModBuffer)
        {
            /*lint -e{701} use shift for performance */
            temp = *pModBuffer++ << FM_MODULATOR_INPUT_SHIFT;
        }

        /* incorporate feedback */
        else
        {
            /*lint -e{703} use shift for performance */
            temp = (temp * (EAS_I32) feedback) << FM_FEEDBACK_SHIFT;
        }

        /*lint -e{737} <use this behavior to avoid extra mask step> */
        phaseTemp = phase + temp;

        /* fetch sample from wavetable */
        temp = sineTable[phaseTemp >> (32 - SINE_TABLE_SIZE_IN_BITS)];

        /* increment operator phase */
        phase += phaseInc;

        /* internal gain for modulation effects */
        temp = FMUL_15x15(temp, (gain >> 16));

        /* output gain calculation */
        temp2 = FMUL_15x15(temp, p->outputGain);

        /* saturating add to buffer */
        if (mix)
        {
            temp2 += *pBuffer;
            *pBuffer++ = FM_Saturate(temp2);
        }

        /* output to buffer */
        else
            *pBuffer++ = (EAS_I16) temp2;

        /* increment gain */
        gain += gainInc;

    }

    /* save phase and gain */
    p->phase = phase;
    p->gain = gainTarget;

    /* save last output for feedback in next frame */
    if (pLastOutput)
        *pLastOutput = (EAS_I16) temp;
}

/*----------------------------------------------------------------------------
 * FM_NoiseOperator()
 *----------------------------------------------------------------------------
 * Purpose:
 * Synthesizes a buffer of samples based on passed parameters.
 *
 * Inputs:
 * nNumSamplesToAdd - number of samples to synthesize
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 *
 * Side Effects:
 *
 *----------------------------------------------------------------------------
*/
void FM_NoiseOperator (
        S_FM_ENG_OPER *p,
        EAS_I32 numSamplesToAdd,
        EAS_PCM *pBuffer,
        EAS_BOOL mix,
        EAS_U16 gainTarget,
        EAS_U8 feedback,
        EAS_I16 *pLastOutput)
{
    EAS_I32 gain;
    EAS_I32 gainInc;
    EAS_U32 phase;
    EAS_I32 temp;
    EAS_I32 temp2;

    /* establish local gain variable */
    gain = (EAS_I32) p->gain << 16;

    /* calculate gain increment */
    /*lint -e{703} use shift for performance */
    gainInc = ((EAS_I32) gainTarget - (EAS_I32) p->gain) << (16 - SYNTH_UPDATE_PERIOD_IN_BITS);

    /* establish local phase variables */
    phase = p->phase;

    /* establish local phase variables */
    phase = p->phase;

    /* recall last sample for filter Z-1 term */
    temp = 0;
    if (pLastOutput)
        temp = *pLastOutput;

    /* generate a buffer of samples */
    while (numSamplesToAdd--)
    {

        /* if using filter */
        if (pLastOutput)
        {
            /* use PRNG for noise */
            temp2 = FM_Noise(&phase);

            /*lint -e{704} use shift for performance */
            temp += ((temp2 -temp) * feedback) >> 8;
        }
        else
        {
            temp = FM_Noise(&phase);
        }

        /* internal gain for modulation effects */
        temp2 = FMUL_15x15(temp, (gain >> 16));

        /* output gain calculation */
        temp2 = FMUL_15x15(temp2, p->outputGain);

        /* saturating add to buffer */
        if (mix)
        {
            temp2 += *pBuffer;
            *pBuffer++ = FM_Saturate(temp2);
        }

        /* output to buffer */
        else
            *pBuffer++ = (EAS_I16) temp2;

        /* increment gain */
        gain += gainInc;

    }

    /* save phase and gain */
    p->phase = phase;
    p->gain = gainTarget;

    /* save last output for feedback in next frame */
    if (pLastOutput)
        *pLastOutput = (EAS_I16) temp;
}

/*----------------------------------------------------------------------------
 * FM_ConfigVoice()
 *----------------------------------------------------------------------------
 * Purpose:
 * Receives parameters to start a new voice.
 *
 * Inputs:
 * voiceNum     - voice number to start
 * vCfg         - configuration data
 * pMixBuffer   - pointer to host supplied buffer
 *
 * Outputs:
 *
 * Side Effects:
 *
 * Notes:
 * pFrameBuffer is not used in the test version, but is passed as a
 * courtesy to split architecture implementations. It can be used as
 * as pointer to the interprocessor communications buffer when the
 * synthesis parameters are passed off to a DSP for synthesis.
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, pFrameBuffer) pFrameBuffer not used in test version - see above */
void FM_ConfigVoice (EAS_I32 voiceNum, S_FM_VOICE_CONFIG *vCfg, EAS_FRAME_BUFFER_HANDLE pFrameBuffer)
{
    S_FM_ENG_VOICE *pVoice;
    EAS_INT i;

    /* establish pointer to voice data */
    pVoice = &voices[voiceNum];

    /* save data */
    pVoice->feedback = vCfg->feedback;
    pVoice->flags = vCfg->flags;
    pVoice->voiceGain = vCfg->voiceGain;

    /* initialize Z-1 terms */
    pVoice->op1Out = 0;
    pVoice->op3Out = 0;

    /* initialize operators */
    for (i = 0; i < 4; i++)
    {
        /* save operator data */
        pVoice->oper[i].gain = vCfg->gain[i];
        pVoice->oper[i].outputGain = vCfg->outputGain[i];
        pVoice->oper[i].outputGain = vCfg->outputGain[i];

        /* initalize operator */
        pVoice->oper[i].phase = 0;
    }

    /* calculate pan */
#if NUM_OUTPUT_CHANNELS == 2
    FM_CalculatePan(vCfg->pan, &pVoice->gainLeft, &pVoice->gainRight);
#endif
}

/*----------------------------------------------------------------------------
 * FM_ProcessVoice()
 *----------------------------------------------------------------------------
 * Purpose:
 * Synthesizes a buffer of samples based on calculated parameters.
 *
 * Inputs:
 * nNumSamplesToAdd - number of samples to synthesize
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 *
 * Side Effects:
 *
 * Notes:
 * pOut is not used in the test version, but is passed as a
 * courtesy to split architecture implementations. It can be used as
 * as pointer to the interprocessor communications buffer when the
 * synthesis parameters are passed off to a DSP for synthesis.
 *----------------------------------------------------------------------------
*/
/*lint -esym(715, pOut) pOut not used in test version - see above */
void FM_ProcessVoice (
        EAS_I32 voiceNum,
        S_FM_VOICE_FRAME *pFrame,
        EAS_I32 numSamplesToAdd,
        EAS_PCM *pTempBuffer,
        EAS_PCM *pBuffer,
        EAS_I32 *pMixBuffer,
        EAS_FRAME_BUFFER_HANDLE pFrameBuffer)
{
    S_FM_ENG_VOICE *p;
    EAS_PCM *pOutBuf;
    EAS_PCM *pMod;
    EAS_BOOL mix;
    EAS_U8 feedback1;
    EAS_U8 feedback3;
    EAS_U8 mode;

    /* establish pointer to voice data */
    p = &voices[voiceNum];
    mode = p->flags & 0x07;

    /* lookup feedback values */
    feedback1 = fmScaleTable[p->feedback >> 4];
    feedback3 = fmScaleTable[p->feedback & 0x0f];

    /* operator 3 is on output bus in modes 0, 1, and 3 */
    if ((mode == 0) || (mode == 1) || (mode == 3))
        pOutBuf = pBuffer;
    else
        pOutBuf = pTempBuffer;

    if (p->flags & FLAG_FM_ENG_VOICE_OP3_NOISE)
    {
        FM_NoiseOperator(
                p->oper + 2,
                numSamplesToAdd,
                pOutBuf,
                EAS_FALSE,
                pFrame->gain[2],
                feedback3,
                &p->op3Out);
    }
    else
    {
        FM_Operator(
                p->oper + 2,
                numSamplesToAdd,
                pOutBuf,
                0,
                EAS_FALSE,
                pFrame->gain[2],
                pFrame->pitch[2],
                feedback3,
                &p->op3Out);
    }

    /* operator 4 is on output bus in modes 0, 1, and 2 */
    if (mode < 3)
        pOutBuf = pBuffer;
    else
        pOutBuf = pTempBuffer;

    /* operator 4 is modulated in modes 2, 4, and 5 */
    if ((mode == 2) || (mode == 4) || (mode == 5))
        pMod = pTempBuffer;
    else
        pMod = 0;

    /* operator 4 is in mix mode in modes 0 and 1 */
    mix = (mode < 2);

    if (p->flags & FLAG_FM_ENG_VOICE_OP4_NOISE)
    {
        FM_NoiseOperator(
                p->oper + 3,
                numSamplesToAdd,
                pOutBuf,
                mix,
                pFrame->gain[3],
                0,
                0);
    }
    else
    {
        FM_Operator(
                p->oper + 3,
                numSamplesToAdd,
                pOutBuf,
                pMod,
                mix,
                pFrame->gain[3],
                pFrame->pitch[3],
                0,
                0);
    }

    /* operator 1 is on output bus in mode 0 */
    if (mode == 0)
        pOutBuf = pBuffer;
    else
        pOutBuf = pTempBuffer;

    /* operator 1 is modulated in modes 3 and 4 */
    if ((mode == 3) || (mode == 4))
        pMod = pTempBuffer;
    else
        pMod = 0;

    /* operator 1 is in mix mode in modes 0 and 5 */
    mix = ((mode == 0) || (mode == 5));

    if (p->flags & FLAG_FM_ENG_VOICE_OP1_NOISE)
    {
        FM_NoiseOperator(
                p->oper,
                numSamplesToAdd,
                pOutBuf,
                mix,
                pFrame->gain[0],
                feedback1,
                &p->op1Out);
    }
    else
    {
        FM_Operator(
                p->oper,
                numSamplesToAdd,
                pOutBuf,
                pMod,
                mix,
                pFrame->gain[0],
                pFrame->pitch[0],
                feedback1,
                &p->op1Out);
    }

    /* operator 2 is modulated in all modes except 0 */
    if (mode != 0)
        pMod = pTempBuffer;
    else
        pMod = 0;

    /* operator 1 is in mix mode in modes 0 -3 */
    mix = (mode < 4);

    if (p->flags & FLAG_FM_ENG_VOICE_OP2_NOISE)
    {
        FM_NoiseOperator(
                p->oper + 1,
                numSamplesToAdd,
                pBuffer,
                mix,
                pFrame->gain[1],
                0,
                0);
    }
    else
    {
        FM_Operator(
                p->oper + 1,
                numSamplesToAdd,
                pBuffer,
                pMod,
                mix,
                pFrame->gain[1],
                pFrame->pitch[1],
                0,
                0);
    }

    /* mix voice output to synthesizer output buffer */
    FM_SynthMixVoice(p, pFrame->voiceGain, numSamplesToAdd, pBuffer, pMixBuffer);
}

/*----------------------------------------------------------------------------
 * FM_SynthMixVoice()
 *----------------------------------------------------------------------------
 * Purpose:
 * Mixes the voice output buffer into the final mix using an anti-zipper
 * filter.
 *
 * Inputs:
 * nNumSamplesToAdd - number of samples to synthesize
 * psEASData - pointer to overall EAS data structure
 *
 * Outputs:
 *
 * Side Effects:
 *
 *----------------------------------------------------------------------------
*/
void FM_SynthMixVoice(S_FM_ENG_VOICE *p,  EAS_U16 nGainTarget, EAS_I32 numSamplesToAdd, EAS_PCM *pInputBuffer, EAS_I32 *pBuffer)
{
    EAS_I32 nGain;
    EAS_I32 nGainInc;
    EAS_I32 nTemp;

    /* restore previous gain */
    /*lint -e{703} <use shift for performance> */
    nGain = (EAS_I32) p->voiceGain << 16;

    /* calculate gain increment */
    /*lint -e{703} <use shift for performance> */
    nGainInc = ((EAS_I32) nGainTarget - (EAS_I32) p->voiceGain) << (16 - SYNTH_UPDATE_PERIOD_IN_BITS);

    /* mix the output buffer */
    while (numSamplesToAdd--)
    {
        /* output gain calculation */
        nTemp = *pInputBuffer++;

        /* sum to output buffer */
#if (NUM_OUTPUT_CHANNELS == 2)

        /*lint -e{704} <use shift for performance> */
        nTemp = ((EAS_I32) nTemp * (nGain >> 16)) >> FM_GAIN_SHIFT;

        /*lint -e{704} <use shift for performance> */
        {
            EAS_I32 nTemp2;
            nTemp = nTemp >> FM_STEREO_PRE_GAIN_SHIFT;
            nTemp2 = (nTemp * p->gainLeft) >> FM_STEREO_POST_GAIN_SHIFT;
            *pBuffer++ += nTemp2;
            nTemp2 = (nTemp * p->gainRight) >> FM_STEREO_POST_GAIN_SHIFT;
            *pBuffer++ += nTemp2;
        }
#else
        /*lint -e{704} <use shift for performance> */
        nTemp = ((EAS_I32) nTemp * (nGain >> 16)) >> FM_MONO_GAIN_SHIFT;
        *pBuffer++ += nTemp;
#endif

        /* increment gain for anti-zipper filter */
        nGain += nGainInc;
    }

    /* save gain */
    p->voiceGain = nGainTarget;
}

