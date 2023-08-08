#pragma once
#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/oled_ssd130x.h"

enum Switches
{
    FS_1, 
    FS_2,
    SWITCH_LAST
};
enum Knobs
{
    knob_1, 
    knob_2, 
    knob_3, 
    knob_4, 
    knob_5, 
    knob_6,
    KNOB_LAST
};
enum class EffectType
{
    Looper,
    Delay,
    Phaser,
    Chorus,
    Flanger,
    Tremolo,
    Overdrive,
    Compressor,
    Bitcrusher,
    Wavefolder,
    Reverb,
    Resonator,
    Wah
};
enum menuItems
{
    SignalChain,
    Looper,
    Delay,
    Phaser,
    Chorus,
    Flanger,
    Tremolo,
    Overdrive,
    Compressor,
    Bitcrusher,
    Wavefolder,
    Reverb,
    Resonator,
    Wah
};

// VARIABLES AND OBJECTS ==========================================================

static daisy::Encoder Encoder;
std::vector<menuItems> menuItemVector = {menuItems::SignalChain,menuItems::Looper,menuItems::Delay,menuItems::Phaser,menuItems::Chorus,menuItems::Flanger,
                                         menuItems::Tremolo,menuItems::Overdrive,menuItems::Compressor,menuItems::Bitcrusher,
                                         menuItems::Wavefolder,menuItems::Reverb,menuItems::Resonator, menuItems::Wah};

daisysp::Phaser phaser; //2
daisysp::Chorus chorus; //3
daisysp::Flanger flanger; //4
daisysp::Tremolo trem; //5
daisysp::Overdrive overd; //6
daisysp::Bitcrush bitcr; //8
daisysp::Wavefolder fold; //9
daisysp::ReverbSc rev; //10
daisysp::Compressor comp;
daisysp::Resonator resonator;
daisysp::Autowah wah;

std::vector<EffectType> signalChain;
menuItems paramMode;
daisy::AnalogControl Knobs[KNOB_LAST];
daisy::Switch Switches[SWITCH_LAST];
using MyOledDisplay = daisy::OledDisplay<daisy::SSD130xI2c128x64Driver>;
MyOledDisplay display;

float masterGain = 1;
float delTime=200, delFeedback=.6f, delWet=.5f;
float phaserRate=.5, phaserNumPoles=2 , phaserDepth=.5 , phaserLFOFreq=.5, phaserWet=.7f, phaserFeedback=.5f;
float chorusFreq =.5, chorusDepth=.8f, chorusFeedback=.5f, chorusDelay=10, chorusWet=.7f; 
float flangerFreq=1, flangerDepth=.5f , flangerFeedback=.5f , flangerDelay=5, flangerWet=.5f;
float tremRate=10, tremDepth=.8f, tremWet=.8f;
float drive=.3f, driveWet=.5f;
float bitDepth=4, crushRate=8000, crushWet=.5f;
float foldGain=-20.f , foldOffset=5.f, foldWet=.5f;
float revFeedback=.6f, revCutoff=10000; 
float compWet=.5f, compRatio=20.f, compAttack=5.f, compThresh=-40.f, compMakeup=40.f, compRelease=5.f;
float resWet=.3f, resFreq=200, resBright=.5f, resDamping=.4f, resStructure=.2f;
float wahWet=80, wahLevel=.7f, wahAmount=.7f;
float loopInLevel=1.f, loopLevel=1.f; bool stopLoopPlayback=false;
bool led_state = true;


// knob variables to be stored
float k1 = .5;
float k2 = 0;
float k3 = 0; 
float k4 = 0;
float k5 = 0;
float k6 = 0;