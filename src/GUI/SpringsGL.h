#pragma once

#include "BinaryData.h"
#include <foleys_gui_magic/foleys_gui_magic.h>
#include <juce_opengl/juce_opengl.h>

extern "C" {
#include "springreverb.h"
}

class SpringsGL : public juce::Component, public juce::OpenGLRenderer
{

  public:
    SpringsGL();
    ~SpringsGL();

    static springsfloat *rms;
    static const int *rmspos;
    static void setRMS(springsfloat t_rms[RMS_BUFFER_SIZE], const int *t_rmspos)
    {
        rms    = t_rms;
        rmspos = t_rmspos;
    }

    //==========================================================================
    // OpenGL Callbacks

    /** Called before rendering OpenGL, after an OpenGLContext has been
       associated with this OpenGLRenderer (this component is a OpenGLRenderer).
        Sets up GL objects that are needed for rendering.
     */
    void newOpenGLContextCreated() override;

    /** Called when done rendering OpenGL, as an OpenGLContext object is
       closing. Frees any GL objects created during rendering.
     */
    void openGLContextClosing() override;

    /** The OpenGL rendering callback.
     */
    void renderOpenGL() override;

    //==========================================================================
    // JUCE Callbacks

    void paint(juce::Graphics &g) override { (void)g; }

    void resized() override {}

  private:
    //==========================================================================
    // OpenGL Functions

    /** Loads the OpenGL Shaders and sets up the whole ShaderProgram
     */
    void createShaders();

    //==============================================================================
    // This class just manages the uniform values that the fragment shader uses.
    struct Uniforms {
        Uniforms(juce::OpenGLContext &openGLContext,
                 juce::OpenGLShaderProgram &shaderProgram)
        {
            resolution.reset(
                createUniform(openGLContext, shaderProgram, "u_resolution"));
            time.reset(createUniform(openGLContext, shaderProgram, "u_time"));
            rms.reset(createUniform(openGLContext, shaderProgram, "u_rms"));
            rmspos.reset(
                createUniform(openGLContext, shaderProgram, "u_rmspos"));
        }

        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> resolution, time,
            rms, rmspos;

      private:
        static juce::OpenGLShaderProgram::Uniform *
        createUniform(juce::OpenGLContext &openGLContext,
                      juce::OpenGLShaderProgram &shaderProgram,
                      const char *uniformName)
        {
            if (openGLContext.extensions.glGetUniformLocation(
                    shaderProgram.getProgramID(), uniformName) < 0)
                return nullptr;

            return new juce::OpenGLShaderProgram::Uniform(shaderProgram,
                                                          uniformName);
        }
    };

    // OpenGL Variables
    juce::OpenGLContext openGLContext;
    GLuint VBO, EBO;

    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<Uniforms> uniforms;

    int itime;

    /** DEV NOTE
        If I wanted to optionally have an interchangeable shader system,
        this would be fairly easy to add. Chack JUCE Demo -> OpenGLDemo.cpp for
        an implementation example of this. For now, we'll just allow these
        shader files to be static instead of interchangeable and dynamic.
        String newVertexShader, newFragmentShader;
     */

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpringsGL)
};

class SpringsGLItem : public foleys::GuiItem
{
  public:
    FOLEYS_DECLARE_GUI_FACTORY(SpringsGLItem)

    SpringsGLItem(foleys::MagicGUIBuilder &builder,
                  const juce::ValueTree &node) :
        GuiItem(builder, node)
    {
        addAndMakeVisible(springGL);
    }

    void update() override{};
    juce::Component *getWrappedComponent() override { return &springGL; }

    [[nodiscard]] std::vector<foleys::SettableProperty>
    getSettableProperties() const override
    {
        std::vector<foleys::SettableProperty> props;
        return props;
    };

  private:
    SpringsGL springGL;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpringsGLItem)
};
