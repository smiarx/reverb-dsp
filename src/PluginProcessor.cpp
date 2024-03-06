#include "PluginProcessor.h"
#include "BinaryData.h"
#include "GUI/SpreadSlider.h"
#include "GUI/SpringsGL.h"

//==============================================================================
PluginProcessor::PluginProcessor() :
    MagicProcessor(
        BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    m_parameters(*this, nullptr, juce::Identifier("Echoes"), createLayout())
{
    for (auto *param : getParameters()) addProcessorAsListener(param);
}

PluginProcessor::~PluginProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
PluginProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioProcessorParameterGroup>(
        "delay", "Delay", "|",
        std::make_unique<juce::AudioParameterFloat>("delay_drywet", "Dry/Wet",
                                                    0.f, 1.f, 0.2f),
        std::make_unique<juce::AudioParameterFloat>("delay_time", "Delay",
                                                    0.01f, 1.f, 0.12f),
        std::make_unique<juce::AudioParameterFloat>(
            "delay_feedback", "Feedback", 0.0f, 1.f, 0.8f),
        std::make_unique<juce::AudioParameterFloat>("delay_cutoff", "Cutoff",
                                                    20.f, 13000.f, 2000.f),
        std::make_unique<juce::AudioParameterFloat>("delay_drive", "Drive",
                                                    -40.f, 20.f, -40.f),
        std::make_unique<juce::AudioParameterFloat>("delay_drift", "Drift", 0.f,
                                                    1.f, 0.f),
        std::make_unique<juce::AudioParameterFloat>(
            "delay_driftfreq", "Drift Freq", 0.1f, 10.f, 0.6f),
        std::make_unique<juce::AudioParameterChoice>(
            "delay_mode", "Mode",
            juce::StringArray{"Normal", "Back&Forth", "Reverse"}, 0)));

    auto springs = std::make_unique<juce::AudioProcessorParameterGroup>(
        "springreverb", "Spring Reverb", "|");
    springs->addChild(std::make_unique<juce::AudioParameterFloat>(
        "springs_drywet", "Dry/Wet", 0.0f, 1.f, 0.f));

    const juce::NormalisableRange<float> rangeHiLo(0.f, 1.f);
    const juce::NormalisableRange<float> rangeLength(0.02f, 0.2f);
    const juce::NormalisableRange<float> rangeDecay(0.2f, 10.f);
    const juce::NormalisableRange<float> rangeDispersion(0.f, LOW_CASCADE_N,
                                                         1.f);
    const juce::NormalisableRange<float> rangeDamp(200.f, 12000.f, 0.f, 0.5f);
    const juce::NormalisableRange<float> rangeChaos(0.f, 0.01f);
    const juce::NormalisableRange<float> rangeSpringness(0.f, 1.f);
    constexpr auto defaultHiLo       = 0.f;
    constexpr auto defaultLength     = 0.045f;
    constexpr auto defaultDecay      = 2.5f;
    constexpr auto defaultDispersion = LOW_CASCADE_N;
    constexpr auto defaultDamp       = 4400.f;
    constexpr auto defaultChaos      = 0.f;
    constexpr auto defaultSpringness = 0.5f;

    for (int i = 0; i < 2; ++i) {
#define paramname(name) \
    (juce::String("springs_") + (i == 0 ? "" : "spread_") + name)
#define paramdefault(Param) (i == 0 ? default##Param : 0.f)
#define paramrange(Param)  \
    (i == 0 ? range##Param \
            : juce::NormalisableRange<float>{0.f, 1.f, 0.f, 0.7f})
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>(
            i == 0 ? "springs_macros" : "springs_spread",
            i == 0 ? "Macros" : "Spread", "|");
        group->addChild(std::make_unique<juce::AudioParameterFloat>(
            paramname("hilo"), "Direct/Rattle Mix", paramrange(HiLo),
            paramdefault(HiLo)));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(
            paramname("length"), "Length", paramrange(Length),
            paramdefault(Length)));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(
            paramname("decay"), "Decay", paramrange(Decay),
            paramdefault(Decay)));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(
            paramname("dispersion"), "Dispersion", paramrange(Dispersion),
            paramdefault(Dispersion)));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(
            paramname("damp"), "Damp", paramrange(Damp), paramdefault(Damp)));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(
            paramname("chaos"), "Chaos", paramrange(Chaos),
            paramdefault(Chaos)));
        group->addChild(std::make_unique<juce::AudioParameterFloat>(
            paramname("springness"), "Springness", paramrange(Springness),
            paramdefault(Springness)));
        springs->addChild(std::move(group));
#undef paramname
#undef paramdefault
#undef paramrange
    }

    for (int i = 0; i < MAXSPRINGS; ++i) {
#define paramname(name) \
    (juce::String("spring") + juce::String(i) + "_" + name)
        springs->addChild(std::make_unique<juce::AudioProcessorParameterGroup>(
            juce::String("spring") + juce::String(i),
            juce::String("Spring ") + juce::String(i), "|",
            std::make_unique<juce::AudioParameterFloat>(
                paramname("vol"), "Volume", -60.f, 0.f, 0.f),
            std::make_unique<juce::AudioParameterBool>(
                juce::String("spring") + juce::String(i) + "_solo", "Solo", 0),
            std::make_unique<juce::AudioParameterBool>(
                juce::String("spring") + juce::String(i) + "_mute", "mute", 0),
            std::make_unique<juce::AudioParameterChoice>(
                paramname("source"), "Source",
                juce::StringArray{"Left", "Right", "Mono"}, 2),
            std::make_unique<juce::AudioParameterFloat>(paramname("pan"), "Pan",
                                                        -1.f, 1.f, 0.f),
            std::make_unique<juce::AudioParameterFloat>(
                paramname("hilo"), "Direct/Rattle Mix", rangeHiLo, defaultHiLo),
            std::make_unique<juce::AudioParameterFloat>(
                paramname("length"), "Length", rangeLength, defaultLength),
            std::make_unique<juce::AudioParameterFloat>(
                paramname("decay"), "Decay", rangeDecay, defaultDecay),
            std::make_unique<juce::AudioParameterFloat>(
                paramname("dispersion"), "Dispersion", rangeDispersion,
                defaultDispersion),
            std::make_unique<juce::AudioParameterFloat>(
                paramname("damp"), "Damp", rangeDamp, defaultDamp),
            std::make_unique<juce::AudioParameterFloat>(
                paramname("chaos"), "Chaos", rangeChaos, defaultChaos),
            std::make_unique<juce::AudioParameterFloat>(
                paramname("springness"), "Springness", rangeSpringness,
                defaultSpringness)));
#undef paramnam
    }

    layout.add(std::move(springs));

    return layout;
}

//==============================================================================
juce::AudioProcessorEditor *PluginProcessor::createEditor()
{
    auto builder = std::make_unique<foleys::MagicGUIBuilder>(magicState);
    builder->registerJUCEFactories();
    builder->registerFactory("SpreadSlider", &SpreadSliderItem::factory);
    builder->registerFactory("SpringsGL", &SpringsGLItem::factory);

    magicState.setGuiValueTree(BinaryData::magic_xml,
                               BinaryData::magic_xmlSize);
    auto *editor =
        new foleys::MagicPluginEditor(magicState, std::move(builder));
    return editor;
}

//==============================================================================
const juce::String PluginProcessor::getName() const { return JucePlugin_Name; }

bool PluginProcessor::acceptsMidi() const { return false; }

bool PluginProcessor::producesMidi() const { return false; }

bool PluginProcessor::isMidiEffect() const { return false; }

double PluginProcessor::getTailLengthSeconds() const { return 0.0; }

int PluginProcessor::getNumPrograms() { return 1; }

int PluginProcessor::getCurrentProgram() { return 0; }

void PluginProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String PluginProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void PluginProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    tapedelay_desc_t desc = {
        /* delay */ 0.125f,
        /* feedback */ 0.8f,
        /* drywet */ 1.f,
        /* cutoff */ 2000.f,
        /* drive */ 0.f,
        /* drift */ 0.f,
        /* drift_freq */ 0.1f,
        /* pingpong */ 0,
        /* fmode */ 0.f,
        /* mode */ NORMAL,
    };
    tapedelay_init(&m_tapedelay, &desc, sampleRate);

    tapedelay_set_delay(&m_tapedelay, desc.delay);
    tapedelay_set_drive(&m_tapedelay, desc.drive);
    tapedelay_set_cutoff(&m_tapedelay, desc.cutoff);
    tapedelay_set_drift(&m_tapedelay, desc.drift);
    tapedelay_set_drift_freq(&m_tapedelay, desc.drift_freq);
    tapedelay_set_fmode(&m_tapedelay, desc.fmode);

    springs_desc_t sdesc = {
        /* ftr */ {4303.f, 4296.f, 4299.f, 4308.f, 4320.f, 4290.f, 4281.f,
                   4322.f},
        /* stages */
        {LOW_CASCADE_N, LOW_CASCADE_N, LOW_CASCADE_N, LOW_CASCADE_N,
         LOW_CASCADE_N, LOW_CASCADE_N, LOW_CASCADE_N, LOW_CASCADE_N},
        /* a1 */ {0.51f, 0.49f, 0.52f, 0.48f, 0.53f, 0.49f, 0.46f, 0.54f},
        /* ahigh */ {0.50f, 0.51f, 0.49f, 0.52f, 0.48f, 0.53f, 0.49f, 0.46f},
        /* Td */
        {0.045f, 0.046f, 0.043f, 0.051f, 0.041f, 0.051f, 0.041f, 0.050f},
        /* fcutoff */ {20.f, 20.f, 20.f, 20.f, 20.f, 20.f, 20.f, 20.f},
        /* gripple */ {0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f},
        /* gecho */ {0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f},
        /* t60 */ {2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f},
        /* chaos */
        {0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f, 0.001f},
        /* vol */ {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
        /* solo */ {0, 0, 0, 0, 0, 0, 0, 0},
        /* mute */ {0, 0, 0, 0, 0, 0, 0, 0},
        /* hilomix */ {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
        /* source */ {2, 2, 2, 2, 2, 2, 2, 2},
        /* pan */ {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
        /* drywet */ 0.2f,
    };

    springs_init(&m_springreverb, &sdesc, sampleRate, samplesPerBlock);

    /* RMS buffer */
    SpringsGL::setRMS(m_springreverb.rms.rms, &m_springreverb.rms.rms_id);

    /* populate spreads */
    auto *rands = &m_paramRands[0][0];
    for (int i = 0; i < sizeof(m_paramRands) / sizeof(m_paramRands[0][0]);
         ++i) {
        rands[i] = (float)std::rand() / RAND_MAX * 2.f - 1.f;
    }
}

void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                   juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);

    int count = buffer.getNumSamples();

    while (!m_paramEvents.empty()) {
        auto event = m_paramEvents.front();
        m_paramEvents.pop_front();
        int spring = 0;
        if (event.id >= springParamBegin) {
            int id              = static_cast<int>(event.id);
            constexpr int begin = static_cast<int>(springParamBegin);
            constexpr int end   = static_cast<int>(springParamEnd);
            spring              = (id - begin) / (end - begin);
            id                  = (id - begin) % (end - begin) + begin;
            event.id            = static_cast<ParamId>(id);
        }
        switch (event.id) {
        case ParamId::DelayDrywet:
            m_tapedelay.desc.drywet = event.value;
            break;
        case ParamId::DelayTime:
            tapedelay_set_delay(&m_tapedelay, event.value);
            break;
        case ParamId::DelayFeedback:
            m_tapedelay.desc.feedback = event.value;
            break;
        case ParamId::DelayCutoff:
            tapedelay_set_cutoff(&m_tapedelay, event.value);
            break;
        case ParamId::DelayDrive:
            tapedelay_set_drive(&m_tapedelay, event.value);
            break;
        case ParamId::DelayDrift:
            tapedelay_set_drift(&m_tapedelay, event.value);
            break;
        case ParamId::DelayDriftFreq:
            tapedelay_set_drift_freq(&m_tapedelay, event.value);
            break;
        case ParamId::DelayMode:
            tapedelay_set_fmode(&m_tapedelay, event.value);
            break;
        case ParamId::SpringsDryWet:
            springs_set_drywet(&m_springreverb, event.value, count);
            break;
        case ParamId::SpringVolume:
            m_springreverb.desc.vol[spring] = event.value;
            springs_set_vol(&m_springreverb, m_springreverb.desc.vol, count);
            break;
        case ParamId::SpringSolo:
            m_springreverb.desc.solo[spring] = event.value != 0.f;
            springs_set_solo(&m_springreverb, m_springreverb.desc.solo, count);
            break;
        case ParamId::SpringMute:
            m_springreverb.desc.mute[spring] = event.value != 0.f;
            springs_set_mute(&m_springreverb, m_springreverb.desc.mute, count);
            break;
        case ParamId::SpringSource:
            m_springreverb.desc.source[spring] = static_cast<int>(event.value);
            break;
        case ParamId::SpringPan:
            m_springreverb.desc.pan[spring] = event.value;
            springs_set_pan(&m_springreverb, m_springreverb.desc.pan, count);
            break;
        case ParamId::SpringHiLo:
            m_springreverb.desc.hilomix[spring] = event.value;
            springs_set_hilomix(&m_springreverb, m_springreverb.desc.hilomix,
                                count);
            break;
        case ParamId::SpringLength:
            m_springreverb.desc.length[spring] = event.value;
            springs_set_length(&m_springreverb, m_springreverb.desc.length,
                               count);
            break;
        case ParamId::SpringDecay:
            m_springreverb.desc.t60[spring] = event.value;
            springs_set_t60(&m_springreverb, m_springreverb.desc.t60, count);
            break;
        case ParamId::SpringDispersion:
            m_springreverb.desc.stages[spring] =
                static_cast<unsigned int>(event.value);
            springs_set_stages(&m_springreverb, m_springreverb.desc.stages);
            break;
        case ParamId::SpringDamp:
            m_springreverb.desc.ftr[spring] = event.value;
            springs_set_ftr(&m_springreverb, m_springreverb.desc.ftr, count);
            break;
        case ParamId::SpringChaos:
            m_springreverb.desc.chaos[spring] = event.value;
            springs_set_chaos(&m_springreverb, m_springreverb.desc.chaos,
                              count);
            break;
        case ParamId::SpringSpringness:
            m_springreverb.desc.a1[spring]    = event.value;
            m_springreverb.desc.ahigh[spring] = event.value;
            springs_set_a1(&m_springreverb, m_springreverb.desc.a1, count);
            springs_set_ahigh(&m_springreverb, m_springreverb.desc.ahigh,
                              count);
        case ParamId::_SpringParamEnd:
        default:
            break;
        }
    }

    const float *const *ins = buffer.getArrayOfReadPointers();
    float *const *outs      = buffer.getArrayOfWritePointers();

    tapedelay_process(&m_tapedelay, ins, outs, count);
    springs_process(&m_springreverb, ins, outs, count);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

//==============================================================================
void PluginProcessor::parameterValueChanged(int id, float newValue)
{
    if (id >= static_cast<int>(macroBegin) &&
        id < static_cast<int>(springParamBegin)) {
        int macro = (id - static_cast<int>(macroBegin)) % numMacros;

        // if macro
        if (id < static_cast<int>(ParamId::_SpringsMacroEnd))
            m_paramMacros[macro] = newValue;
        // if spread
        else
            m_paramSpreads[macro] = newValue;

        int paramid = macro + static_cast<int>(macroSpringBegin);
        for (int i = 0; i < MAXSPRINGS; ++i) {
            constexpr auto begin = static_cast<int>(springParamBegin);
            constexpr auto end   = static_cast<int>(springParamEnd);
            id                   = paramid + (end - begin) * i;
            float value          = m_paramMacros[macro] +
                          m_paramSpreads[macro] * m_paramRands[macro][i];
            getParameters()[id]->setValueNotifyingHost(value);
        }
        return;
    }
    auto *ptr = static_cast<juce::RangedAudioParameter *>(getParameters()[id]);
    float value = ptr->convertFrom0to1(newValue);

    /* remove previous event with same id */
    for (auto it = m_paramEvents.begin(); it != m_paramEvents.end();) {
        if (it->id == static_cast<ParamId>(id)) m_paramEvents.erase(it);
        else
            ++it;
    }
    m_paramEvents.emplace_back(id, value);
}

void PluginProcessor::addProcessorAsListener(
    juce::AudioProcessorParameter *param)
{
    jassert(param != nullptr);
    param->addListener(this);
    parameterValueChanged(param->getParameterIndex(), param->getValue());
}
