#include "SpringsGL.h"

springsfloat *SpringsGL::rms;
const int *SpringsGL::rmspos;

SpringsGL::SpringsGL()
{
    //// Sets the OpenGL version to 3.2
    // openGLContext.setOpenGLVersionRequired(
    //     juce::OpenGLContext::OpenGLVersion::openGL3_2);

    // Attach the OpenGL context but do not start [ see start() ]
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    openGLContext.setContinuousRepainting(true);
}

SpringsGL::~SpringsGL()
{
    // Turn off OpenGL
    openGLContext.setContinuousRepainting(false);
    openGLContext.detach();
}

void SpringsGL::newOpenGLContextCreated()
{
    // Setup Shaders
    createShaders();

    // Setup Buffer Objects
    openGLContext.extensions.glGenBuffers(1, &VBO); // Vertex Buffer Object
    openGLContext.extensions.glGenBuffers(1, &EBO); // Element Buffer Object
}

void SpringsGL::openGLContextClosing()
{
    shader.reset();
    uniforms.reset();
}

void SpringsGL::renderOpenGL()
{
    jassert(juce::OpenGLHelpers::isContextActive());

    // Setup Viewport
    const float renderingScale = (float)openGLContext.getRenderingScale();
    juce::gl::glViewport(0, 0, juce::roundToInt(renderingScale * getWidth()),
                         juce::roundToInt(renderingScale * getHeight()));

    // Set background Color
    juce::OpenGLHelpers::clear(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // Enable Alpha Blending
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA,
                          juce::gl::GL_ONE_MINUS_SRC_ALPHA);

    // Use Shader Program that's been defined
    shader->use();

    // Setup the Uniforms for use in the Shader

    if (uniforms->resolution != nullptr)
        uniforms->resolution->set((GLfloat)renderingScale * getWidth(),
                                  (GLfloat)renderingScale * getHeight());
    if (uniforms->time != nullptr)
        uniforms->time->set((GLfloat)(++itime) / 60.f);

    if (uniforms->rms != nullptr)
        uniforms->rms->set((GLfloat *)&rms[0][0], RMS_BUFFER_SIZE * MAXSPRINGS);

    if (uniforms->rmspos != nullptr) uniforms->rmspos->set((GLint)*rmspos);

    // Define Vertices for a Square (the view plane)
    GLfloat vertices[] = {
        1.0f,  1.0f,  0.0f, // Top Right
        1.0f,  -1.0f, 0.0f, // Bottom Right
        -1.0f, -1.0f, 0.0f, // Bottom Left
        -1.0f, 1.0f,  0.0f  // Top Left
    };
    // Define Which Vertex Indexes Make the Square
    GLuint indices[] = {
        // Note that we start from 0!
        0, 1, 3, // First Triangle
        1, 2, 3  // Second Triangle
    };

    // Vertex Array Object stuff for later
    // openGLContext.extensions.glGenVertexArrays(1, &VAO);
    // openGLContext.extensions.glBindVertexArray(VAO);

    // VBO (Vertex Buffer Object) - Bind and Write to Buffer
    openGLContext.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, VBO);
    openGLContext.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER,
                                          sizeof(vertices), vertices,
                                          juce::gl::GL_STREAM_DRAW);
    // GL_DYNAMIC_DRAW or GL_STREAM_DRAW
    // Don't we want GL_DYNAMIC_DRAW since this
    // vertex data will be changing alot??
    // test this

    // EBO (Element Buffer Object) - Bind and Write to Buffer
    openGLContext.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER,
                                          EBO);
    openGLContext.extensions.glBufferData(juce::gl::GL_ELEMENT_ARRAY_BUFFER,
                                          sizeof(indices), indices,
                                          juce::gl::GL_STREAM_DRAW);
    // GL_DYNAMIC_DRAW or GL_STREAM_DRAW
    // Don't we want GL_DYNAMIC_DRAW since this
    // vertex data will be changing alot??
    // test this

    // Setup Vertex Attributes
    openGLContext.extensions.glVertexAttribPointer(
        0, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, 3 * sizeof(GLfloat),
        (GLvoid *)0);
    openGLContext.extensions.glEnableVertexAttribArray(0);

    // Draw Vertices
    // glDrawArrays (GL_TRIANGLES, 0, 6); // For just VBO's (Vertex Buffer
    // Objects)
    juce::gl::glDrawElements(juce::gl::GL_TRIANGLES, 6,
                             juce::gl::GL_UNSIGNED_INT,
                             0); // For EBO's (Element Buffer Objects) (Indices)

    // Reset the element buffers so child Components draw correctly
    openGLContext.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    openGLContext.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, 0);
    // openGLContext.extensions.glBindVertexArray(0);
}

void SpringsGL::createShaders()
{
    constexpr char vertexShader[] = "attribute vec3 position;\n"
                                    "\n"
                                    "void main()\n"
                                    "{\n"
                                    "    gl_Position = vec4(position, 1.0);\n"
                                    "}\n";

    std::unique_ptr<juce::OpenGLShaderProgram> shaderProgramAttempt =
        std::make_unique<juce::OpenGLShaderProgram>(openGLContext);

    // Sets up pipeline of shaders and compiles the program
    if (shaderProgramAttempt->addVertexShader(
            juce::OpenGLHelpers::translateVertexShaderToV3(vertexShader)) &&
        shaderProgramAttempt->addFragmentShader(
            juce::OpenGLHelpers::translateFragmentShaderToV3(
                "#define RMS_BUFFER_SIZE " + juce::String(RMS_BUFFER_SIZE) +
                "\n#define MAXSPRINGS " + juce::String(MAXSPRINGS) + "\n" +
                juce::String(BinaryData::spring_shader))) &&
        shaderProgramAttempt->link()) {
        uniforms.reset();
        shader = std::move(shaderProgramAttempt);
        uniforms.reset(new Uniforms(openGLContext, *shader));
    } else {
    }
}
