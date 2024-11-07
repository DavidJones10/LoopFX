#include "LoopFX.h"

#define MAX_DELAY static_cast<size_t>(48000)
#define MAX_LOOP_SIZE (48000 * 60 * 3)


constexpr daisy::Pin PIN_FS1 =        daisy::seed::D25;
constexpr daisy::Pin PIN_FS2 =        daisy::seed::D26;
constexpr daisy::Pin PIN_KNOB1 =      daisy::seed::D21;
constexpr daisy::Pin PIN_KNOB2 =      daisy::seed::D20;
constexpr daisy::Pin PIN_KNOB3 =      daisy::seed::D19;
constexpr daisy::Pin PIN_KNOB4 =      daisy::seed::D18;
constexpr daisy::Pin PIN_KNOB5 =      daisy::seed::D17;
constexpr daisy::Pin PIN_KNOB6 =      daisy::seed::D16;
constexpr daisy::Pin PIN_ENC_DEC =    daisy::seed::D23;
constexpr daisy::Pin PIN_ENC_INC =    daisy::seed::D22;
constexpr daisy::Pin PIN_ENC_BUTTON = daisy::seed::D14;

//==========================================================================================
daisysp::DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS del; //1
daisysp::Looper looper;
float DSY_SDRAM_BSS loopBuffer[MAX_LOOP_SIZE];
//==========================================================================================
static daisy::DaisySeed hw;
static daisy::AdcHandle adc;
//==========================================================================================
// Checks if effect is the first in the SignalChain vector
bool isFirstEffect(EffectType effect)
{
    return !signalChain.empty() && signalChain.front() == effect;
}
//==========================================================================================
// Scales knob's 0 to 1 value from min to max
float scaleKnob (float knobValue, float min, float max)
{
    return knobValue * (max-min) + min;
}
//==========================================================================================
// Scales the logarithmic pots to have a more linear response
float LinearizeLogarithmicValue(float logValue)
{
    return std::sqrt(logValue);
}
//==========================================================================================
// Changes an effect parameter based on a changing knob value and scales and or type 
// casts if necessary
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
// Initializes encoder and footswitches
void InitOtherControls()
{
    Encoder.Init(PIN_ENC_INC, PIN_ENC_DEC, PIN_ENC_BUTTON, hw.AudioCallbackRate());

    constexpr daisy::Pin pin_numbers[SWITCH_LAST] = {PIN_FS1, PIN_FS2};
    for(size_t i = 0; i < SWITCH_LAST; i++)
    {
        Switches[i].Init(pin_numbers[i]);
    }
}
//==========================================================================================
// Initializes potentiometers
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
    hw.adc.Start();
}
//==========================================================================================
// Removes given effect from the signal chain
void removeEffectFromSignalChain(EffectType effect)
{
    auto removed = std::find(signalChain.begin(), signalChain.end(), effect);
    if (removed != signalChain.end())
    {
        signalChain.erase(removed);
    }
}
//==========================================================================================
// Places effect in signal chain at specified location from 0 to 7 and -1 if not present
void placeEffectInSignalChain(EffectType effect, int newPosition)
{
    if (newPosition == -1)
        {removeEffectFromSignalChain(effect);}
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
// Gets index of an effect in the signal chain
int getChainIndex(EffectType effect)
{
    auto it = std::find(signalChain.begin(), signalChain.end(), effect);
    if (it != signalChain.end())
    {
        return std::distance(signalChain.begin(), it);
    }
    return -1; // Effect not found in signal chain
}
//==========================================================================================
// Gets effect type from menu item
void effectFromMenu()
{
    switch (paramMode)
        {   
            case menuItems::Delay:
                menuToEffect = EffectType::Delay;
                break;
            case menuItems::Phaser:  
                menuToEffect = EffectType::Phaser;
                break;
            case menuItems::Chorus:
                menuToEffect = EffectType::Chorus;
                break;
            case menuItems::Flanger:
                menuToEffect = EffectType::Flanger;
                break;
            case menuItems::Tremolo:
                menuToEffect = EffectType::Tremolo;
                break;
            case menuItems::Overdrive:
                menuToEffect = EffectType::Overdrive;
                break;
            case menuItems::Bitcrusher:
                menuToEffect = EffectType::Bitcrusher;
                break;
            case menuItems::Wavefolder:
                menuToEffect = EffectType::Wavefolder;
                break;
            case menuItems::Reverb:
                menuToEffect = EffectType::Reverb;
                break;
            case menuItems::Compressor:
                menuToEffect = EffectType::Compressor;
                break;
            case menuItems::Resonator:
                menuToEffect = EffectType::Resonator;
                break;
            case menuItems::Wah:
                menuToEffect = EffectType::Wah;
                break;
            case menuItems::Looper:
                break;
            case menuItems::SignalChain:
                break;
        }

}
//==========================================================================================
// Uses encoder to control main menu
void menuEncoderLogic()
{
    Encoder.Debounce();
    if (!inSubmenu)
    {   
        if (Encoder.RisingEdge())
            {inSubmenu = true;}
        currentEffectIndex += Encoder.Increment();
        currentEffectIndex = (currentEffectIndex + 13 % 13) % 13;
        if (currentEffectIndex < 0) 
            {currentEffectIndex = 13;} 
        else if (currentEffectIndex > 13) 
            {currentEffectIndex = 0;}
        if ((unsigned int)currentEffectIndex < menuItemVector.size())
            {paramMode = menuItemVector[currentEffectIndex];}
        else
            {paramMode = menuItems::SignalChain;}
        effectFromMenu();
    }  
    else 
    {
        if (Encoder.TimeHeldMs() > 2000)
            {
                inSubmenu = false;
                currentEffectIndex = 0;
            }
        if (paramMode != menuItems::SignalChain)
        {
            if (Encoder.RisingEdge())
            {editLocationAvailable = !editLocationAvailable;}
            int currentChainLocation = getChainIndex(menuToEffect);
            if (editLocationAvailable)
                {
                    currentChainLocation += Encoder.Increment();
                    currentChainLocation = currentChainLocation > 7 ? -1 : currentChainLocation;
                    currentChainLocation = currentChainLocation < -1 ? 7 : currentChainLocation;
                    visibleChainIndex = currentChainLocation;
                }
            else
                {
                    placeEffectInSignalChain(menuToEffect,currentChainLocation);
                    visibleChainIndex = currentChainLocation;
                }
        }
        else 
        {
            int chainLocation = 0;
            chainLocation += Encoder.Increment();
            if (signalChain.size() > 4)
            {
                chainEncValue = chainLocation;
            }
        }

    }
}
//==========================================================================================
// Makes strings and writes to OLED for main menu
void mainMenu(char* charPointer)
{   
    int_fast8_t x = 100;
    int_fast8_t y = pow(sqrt((currentEffectIndex * 14) % 70), 2);
    y = y < 0 ? 60 : y;
    if (y == 0) {y=4;}
    if (!inSubmenu)
    {   
        // Page 1
        if (currentEffectIndex < 5)
        {
            display.Fill(false);
            display.SetCursor(0, 0);
            displayString = "Signal Chain   ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0,12);
            displayString = "Looper         ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0, 24);
            displayString = "Delay          ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0, 36);
            displayString = "Phaser         ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0, 48);
            displayString = "Chorus         ";
            display.WriteString(charPointer,Font_7x10, true);
            display.DrawCircle(x,y,3,true);
            
        }
        // Page 2
        else if (currentEffectIndex < 9 && currentEffectIndex >= 5)
        {
            display.Fill(false);
            display.SetCursor(0, 0);
            displayString = "Flanger        ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0,12);
            displayString = "Tremolo        ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0, 24);
            displayString = "Overdrive      ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0, 36);
            displayString = "Compressor     ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0, 48);
            displayString = "Bitcrusher     ";
            display.WriteString(charPointer,Font_7x10, true);
            display.DrawCircle(x,y,3,true);
        }
        // Page 3
        else if (currentEffectIndex >= 10)
        {
            display.Fill(false);
            display.SetCursor(0, 0);
            displayString = "Wavefolder     ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0,12);
            displayString = "Reverb         ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0, 24);
            displayString = "Resonator      ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0, 36);
            displayString = "Wah            ";
            display.WriteString(charPointer,Font_7x10, true);
            display.SetCursor(0, 48);
            displayString = "               ";
            display.WriteString(charPointer,Font_7x10, true);
            display.DrawCircle(x,y,3,true);
        }
    }
}
//==========================================================================================
// Display for the signal chain menu item
void signalChainDisplay(char* charPointer)
{
    int arrSize = signalChain.size();
    int startIdx;
    int endIdx;
    if (arrSize < 4)
    {
        startIdx = 0;
        endIdx = arrSize-1;
    }
    else 
    {
        startIdx = chainEncValue;
        endIdx = startIdx + 4;
    }
    display.Fill(false);
    for (int i=startIdx; i < endIdx && i < arrSize; i++)
    {
        EffectType effect = signalChain[i];
        switch (effect)
        {
            case (EffectType::Delay):
                displayString = "Delay";
                break;
            case EffectType::Phaser:
                displayString = "Phaser";
                break;
            case EffectType::Chorus:
                displayString = "Chorus";
                break;
            case EffectType::Flanger:
                displayString = "Flanger";
                break;
            case EffectType::Tremolo:
                displayString = "Tremolo";
                break;
            case EffectType::Overdrive:
                displayString = "Overdrive";
                break;
            case EffectType::Bitcrusher:
                displayString = "Bitcrusher";
                break;
            case EffectType::Wavefolder:
                displayString = "Wavefolder";
                break;
            case EffectType::Reverb:
                displayString = "Reverb";
                break;
            case EffectType::Compressor:
                displayString = "Compressor";
                break;
            case EffectType::Resonator:
                displayString = "Resonator";
                break;
            case EffectType::Wah:
                displayString = "Wah";
                break;
        }
        display.SetCursor(0, (i-startIdx)*10);
        display.WriteString(charPointer, Font_6x8, true);
    }
}
//==========================================================================================
// Takes vector of names and values and prints them to screen for a submenu
void submenuDisplay(std::vector<std::string> strArr, std::vector<float> paramValues,char* strPointer, bool isLooper = false)
{
    int arrSize = strArr.size();
    int lineDistance = 10;
    display.Fill(false);
    if (!isLooper)
    {
        for (int i = 0; i < arrSize; i++)
        {
            displayString = strArr[i] + std::to_string(static_cast<int32_t>(paramValues[i]));
            display.SetCursor(0,lineDistance * i);
            display.WriteString(strPointer, Font_6x8, true);
        }
        display.SetCursor(100,0);
        displayString = std::to_string(static_cast<int32_t>(visibleChainIndex));
        display.WriteString(strPointer,Font_7x10, true);
    }
    else 
    {
        for (int i = 0; i < arrSize-1; i++)
        {
            displayString = strArr[i] + std::to_string(static_cast<int32_t>(paramValues[i]));
            display.SetCursor(0,lineDistance * i);
            display.WriteString(strPointer, Font_6x8, true);
        }
        displayString = strArr[2];
        display.SetCursor(0, lineDistance * 2);
        display.WriteString(strPointer, Font_6x8, true);
    }
}
//==========================================================================================
// Runs all display function and is called in Main
// TODO: Find out why last two strings in paramaterNames arent being displayed
void displayMenu()
{   
    daisy::System::Delay(200);
    
    char* strptr = &displayString[0];
    mainMenu(strptr);
    if (inSubmenu)
    {
        switch (paramMode)
            {   
                case menuItems::Delay:
                    parameterNames = {std::string("Time(ms):"), std::string("Feedback(0-100):"), std::string("Wet(0-100):")};
                    parameterValues = {delTime, delFeedback*100, delWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Phaser:  
                    parameterNames = {std::string("Rate(Hz):"), std::string("Depth(0-100):"), std::string("LFO Freq(Hz):"),std::string("Poles(1-8):"), 
                                        std::string("Feedback(0-100):"), std::string("Wet(0-100):")};
                    parameterValues = {phaserRate, phaserDepth*100, phaserLFOFreq, phaserNumPoles, phaserFeedback*100, phaserWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Chorus:
                    parameterNames = {std::string("Rate(Hz):"),std::string("Depth(0-100):"),std::string("Feedback(0-100):"),std::string("Delay(ms):"),
                                        std::string("Wet(0-100):")};
                    parameterValues = {chorusFreq, chorusDepth*100, chorusFeedback*100, chorusDelay, chorusWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Flanger:
                    parameterNames = {std::string("Rate(Hz):"),std::string("Depth(0-100):"),std::string("Feedback(0-100):"),std::string("Delay(ms):"),
                                        std::string("Wet(0-100):")};
                    parameterValues = {flangerFreq, flangerDepth*100, flangerFeedback*100, flangerDelay, flangerWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Tremolo:
                    parameterNames = {std::string("Rate(Hz):"),std::string("Depth(0-100):"),std::string("Wet(0-100):")};
                    parameterValues = {tremRate, tremDepth*100, tremWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Overdrive:
                    parameterNames = {std::string("Drive(0-100)"), std::string("Wet(0-100):")};
                    parameterValues = {drive*100, driveWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Bitcrusher:
                    parameterNames = {std::string("BitDepth(0-16):"),std::string("Rate(0-48k):"),std::string("Wet(0-100)")};
                    parameterValues = {bitDepth, crushRate, crushWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Wavefolder:
                    parameterNames = {std::string("Gain(-40-0):"),std::string("Offset(0-15):"),std::string("Wet(0-100):")};
                    parameterValues = {foldGain, foldOffset, foldWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Reverb:
                    parameterNames = {std::string("Feedback(0-100):"),std::string("Cutoff(0-24k):")};
                    parameterValues = {revFeedback*100,revCutoff};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Compressor:
                    parameterNames = {std::string("Ratio(1-40):"),std::string("Attack(0-10):"),std::string("Release(0-10):"),std::string("Makeup(0-80):"),
                                        std::string("Threshold(0):"),std::string("Wet(0-100):")};
                    parameterValues = {compRatio, compAttack, compRelease, compMakeup, compThresh, compWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Resonator:
                    parameterNames = {std::string("Damping(0-100):"),std::string("Bright(0-100):"),std::string("Structure(0-100):"),std::string("Frequency(20-10k):"),
                                        std::string("Wet(0-100):")};
                    parameterValues = {resDamping*100, resBright*100, resFreq, resStructure*100, resWet*100};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Wah:
                    parameterNames = {std::string("Amount(0-100):"),std::string("Level(0-100):"),std::string("Wet(0-100):")};
                    parameterValues = {wahAmount*100, wahLevel*100, wahWet};
                    submenuDisplay(parameterNames, parameterValues, strptr);
                    break;
                case menuItems::Looper:
                    parameterNames = {std::string("In Level(0-100):"),std::string("Loop Level(0-200):"), std::string("Mode: ") + looperString};
                    parameterValues = {loopInLevel*100, loopLevel*100};
                    submenuDisplay(parameterNames, parameterValues, strptr, true);
                    break;
                case menuItems::SignalChain:
                    signalChainDisplay(strptr);
                    break;
            }
    }
    display.Update();
}
//==========================================================================================
// Initializes OLED screen
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
// Preliminary processing of all of the knobs
void processAnalogControls()
{
    
    for(size_t i = 0; i < KNOB_LAST; i++)
    {
        Knobs[i].Process();  
    }
}
//==========================================================================================
// Sets all effect values using all corresponding variables
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
// Processes knobs and adaptively uses their values to change effect parameters
void knobsToValues()
{   
    switch (paramMode)
        {
            case menuItems::Delay:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), delTime, 1.f, 500.f);
                conditionalParameter(k2, Knobs[knob_2].Process(), delFeedback);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), delWet);
                break;
            case menuItems::Phaser:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), phaserRate, .01f, 15.f);
                conditionalParameter(k2, Knobs[knob_2].Process(), phaserDepth);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), phaserLFOFreq, .01f, 15.f);
                conditionalParameter(k4, Knobs[knob_4].Process(), phaserNumPoles, 1, 8, true);
                conditionalParameter(k5, LinearizeLogarithmicValue(Knobs[knob_5].Process()), phaserFeedback);
                conditionalParameter(k6, LinearizeLogarithmicValue(Knobs[knob_6].Process()), phaserWet);             
                break;
            case menuItems::Chorus:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), chorusFreq, .01f, 15.f);
                conditionalParameter(k2, Knobs[knob_2].Process(), chorusDepth);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), chorusFeedback);
                conditionalParameter(k4, Knobs[knob_4].Process(), chorusDelay, 20.f, 50.f);
                conditionalParameter(k5, LinearizeLogarithmicValue(Knobs[knob_5].Process()), chorusWet);
                break;
            case menuItems::Flanger:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), flangerFreq, .01f, 20.f);
                conditionalParameter(k2, Knobs[knob_2].Process(), flangerDepth);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), flangerFeedback);
                conditionalParameter(k4, Knobs[knob_4].Process(), flangerDelay, .01f, 15.f);
                conditionalParameter(k5, LinearizeLogarithmicValue(Knobs[knob_5].Process()), flangerWet);
                break;
            case menuItems::Tremolo:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), tremRate, .01f, 15.f);
                conditionalParameter(k2, Knobs[knob_2].Process(), tremDepth);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), tremWet);
                break;
            case menuItems::Overdrive:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), drive);
                conditionalParameter(k2, Knobs[knob_2].Process(), driveWet);
                break;
            case menuItems::Bitcrusher:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), bitDepth,0,16,true);
                conditionalParameter(k2, Knobs[knob_2].Process(), crushRate, 0,48000, true);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), crushWet);
                break;
            case menuItems::Wavefolder:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), foldGain, -40.f, 0.f);
                conditionalParameter(k2, Knobs[knob_2].Process(), foldOffset, 0.f, 10.f);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), foldWet);
                break;
            case menuItems::Reverb:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), revFeedback);
                conditionalParameter(k2, Knobs[knob_2].Process(), revCutoff, 0, 24000);
                break;
            case menuItems::Compressor:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), compRatio,1.f, 40.f);
                conditionalParameter(k2, Knobs[knob_2].Process(), compAttack, .001f, 10.f);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), compRelease, .001f, 10.f);
                conditionalParameter(k4, Knobs[knob_4].Process(), compMakeup, 0.f, 80.f);
                conditionalParameter(k5, LinearizeLogarithmicValue(Knobs[knob_5].Process()), compThresh, 0.f, -80.f);
                conditionalParameter(k6, LinearizeLogarithmicValue(Knobs[knob_6].Process()), compWet);  
                break;
            case menuItems::Resonator:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), resDamping);
                conditionalParameter(k2, Knobs[knob_2].Process(), resBright);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), resFreq, 20, 10000);
                conditionalParameter(k4, Knobs[knob_4].Process(), resStructure);
                conditionalParameter(k5, LinearizeLogarithmicValue(Knobs[knob_5].Process()), resWet);
                break;
            case menuItems::Wah:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), wahAmount);
                conditionalParameter(k2, Knobs[knob_2].Process(), wahLevel);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), wahWet, 0, 100);
                break;
            case menuItems::Looper:
                conditionalParameter(k1, LinearizeLogarithmicValue(Knobs[knob_1].Process()), loopInLevel, 0.f, 2.f);
                conditionalParameter(k2, Knobs[knob_2].Process(), loopLevel, 0.f, 2.f);
                conditionalParameter(k3, LinearizeLogarithmicValue(Knobs[knob_3].Process()), looperMode, 0, 3, true);
                break;
            case menuItems::SignalChain:
                
                break;
        }
    setEffectValues(); 
}
//==========================================================================================
// Main audio function: run audio through all active effects and processes looper
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
        else if (stopLoopPlayback && !signalChain.empty())
        {
            output = output;
        }
        else 
            {output = input;}
        
        
}        
//==========================================================================================
// Uses footswitches to control the looper
void looperLogic()
{
    switch ((int)looperMode)
    {
        case (0):
            looper.SetMode(daisysp::Looper::Mode::NORMAL);
            looperString = "Normal";
            break;
        case (1):
            looper.SetMode(daisysp::Looper::Mode::ONETIME_DUB);
            looperString = "OneTime Dub";
            break;
        case (2):
            looper.SetMode(daisysp::Looper::Mode::REPLACE);
            looperString = "Replace";
            break;
        case (4):
            looper.SetMode(daisysp::Looper::Mode::FRIPPERTRONICS);
            looperString = "Frippertronics";
            break;
    }
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
// Initializes all effects
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
// Main audio fuction
void AudioCallback(daisy::AudioHandle::InputBuffer  in,
                   daisy::AudioHandle::OutputBuffer out,
                   size_t                    size)
    {   
        processAnalogControls();
        menuEncoderLogic();
        knobsToValues();
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
	hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);
    initEffects();  
    initOLED();
    signalChain = {EffectType::Overdrive,EffectType::Phaser, EffectType::Chorus, EffectType::Delay, EffectType::Resonator};
    paramMode = menuItems::Delay;
    hw.SetLed(true);
    InitOtherControls();
    InitAnalogControls();

    hw.SetAudioBlockSize(2);
    hw.StartAudio(AudioCallback);
    
	while(1) 
    {
        displayMenu();
    }
}
