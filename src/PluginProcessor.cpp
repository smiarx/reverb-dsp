#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor() :
    AudioProcessor(
        BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    m_parameters(*this, nullptr, juce::Identifier("Echoes"), createLayout())
{
    addListener("delay_drywet");
    addListener("delay_time");
    addListener("delay_feedback");
    addListener("delay_cutoff");
    addListener("delay_drive");
    addListener("delay_drift");
    addListener("delay_driftfreq");
    addListener("delay_mode");
    addListener("springs_drywet");
    for (int i = 0; i < MAXSPRINGS; ++i) {
        addListener(juce::String("spring") + juce::String(i) +
                    juce::String("_vol"));
    }
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

    for (int i = 0; i < MAXSPRINGS; ++i) {
        springs->addChild(std::make_unique<juce::AudioProcessorParameterGroup>(
            juce::String("spring") + juce::String(i),
            juce::String("Spring ") + juce::String(i), "|",
            std::make_unique<juce::AudioParameterFloat>(
                juce::String("spring") + juce::String(i) + "_vol", "Volume",
                -60.f, 0.f, 0.f)));
    }

    layout.add(std::move(springs));

    return layout;
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
        /* a1 */ {0.51f, 0.49f, 0.52f, 0.48f, 0.53f, 0.49f, 0.46f, 0.54f},
        /* ahigh */ {0.50f, 0.51f, 0.49f, 0.52f, 0.48f, 0.53f, 0.49f, 0.46f},
        /* Td */
        {0.045f, 0.046f, 0.043f, 0.051f, 0.041f, 0.051f, 0.041f, 0.050f},
        /* fcutoff */ {20.f, 20.f, 20.f, 20.f, 20.f, 20.f, 20.f, 20.f},
        /* gripple */ {0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f},
        /* gecho */ {0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f},
        /* glf */ {0.95f, 0.95f, 0.95f, 0.95f, 0.95f, 0.95f, 0.95f, 0.95f},
        /* ghf */ {0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f, 0.85f},
        /* vol */ {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
        /* hilomix */ {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
    };

    springs_init(&m_springreverb, &sdesc, sampleRate);
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

    while (!m_paramEvents.empty()) {
        auto event = m_paramEvents.front();
        m_paramEvents.pop_front();
        int spring = 0;
        if (event.id >= ParamId::SpringParamBegin) {
            int id              = static_cast<int>(event.id);
            constexpr int begin = static_cast<int>(ParamId::SpringParamBegin);
            constexpr int end   = static_cast<int>(ParamId::SpringParamEnd);
            spring              = (id - begin) / (end - begin - 1);
            id                  = (id - begin) % (end - begin - 1) + begin + 1;
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
            m_springreverb.desc.drywet = event.value;
            break;
        case ParamId::SpringVolume:
            m_springreverb.desc.vol[spring] = event.value;
            springs_set_vol(&m_springreverb, m_springreverb.desc.vol);
            break;
        }
    }

    const float *const *ins = buffer.getArrayOfReadPointers();
    float *const *outs      = buffer.getArrayOfWritePointers();

    tapedelay_process(&m_tapedelay, ins, outs, buffer.getNumSamples());
    springs_process(&m_springreverb, ins, outs, buffer.getNumSamples());
}

//==============================================================================
bool PluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

//==============================================================================
void PluginProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = m_parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(
        getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(m_parameters.state.getType()))
            m_parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
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

void PluginProcessor::addListener(const juce::String &stringId)
{
    auto *param = m_parameters.getParameter(stringId);
    jassert(param != nullptr);
    param->addListener(this);
    parameterValueChanged(param->getParameterIndex(), param->getValue());
}
