// Andrew Frost

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>

#include "perlin.h"

const static int   g_winWidth  = 400;
const static int   g_winHeight = 400;
const static int   perlinSize  = 80;
const static float scale       = 0.2f;

// Very wasteful, but easy to read
static const char* vertexShaderText =
"#version 110                                  \n"
"uniform mat4 MVP;                             \n"
"attribute vec2 vPos;                          \n"
"varying vec2 texcoord;                        \n"
"void main()                                   \n"
"{                                             \n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0); \n"
"    texcoord = vPos;                          \n"
"}                                             \n";

static const char* fragShaderText =
"#version 110                                                            \n"
"uniform sampler2D texture;                                              \n"
"uniform vec3 color;                                                     \n"
"uniform float time;                                                     \n"
"varying vec2 texcoord;                                                  \n"
"void main()                                                             \n"
"{                                                                       \n"
"    gl_FragColor = vec4(color * texture2D(texture, vec2(texcoord.x + time, texcoord.y + time)).rgb, 1.0); \n"
"}                                                                       \n";

static const glm::vec2 vertices[4] =
{
    { 0.f, 0.f },
    { 1.f, 0.f },
    { 1.f, 1.f },
    { 0.f, 1.f }
};

///////////////////////////////////////////////////////////////////////////
// GLFW Callbacks                                                        //
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
// GLFW on Error Callback
//
static void onErrorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

//-------------------------------------------------------------------------
// Key Callback
//
static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, 1);
        break;
    }
}

///////////////////////////////////////////////////////////////////////////
// Main / Entry Point                                                    //
///////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) 
{
    (void)(argc), (void)(argv);

    // GUI
    glfwSetErrorCallback(onErrorCallback);
    if (!glfwInit()) return EXIT_FAILURE;


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(g_winWidth, g_winHeight, "Noise Generator", nullptr, nullptr);
    if (window == NULL) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return EXIT_FAILURE;

    glfwSwapInterval(1);

    GLuint texture, program, vertexBuffer;
    GLint mvpLocation, vposLocation, colorLocation, textureLocation;
    GLint textureOffset;

    unsigned int timer = 0;

    Perlin perlin = Perlin(perlinSize * scale, 12u);
    // create OpenGL objects
    {
        char pixels[perlinSize * perlinSize];
        GLuint vertexShader, fragShader;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        srand((unsigned int)glfwGetTimerValue());

        float p;
        for (int y = 0; y < perlinSize; y++) {
            for (int x = 0; x < perlinSize; x++) {
                // Perlin Noise
                p = perlin.octave(x * scale, y * scale, 0.f, 2, 0.2f);
                pixels[y * perlinSize + x] = (char)(p * 255);
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, perlinSize, perlinSize, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderText, NULL);
        glCompileShader(vertexShader);

        fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragShader, 1, &fragShaderText, NULL);
        glCompileShader(fragShader);

        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        mvpLocation = glGetUniformLocation(program, "MVP");
        colorLocation = glGetUniformLocation(program, "color");
        textureLocation = glGetUniformLocation(program, "texture");
        vposLocation = glGetAttribLocation(program, "vPos");
        textureOffset = glGetUniformLocation(program, "time");

        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }

    glUseProgram(program);
    glUniform1i(textureLocation, 0);
    glUniform1f(textureOffset, 0);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glEnableVertexAttribArray(vposLocation);
    glVertexAttribPointer(vposLocation, 2, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*)0);

    // GUI Loop
    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        timer = (timer + 1) % (perlinSize*3);
        float offset = (float)(timer) / (perlinSize*3);
        const glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);

        int width, height;
        glm::mat4 mvp;

        glfwGetFramebufferSize(window, &width, &height);
        glfwMakeContextCurrent(window);

        glUniform1f(textureOffset, offset);

        glViewport(0, 0, width, height);

        mvp = glm::ortho(0.f, 1.f, 0.f, 1.f, 0.f, 1.f);
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, (const GLfloat*)glm::value_ptr(mvp));
        glUniform3fv(colorLocation, 1, glm::value_ptr(color));
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}