#include "LoopFX.h"

#define MAX_DELAY static_cast<size_t>(48000*2)
#define MAX_LOOP_SIZE (48000 * 60 * 5)


constexpr daisy::Pin PIN_FS1 =        daisy::seed::D26;
constexpr daisy::Pin PIN_FS2 =        daisy::seed::D25;
constexpr daisy::Pin PIN_KNOB1 =      daisy::seed::D21;
constexpr daisy::Pin PIN_KNOB2 =      daisy::seed::D20;
constexpr daisy::Pin PIN_KNOB3 =      daisy::seed::D19;
constexpr daisy::Pin PIN_KNOB4 =      daisy::seed::D18;
constexpr daisy::Pin PIN_KNOB5 =      daisy::seed::D17;
constexpr daisy::Pin PIN_KNOB6 =      daisy::seed::D16;
constexpr daisy::Pin PIN_ENC_DEC =    daisy::seed::D23;
constexpr daisy::Pin PIN_ENC_INC =    daisy::seed::D22;
constexpr daisy::Pin PIN_ENC_BUTTON = daisy::seed::D13;

//==========================================================================================
daisysp::DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS del; //1
daisysp::Looper looper;
float DSY_SDRAM_BSS loopBuffer[MAX_LOOP_SIZE];
//==========================================================================================
static daisy::DaisySeed hw;
static daisy::AdcHandle adc;
static daisy::DacHandle dac;
//==========================================================================================

bool isFirstEffect(EffectType effect)
{
    return !signalChain.empty() && signalChain.front() == effect;
}
//==========================================================================================

float scaleKnob (float knobValue, float min, float max)
{
    return knobValue * (max-min) + min;
}
//==========================================================================================

void conditionalParameter(float &oldValue, float newValue, float &param, float min=0, float max=1, bool isInt=false)
{
    if(abs(oldValue - newValue) > 0.005f)
    {   
        float scaledParam = scaleKnob(newValue, min, max);
        if (isInt) {param = (int)scaledParam;}
        else {param = scaledParam;}
		oldValue = newValue;

    }
}
//===========================================================================================

void InitOtherControls()
{
    Encoder.Init(PIN_ENC_INC, PIN_ENC_DEC, PIN_ENC_BUTTON);

    constexpr daisy::Pin pin_numbers[SWITCH_LAST] = {PIN_FS1, PIN_FS2};
    for(size_t i = 0; i < SWITCH_LAST; i++)
    {
        Switches[i].Init(pin_numbers[i]);
    }
}
void InitAnalogControls()
{
     
    daisy::AdcChannelConfig cfg[KNOB_LAST];
    cfg[knob_1].InitSingle(PIN_KNOB1);
    cfg[knob_2].InitSingle(PIN_KNOB2);
    cfg[knob_3].InitSingle(PIN_KNOB3);
    cfg[knob_4].InitSingle(PIN_KNOB4);
    cfg[knob_5].InitSingle(PIN_KNOB5);
    cfg[knob_6].InitSingle(PIN_KNOB6);
    hw.adc.Init(cfg, KNOB_LAST);
    // Make an array of pointers to the knob.
    for(int i = 0; i < KNOB_LAST; i++)
    {
        Knobs[i].Init(hw.adc.GetPtr(i), hw.AudioCallbackRate());
        Knobs[i].SetSampleRate(hw.AudioSampleRate());
    }
    
}
//==========================================================================================
void paint()
{
    daisy::System::Delay(10);
    std::string line1 = "Knob1Val= ";
    char* strptr = &line1[0];
    line1 += std::to_string(static_cast<uint32_t>(100*Knobs[knob_3].Value()));
    display.WriteString(strptr,Font_7x10, true);
    display.SetCursor(0, 0);
    display.Update();
}
void initOLED()
{
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.i2c_address               = 0x3C;
    disp_cfg.driver_config.transport_config.i2c_config.periph         = daisy::I2CHandle::Config::Peripheral::I2C_1;
    disp_cfg.driver_config.transport_config.i2c_config.speed          = daisy::I2CHandle::Config::Speed::I2C_100KHZ;
    disp_cfg.driver_config.transport_config.i2c_config.mode           = daisy::I2CHandle::Config::Mode::I2C_MASTER;
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = {DSY_GPIOB, 8};    disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = {DSY_GPIOB, 9};
    /** And Initialize */
    display.Init(disp_cfg);
    
}

//==========================================================================================

void processAnalogControls()
{
    
    for(size_t i = 0; i < KNOB_LAST; i++)
    {
        Knobs[i].Process();  
    }
}
//==========================================================================================

void setEffectValues()
{
    flanger.SetLfoDepth(flangerDepth);
	flanger.SetLfoFreq(flangerFreq);
    flanger.SetFeedback(flangerFeedback);
    flanger.SetDelayMs(flangerDelay);

    chorus.SetDelayMs(chorusDelay);
    chorus.SetFeedback(chorusFeedback);
    chorus.SetLfoDepth(chorusDepth);
    chorus.SetLfoFreq(chorusFreq);

    phaser.SetFeedback(phaserFeedback);
    phaser.SetFreq(phaserRate);
    phaser.SetLfoDepth(phaserDepth);
    phaser.SetLfoFreq(phaserLFOFreq);
    phaser.SetPoles(phaserNumPoles);

    comp.SetAttack(compAttack);
    comp.SetMakeup(compMakeup);
    comp.SetRatio(compRatio);
    comp.SetRelease(compRelease);
    comp.SetThreshold(compThresh);

    del.SetDelay(delTime*48000/1000);

    trem.SetDepth(tremDepth);
    trem.SetFreq(tremRate);

    overd.SetDrive(drive);
    
    fold.SetGain(foldGain);
    fold.SetOffset(foldOffset);

    bitcr.SetBitDepth(bitDepth);
    bitcr.SetCrushRate(crushRate);

    rev.SetFeedback(revFeedback);
    rev.SetLpFreq(revCutoff);  

    wah.SetDryWet(wahWet);
    wah.SetLevel(wahLevel);
    wah.SetWah(wahAmount);

    resonator.SetBrightness(resBright);
    resonator.SetDamping(resDamping);
    resonator.SetFreq(resFreq);
    resonator.SetStructure(resStructure);
}
//==========================================================================================
void processADC()
{
    switch (paramMode)
        {
            case menuItems::Delay:
                conditionalParameter(k1, Knobs[knob_1].Value(), delTime, 1.f, 2000.f);
                conditionalParameter(k2, Knobs[knob_2].Value(), delFeedback);
                conditionalParameter(k3, Knobs[knob_3].Value(), delWet);
                break;
            case menuItems::Phaser:
                conditionalParameter(k1, Knobs[knob_1].Value(), phaserRate, .01f, 15.f);
                conditionalParameter(k2, Knobs[knob_2].Value(), phaserDepth);
                conditionalParameter(k3, Knobs[knob_3].Value(), phaserLFOFreq, .01f, 15.f);
                conditionalParameter(k4, Knobs[knob_4].Value(), phaserNumPoles, 1, 8, true);
                conditionalParameter(k5, Knobs[knob_5].Value(), phaserFeedback);
                conditionalParameter(k6, Knobs[knob_6].Value(), phaserWet);                
                break;
            case menuItems::Chorus:
                conditionalParameter(k1, Knobs[knob_1].Value(), chorusFreq, .01f, 15.f);
                conditionalParameter(k2, Knobs[knob_2].Value(), chorusDepth);
                conditionalParameter(k3, Knobs[knob_3].Value(), chorusFeedback);
                conditionalParameter(k4, Knobs[knob_4].Value(), chorusDelay, 20.f, 50.f);
                conditionalParameter(k5, Knobs[knob_5].Value(), chorusWet);
                break;
            case menuItems::Flanger:
                conditionalParameter(k1, Knobs[knob_1].Value(), flangerFreq, .01f, 20.f);
                conditionalParameter(k2, Knobs[knob_2].Value(), flangerDepth);
                conditionalParameter(k3, Knobs[knob_3].Value(), flangerFeedback);
                conditionalParameter(k4, Knobs[knob_4].Value(), flangerDelay, .01f, 15.f);
                conditionalParameter(k5, Knobs[knob_5].Value(), flangerWet);
                break;
            case menuItems::Tremolo:
                conditionalParameter(k1, Knobs[knob_1].Value(), tremRate, .01f, 15.f);
                conditionalParameter(k2, Knobs[knob_2].Value(), tremDepth);
                conditionalParameter(k3, Knobs[knob_3].Value(), tremWet);

                break;
            case menuItems::Overdrive:
                conditionalParameter(k1, Knobs[knob_1].Value(), drive);
                conditionalParameter(k2, Knobs[knob_2].Value(), driveWet);
                
                break;
            case menuItems::Bitcrusher:
                conditionalParameter(k1, Knobs[knob_1].Value(), bitDepth,0,16,true);
                conditionalParameter(k2, Knobs[knob_2].Value(), crushRate, 0,48000, true);
                conditionalParameter(k3, Knobs[knob_3].Value(), crushWet);
                break;
            case menuItems::Wavefolder:
                conditionalParameter(k1, Knobs[knob_1].Value(), foldGain, -40.f, 0.f);
                conditionalParameter(k2, Knobs[knob_2].Value(), foldOffset, 0.f, 10.f);
                conditionalParameter(k3, Knobs[knob_3].Value(), foldWet);
                fold.SetOffset(0);
                break;
            case menuItems::Reverb:
                conditionalParameter(k1, Knobs[knob_1].Value(), revFeedback);
                conditionalParameter(k2, Knobs[knob_2].Value(), revCutoff, 0, 24000);
                break;
            case menuItems::Compressor:
                conditionalParameter(k1, Knobs[knob_1].Value(), compRatio,1.f, 40.f);
                conditionalParameter(k2, Knobs[knob_2].Value(), compAttack, .001f, 10.f);
                conditionalParameter(k3, Knobs[knob_3].Value(), compRelease, .001f, 10.f);
                conditionalParameter(k4, Knobs[knob_4].Value(), compMakeup, 0.f, 80.f);
                conditionalParameter(k5, Knobs[knob_5].Value(), compThresh, 0.f, -80.f);
                conditionalParameter(k6, Knobs[knob_6].Value(), compWet);  
                break;
            case menuItems::Resonator:
                conditionalParameter(k1, Knobs[knob_1].Value(), resDamping);
                conditionalParameter(k2, Knobs[knob_2].Value(), resBright);
                conditionalParameter(k3, Knobs[knob_3].Value(), resFreq, 20, 15000);
                conditionalParameter(k4, Knobs[knob_4].Value(), resStructure);
                conditionalParameter(k5, Knobs[knob_5].Value(), resWet);
                break;
            case menuItems::Wah:
                conditionalParameter(k1, Knobs[knob_1].Value(), wahAmount);
                conditionalParameter(k2, Knobs[knob_2].Value(), wahLevel);
                conditionalParameter(k3, Knobs[knob_3].Value(), wahWet, 0, 100);
                break;
            case menuItems::Looper:
                conditionalParameter(k1, Knobs[knob_1].Value(), loopInLevel, 0.f, 2.f);
                conditionalParameter(k2, Knobs[knob_2].Value(), loopLevel, 0.f, 2.f);
                break;
            case menuItems::SignalChain:
                k2 = Knobs[knob_2].Value();
                break;
        }

    setEffectValues(); 
}

//==========================================================================================
void processEffects(float input, float& output)
{
     for (EffectType effect : signalChain)  
     {
        switch (effect)
        {
            case EffectType::Delay:
                float del_out, sig_out, fback;
                // Read from delay line
                del_out = del.Read();
                // Calculate output and feedback
                if (isFirstEffect(effect))
                {
                    sig_out  = del_out + input;
                    fback = (del_out * delFeedback) + input;
                    del.Write(fback);
                    output += sig_out * delWet;
                }
                else 
                {
                    sig_out  = del_out + output;
                    fback = (del_out * delFeedback) + output;
                    del.Write(fback);
                    output += sig_out * delWet;
                }
                break;
            case EffectType::Phaser:
                if (isFirstEffect(effect)){output = input*(1-phaserWet) + phaser.Process(input)*phaserWet;}
                else {output = output*(1-phaserWet) + phaser.Process(output)*phaserWet;}
                break;
            case EffectType::Chorus:
                if (isFirstEffect(effect)){output = input*(1-chorusWet) + chorus.Process(input)*chorusWet;}
                else {output = output*(1-chorusWet) + chorus.Process(output)*chorusWet;}
                break;
            case EffectType::Flanger:
                if (isFirstEffect(effect)){output = input*(1-flangerWet) + flanger.Process(input)*flangerWet;}
                else {output = output*(1-flangerWet) + flanger.Process(output)*flangerWet;}
                break;
            case EffectType::Tremolo:
                if (isFirstEffect(effect)){output = input*(1-tremWet) + trem.Process(input)*tremWet;}
                else {output = output*(1-tremWet) + trem.Process(output)*tremWet;}
                break;
            case EffectType::Overdrive:
                if (isFirstEffect(effect)){output = input*(1-driveWet)+ overd.Process(input)*driveWet;}
                else {output = output*(1-driveWet)+ overd.Process(output)*driveWet;}
                break;
            case EffectType::Bitcrusher:
                if (isFirstEffect(effect)){output = input*(1-crushWet)+ bitcr.Process(input)*crushWet;}
                else {output = output*(1-crushWet)+ bitcr.Process(output)*crushWet;}
                break;
            case EffectType::Wavefolder:
                if (isFirstEffect(effect)){output = input*(1-foldWet) + fold.Process(input)*foldWet;}
                else {output = output*(1-foldWet) + fold.Process(output)*foldWet;}
                break;
            case EffectType::Reverb:
                if (isFirstEffect(effect)){rev.Process(input, input, &output, &output);}
                else {rev.Process(output,output,&output,&output);}
                break;
            case EffectType::Compressor:
                if (isFirstEffect(effect)){output = input*(1-compWet) + comp.Process(input)*compWet;}
                else {output = output*(1-compWet) + comp.Process(output)*compWet;}
                break;
            case EffectType::Resonator:
                if (isFirstEffect(effect)){output = input*(1-resWet) + resonator.Process(input)*resWet;}
                else {output = output*(1-resWet) + resonator.Process(output)*resWet;}
                break;
            case EffectType::Wah:
                if (isFirstEffect(effect)){output = wah.Process(input);}
                else {output = wah.Process(output);}
                break;
            case EffectType::Looper:
                break;
        }
        
     }
     
     if (signalChain.empty() && !stopLoopPlayback)
        {
            float inSig = input*loopInLevel;
            output = inSig + looper.Process(inSig)*loopLevel;
        }
        else if (!stopLoopPlayback)
        {
            float effectedSig = output*loopInLevel;
            output = effectedSig + looper.Process(effectedSig)*loopLevel;
        }
        else 
            {output = input;}
        
}
//=======================================================================================================//

size_t currentEffectIndex = 0;                                                                          //
                                                                                                       //
// Function to update the paramMode variable based on the current index
void updateParamMode()
{
    if (currentEffectIndex < signalChain.size())
    {
        paramMode = menuItemVector[currentEffectIndex];
    }
    else
    {
        // If the index is out of range, set paramMode to a default value
        paramMode = menuItems::SignalChain; // Or any other default value
    }
}

void menuLogic ()
{
    Encoder.Debounce();
    bool buttonPressed = Encoder.RisingEdge();
    int encValue = Encoder.Increment();
       // if (buttonPressed) {encValue = 0;};
    if (buttonPressed)
    {
        // Increment the currentEffectIndex and handle wrapping around if it goes beyond the vector size
        currentEffectIndex = (currentEffectIndex + 1) % signalChain.size();
        // Update the paramMode variable based on the new index
        updateParamMode();
    }                                                                                                         //
                                                                                                              //
}          
                                                                                                           //
//=============================================================================================================

void looperLogic()
{
    Switches[FS_1].Debounce();
    Switches[FS_2].Debounce();
    bool fs1_pressed = Switches[FS_1].RisingEdge();
    bool fs2_pressed = Switches[FS_2].RisingEdge();
    if (fs1_pressed)
        {looper.TrigRecord();}
    if (fs2_pressed)
        {stopLoopPlayback = !stopLoopPlayback;}
    if (Switches[FS_2].TimeHeldMs()>2000)
        {looper.Clear();}

}

//==========================================================================================
void initEffects()
{
    // Initialize effects and their varibles
	flanger.Init(hw.AudioSampleRate());
    chorus.Init(hw.AudioSampleRate());
    phaser.Init(hw.AudioSampleRate());
    trem.Init(hw.AudioSampleRate());
    overd.Init();
    fold.Init();
    del.Init();
    rev.Init(hw.AudioSampleRate());
    bitcr.Init(hw.AudioSampleRate());
    comp.Init(hw.AudioSampleRate());
    resonator.Init(.015, 24, hw.AudioSampleRate());
    wah.Init(hw.AudioSampleRate());
    looper.Init(loopBuffer, MAX_LOOP_SIZE);
    looper.SetMode(daisysp::Looper::Mode::NORMAL);
    setEffectValues();
}
//==========================================================================================
void removeEffectFromSignalChain(EffectType effect)
{
    auto removed = std::find(signalChain.begin(), signalChain.end(), effect);
    if (removed != signalChain.end())
    {
        signalChain.erase(removed);
    }
}
//==========================================================================================
void placeEffectInSignalChain(EffectType effect, int newPosition)
{
    auto it = std::find(signalChain.begin(), signalChain.end(), effect);
    if (it != signalChain.end())
    {
        signalChain.erase(it);
        signalChain.insert(signalChain.begin() + newPosition, effect);
    }
    else
    {signalChain.insert(signalChain.begin() + newPosition, effect);}
    if (signalChain.size() > 8)
        {removeEffectFromSignalChain(signalChain.back());}
}
//==========================================================================================
void AudioCallback(daisy::AudioHandle::InputBuffer  in,
                   daisy::AudioHandle::OutputBuffer out,
                   size_t                    size)
    {   
        processAnalogControls();
        processADC();
        menuLogic();
        looperLogic();
        for (size_t i=0; i < size; i++)
        {
            processEffects(in[0][i], out[0][i]);   
        }
    }
//==========================================================================================
int main ()
{
    hw.Init();
    hw.SetAudioBlockSize(48); // number of samples handled per callback
	hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
    initEffects();  
    InitOtherControls();
    InitAnalogControls();
    initOLED();
    hw.adc.Start();
    hw.SetLed(true);
    signalChain = {EffectType::Overdrive,EffectType::Phaser, EffectType::Chorus};
    
    hw.StartAudio(AudioCallback);
    
	while(1) 
    {
        paint();
    }
}
