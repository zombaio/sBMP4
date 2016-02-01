/*
 ==============================================================================
 sBMP4: kilker subtractive synth!
 
 Copyright (C) 2014  BMP4
 
 Developer: Vincent Berthiaume
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ==============================================================================
 */

#ifndef sBMP4_Header_h
#define sBMP4_Header_h

//convert p_tValue01 from range [0,1] to human-readable range [p_tMinHr, p_tMaxHr]
template <typename T>
static T const& convert01ToHr (T const& p_tValue01, T const& p_tMinHr, T const& p_tMaxHr) { 
    return p_tValue01*(p_tMaxHr-p_tMinHr) + p_tMinHr; 
} 

//convert p_tValue01 from human-readable range [p_tMinHr, p_tMaxHr] to range [0,1] 
template <typename T>
static T const& convertHrTo01 (T const& p_tValueHr, T const& p_tMinHr, T const& p_tMaxHr) { 
    return (p_tValueHr - p_tMinHr) / (p_tMaxHr-p_tMinHr); 
} 

enum Parameters{
     paramGain = 0
    ,paramDelay
    ,paramWave
	,paramFilterFr
	,paramLfoFr
	,paramQ
	,paramLfoOn
    ,paramTotalNum
};

const float defaultGain			= 1.0f;
const float defaultDelay		= 0.0f;
const float defaultWave			= 0.0f;

//----FILTER FR
const float defaultFilterFr		= 0.0f;

//----FILTER Q
const float k_fMinQHr			= 0.01f;
const float k_fMaxQHr			= 20.f;
const float k_fDefaultQHr		= 5.f;
const float k_fDefaultQ01		= convertHrTo01(k_fDefaultQHr, k_fMinQHr, k_fMaxQHr);

//----LFO
const float k_fMinLfoFr			= 0.5f;
const float k_fMaxLfoFr			= 40.f;
const float k_fDefaultLfoFrHr	= 2.;
const float k_fDefaultLfoFr01	= convertHrTo01(k_fDefaultLfoFrHr, k_fMinLfoFr, k_fMaxLfoFr);

const float k_fDefaultLfoOn		= 0.;

const int   s_iSimpleFilterLF = 600;
const int   s_iSimpleFilterHF = 20000;// 12000;
const int   s_iNumberOfVoices = 5;

#if	WIN32
const bool  s_bUseSimplestLp = false;
#else
const bool  s_bUseSimplestLp = true;
#endif

static bool areSame(double a, double b){
    return fabs(a - b) < .0001;//std::numeric_limits<double>::epsilon();
}

//-------stuff related to wavetables
const bool  s_bUseWaveTables = true;

JUCE_COMPILER_WARNING("Reasoning for finding k_iTotalWaveFrames. THIS CANNOT BE HARDCODED");
float k_fWavTableFr = 100.f;	//frequency of wave stored in wave table

//float fT = 1/fFr;	//period
float k_fSampleRate = 44100;	
//k_iTotalWaveFrames = k_fSampleRate * fT = 441;
const int   k_iTotalWaveFrames = 441;

//-------stuff related to size of things
const int s_iXMargin		= 20;
const int s_iYMargin		= 25;
const int s_iKeyboardHeight	= 70;
const int s_iSliderWidth	= 75;
const int s_iSliderHeight	= 40;
const int s_iLabelHeight	= 20;
const int s_iLogoW			= 75;
const int s_iLogoH			= 30;

const int s_iNumberOfHorizontalSliders	= 4;
const int s_iNumberOfVerticaltalSliders = 2;

#endif
