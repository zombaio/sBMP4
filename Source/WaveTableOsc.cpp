/*
 ==============================================================================
 sBMP4: killer subtractive synth!
 
 Copyright (C) 2016  BMP4
 
 Developer: Vincent Berthiaume, building on code from Nigel Redmon, see original license below.
 
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

 //
//  WaveTableOsc.cpp
//
//  Created by Nigel Redmon on 5/15/12
//  EarLevel Engineering: earlevel.com
//  Copyright 2012 Nigel Redmon
//
//  For a complete explanation of the wavetable oscillator and code,
//  read the series of articles by the author, starting here:
//  www.earlevel.com/main/2012/05/03/a-wavetable-oscillator—introduction/
//
//  License:
//
//  This source code is provided as is, without warranty.
//  You may copy and distribute verbatim copies of this document.
//  You may modify and use this source code to create binary code for your own purposes, free or commercial.
//

#include "WaveTableOsc.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "constants.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// set to a large number (greater than or equal to the length of the lowest octave table) for constant table size; 
//set to 0 for a constant oversampling ratio (each higher ocatave table length reduced by half); set somewhere
//between (64, for instance) for constant oversampling but with a minimum limit
#define constantRatioLimit (99999)    

using namespace std;

// I grabbed (and slightly modified) this code from Rabiner & Gold (1975), After Cooley, Lewis, and Welch; 
void WaveTableOsc::fft(int N) {    
    int i, j, k, L;            /* indexes */
    int M, TEMP, LE, LE1, ip;  /* M = log N */
    int NV2, NM1;
    double t;               /* temp */
    double Ur, Ui, Wr, Wi, Tr, Ti;
    double Ur_old;
    
    // if ((N > 1) && !(N & (N - 1)))   // make sure we have a power of 2
    NV2 = N >> 1;
    NM1 = N - 1;
    TEMP = N; /* get M = log N */
    M = 0;
    while (TEMP >>= 1){
		++M;
	}
    /* shuffle */
    j = 1;
    for (i = 1; i <= NM1; i++) {
        if(i<j) {             /* swap a[i] and a[j] */
            t = m_vPartials[j-1];     
            m_vPartials[j-1] = m_vPartials[i-1];
            m_vPartials[i-1] = t;
            t = m_vWave[j-1];
            m_vWave[j-1] = m_vWave[i-1];
            m_vWave[i-1] = t;
        }
        k = NV2;             /* bit-reversed counter */
        while(k < j) {
            j -= k;
            k /= 2;
        }  
        j += k;
    }
    LE = 1.;
    for (L = 1; L <= M; L++) {            // stage L
        LE1 = LE;                         // (LE1 = LE/2) 
        LE *= 2;                          // (LE = 2^L)
        Ur = 1.0;
        Ui = 0.; 
        Wr = cos(M_PI/(float)LE1);
        Wi = -sin(M_PI/(float)LE1); // Cooley, Lewis, and Welch have "+" here
        for (j = 1; j <= LE1; j++) {
            for (i = j; i <= N; i += LE) { // butterfly
                ip = i+LE1;
                Tr = m_vPartials[ip-1] * Ur - m_vWave[ip-1] * Ui;
                Ti = m_vPartials[ip-1] * Ui + m_vWave[ip-1] * Ur;
                m_vPartials[ip-1] = m_vPartials[i-1] - Tr;
                m_vWave[ip-1] = m_vWave[i-1] - Ti;
                m_vPartials[i-1]  = m_vPartials[i-1] + Tr;
                m_vWave[i-1]  = m_vWave[i-1] + Ti;
            }
            Ur_old = Ur;
            Ur = Ur_old * Wr - Ui * Wi;
            Ui = Ur_old * Wi + Ui * Wr;
        }
    }
}

WaveTableOsc::WaveTableOsc(const int sampleRate, const WaveTypes waveType):
	phasor(0.0)			// phase accumulator
    , phaseInc(0.0)		// phase increment
    , phaseOfs(0.5)		// phase offset for PWM
    , numWaveTables(0)	//why is this 0?
	{
	//initialize the array of waveTable structures (which could be replaced by vectors...)
    for (int idx = 0; idx < numWaveTableSlots; idx++) {
        m_oWaveTables[idx].topFreq = 0;
        m_oWaveTables[idx].waveTableLen = 0;
    }

	//TODO: understand this
    // calc number of harmonics where the highest harmonic baseFreq and lowest alias an octave higher would meet
    int maxHarms = sampleRate / (3.0 * k_iBaseFrequency) + 0.5;	//maxHarms = 735

	//TODO: find less opaque way of doing this, check aspma notes
    // round up to nearest power of two, for fft size
    unsigned int v = maxHarms;
    v--;            // so we don't go up if already a power of 2
    v |= v >> 1;    // roll the highest bit into all lower bits...
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;            // and increment to power of 2
    int tableLen = v * 2 * k_iOverSampleFactor;  // double for the sample rate, then oversampling, tablelen = 4096
    // for ifft
	m_vPartials = vector<double>(tableLen, 0.);
	m_vWave		= vector<double>(tableLen, 0.);

	//calculate topFrequency based on Nyquist and base frequency... 
	//TODO: why is base frequency relevant here?
    double topFreq = k_iBaseFrequency * 2.0 / sampleRate;	//topFreq = 0.00090702947845804993
    double scale = 0.0;
    for (; maxHarms >= 1; maxHarms /= 2) {
		//fill m_vPartials with partial amplitudes for a sawtooth. This will be ifft'ed to get a wave
		switch (waveType){
			case triangleWave:
				JUCE_COMPILER_WARNING("these should be called something like getPartials")
				defineTrianglePartials(tableLen, maxHarms);
				break;
			case sawtoothWave:
				defineSawtoothPartials(tableLen, maxHarms);	
				break;
			case squareWave:
				defineSquarePartials(tableLen, maxHarms);	
				break;
			default:
				jassertfalse;
				break;
		}

		//from the m_vPartials partials, make a wave in m_vWave, then store it in osc. keep scale so that we can reuse it for the next maxHarm, so that we have a normalized volume accross wavetables
		scale = makeWaveTable(tableLen, scale, topFreq);
        topFreq *= 2;
		//not sure, doesn't matter, not hit with default values
        if (tableLen > constantRatioLimit){ // variable table size (constant oversampling but with minimum table size)
            tableLen /= 2;
		}
    }
}

// if scale is 0, auto-scales
// returns scaling factor (0.0 if failure), and wavetable in m_vWave array
float WaveTableOsc::makeWaveTable(int len, double scale, double topFreq) {
    fft(len);	//after this, m_vWave contains the wave form, produced by an ifft I assume, and m_vPartials contains... noise? see waveTableOscFFtOutput.xlsx in dropbox/sBMP4
    //if no scale was supplied, find maximum sample amplitude, then derive scale
    if (scale == 0.0) {
        // calc normal
        double max = 0;
        for (int idx = 0; idx < len; idx++) {
            double temp = fabs(m_vWave[idx]);
            if (max < temp)
                max = temp;
        }
        scale = 1.0 / max * .999;        
    }
    
    // normalize
    vector<float> wave(len);
    for (int idx = 0; idx < len; idx++)
        wave[idx] = m_vWave[idx] * scale;
        
    if (addWaveTable(len, wave, topFreq))
        scale = 0.0;
    
    return scale;
}

// prepares sawtooth harmonics for ifft
void WaveTableOsc::defineSawtoothPartials(int len, int numHarmonics){
	if(numHarmonics > (len / 2)){
		numHarmonics = (len / 2);
	}
	for(int idx = 0; idx < len; idx++){
		m_vWave[idx] = 0;
		m_vPartials[idx] = 0;
	}
	//fill the m_vPartials vector, which I presume is the amplitude of real harmonics
	for(int idx = 1, jdx = len - 1; idx <= numHarmonics; idx++, jdx--){
		double temp = -1.0 / idx;	//for sawtooh, harmonic amplitude decreases as their index increases.
		m_vPartials[idx] = -temp;			//the firt half will be positive
		m_vPartials[jdx] = temp;				//the second half negative... why?
	}
}

// prepares sawtooth harmonics for ifft
void WaveTableOsc::defineSquarePartials(int len, int numHarmonics) {
	if(numHarmonics > (len / 2)){
		numHarmonics = (len / 2);
	}
	for(int idx = 0; idx < len; idx++){
		m_vWave[idx] = 0;
		m_vPartials[idx] = 0;
	}
	//fill the m_vPartials vector, which I presume is the amplitude of real harmonics
	for(int idx = 1, jdx = len - 1; idx <= numHarmonics; idx++, jdx--){
		double temp = idx & 0x01 ? 1.0 / idx : 0.0;
		m_vPartials[idx] = -temp;
		m_vPartials[jdx] = temp;
	}
}

// prepares sawtooth harmonics for ifft
void WaveTableOsc::defineTrianglePartials(int len, int numHarmonics){
	if(numHarmonics > (len / 2)){
		numHarmonics = (len / 2);
	}
	for(int idx = 0; idx < len; idx++){
		m_vWave[idx] = 0;
		m_vPartials[idx] = 0;
	}
	//fill the m_vPartials vector, which I presume is the amplitude of real harmonics
	float sign = 1;
	for(int idx = 1, jdx = len - 1; idx <= numHarmonics; idx++, jdx--){
		double temp = idx & 0x01 ? 1.0 / (idx * idx) * (sign = -sign) : 0.0;
		m_vPartials[idx] = -temp;
		m_vPartials[jdx] = temp;
	}
}


//
// addWaveTable
//
// add wavetables in order of lowest frequency to highest
// topFreq is the highest frequency supported by a wavetable
// wavetables within an oscillator can be different lengths
//
// returns 0 upon success, or the number of wavetables if no more room is available
//
int WaveTableOsc::addWaveTable(int len, std::vector<float> waveTableIn, double topFreq) {
    if (numWaveTables < numWaveTableSlots) {
		m_oWaveTables[numWaveTables].waveTable = vector<float>(len);
        m_oWaveTables[numWaveTables].waveTableLen = len;
        m_oWaveTables[numWaveTables].topFreq = topFreq;
        
        // fill in wave
        for (long idx = 0; idx < len; idx++){
            m_oWaveTables[numWaveTables].waveTable[idx] = waveTableIn[idx];
		}

		++numWaveTables;
        return 0;
    }
    return numWaveTables;
}

//
// getOutput
//
// returns the current oscillator output
//
float WaveTableOsc::getOutput() {
    // grab the appropriate wavetable
    int waveTableIdx = 0;
	//TODO: why is phaseInc compared to the topFreq?
    while ((phaseInc >= m_oWaveTables[waveTableIdx].topFreq) && (waveTableIdx < (numWaveTables - 1))) {
        ++waveTableIdx;
    }
    waveTable *waveTable = &m_oWaveTables[waveTableIdx];

#if !doLinearInterp
    // truncate
    return waveTable->waveTable[int(phasor * waveTable->waveTableLen)];
#else
    // linear interpolation
    double temp = phasor * waveTable->waveTableLen;
    int intPart = temp;
    double fracPart = temp - intPart;
    float samp0 = waveTable->waveTable[intPart];
    if (++intPart >= waveTable->waveTableLen)
        intPart = 0;
    float samp1 = waveTable->waveTable[intPart];
    
    return k_fWaveTableGain * (samp0 + (samp1 - samp0) * fracPart);
#endif
}


//
// getOutputMinusOffset
//
// for variable pulse width: initialize to sawtooth,
// set phaseOfs to duty cycle, use this for osc output
//
// returns the current oscillator output
//
float WaveTableOsc::getOutputMinusOffset() {
    // grab the appropriate wavetable
    int waveTableIdx = 0;
    while ((this->phaseInc >= this->m_oWaveTables[waveTableIdx].topFreq) && (waveTableIdx < (this->numWaveTables - 1))) {
        ++waveTableIdx;
    }
    waveTable *waveTable = &this->m_oWaveTables[waveTableIdx];
    
#if !doLinearInterp
    // truncate
    double offsetPhasor = this->phasor + this->phaseOfs;
    if (offsetPhasor >= 1.0)
        offsetPhasor -= 1.0;
    return waveTable->waveTable[int(this->phasor * waveTable->waveTableLen)] - waveTable->waveTable[int(offsetPhasor * waveTable->waveTableLen)];
#else
    // linear
    double temp = this->phasor * waveTable->waveTableLen;
    int intPart = temp;
    double fracPart = temp - intPart;
    float samp0 = waveTable->waveTable[intPart];
    if (++intPart >= waveTable->waveTableLen)
        intPart = 0;
    float samp1 = waveTable->waveTable[intPart];
    float samp = samp0 + (samp1 - samp0) * fracPart;
    
    // and linear again for the offset part
    double offsetPhasor = this->phasor + this->phaseOfs;
    if (offsetPhasor > 1.0)
        offsetPhasor -= 1.0;
    temp = offsetPhasor * waveTable->waveTableLen;
    intPart = temp;
    fracPart = temp - intPart;
    samp0 = waveTable->waveTable[intPart];
    if (++intPart >= waveTable->waveTableLen)
        intPart = 0;
    samp1 = waveTable->waveTable[intPart];
    
    return k_fWaveTableGain * (samp - (samp0 + (samp1 - samp0) * fracPart));
#endif
}
