// main.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <stddef.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shaders.h"
#include "image_paths.h"

//#define SPRITE_COUNT suki_sprites
#define SPRITE_COUNT 360
// int WINDOW_WIDTH  = 1280;
// int WINDOW_HEIGHT = 720;
int WINDOW_WIDTH  = 1280;
int WINDOW_HEIGHT = 720;


typedef struct {
    int width, height;
    GLuint textureID;
} texture;

typedef struct {
    float x, y;
    float rot;
    float scale;
    texture texture;
} spite;

typedef struct {
    GLuint bufferId;
    GLuint colorTexture;
    int renderWidth, renderHeight;
} framebuffer;


#define DEFAULT_DRAW_WIDTH 1280.0
#define DEFAULT_DRAW_HEIGHT 720.0

framebuffer drawBuffer = {
    0,
    0,
    DEFAULT_DRAW_WIDTH,
    DEFAULT_DRAW_HEIGHT
};
float GlobalScale = 1;


static const char* gl_error_string(GLenum error) {
    switch (error) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default: return "UNKNOWN_ERROR";
    }
}

static void check_gl_errors(const char* file, int line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL Error at %s:%d - %s (0x%x)\n",
               file, line, gl_error_string(error), error);
    }
}

#define CHECK_GL_ERRORS() check_gl_errors(__FILE__, __LINE__)



static GLuint makeShaderProgram(const GLuint frag, const GLuint vert) {
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vert);
    glAttachShader(shaderProgram, frag);
    glLinkProgram(shaderProgram);

    CHECK_GL_ERRORS();

    return shaderProgram;
}

static texture* loadTextures(const char** paths, const size_t pathsc)
{
    texture* tex = malloc(sizeof(texture) * pathsc);
    GLuint* idsIDK = malloc(sizeof(GLuint) * pathsc);
    glGenTextures(pathsc, idsIDK);

    for (size_t i = 0; i < pathsc; ++i) {
        int w, h, n;
        unsigned char* data = stbi_load(paths[i], &w, &h, &n, 4);
        if (!data) {
            fprintf(stderr, "Failed to load '%s': %s\n", paths[i], stbi_failure_reason());
            return nullptr;
        }

        glBindTexture(GL_TEXTURE_2D, idsIDK[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        tex[i].width = w;
        tex[i].height = h;
        tex[i].textureID = idsIDK[i];

        stbi_image_free(data);
    }
    CHECK_GL_ERRORS();

    free(idsIDK);
    return tex;
}

GLuint loadShaderDir(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    if (!shader) {
        fprintf(stderr, "Failed to create shader of type: %d\n", type);
        return 0;
    }

    glShaderSource(shader, 1, (const GLchar**)&source, NULL);

    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        GLchar *infoLog = (GLchar *)malloc(logLength);
        if (infoLog) {
            glGetShaderInfoLog(shader, logLength, NULL, infoLog);
            fprintf(stderr, "Shader compilation failed: %s (%s)\n", infoLog, source);
            free(infoLog);
        } else {
            fprintf(stderr, "Shader compilation failed, but error log could not be retrieved\n");
        }

        glDeleteShader(shader);
        return 0;
    }
    CHECK_GL_ERRORS();
    return shader;
}

GLuint quadVAO, quadVBO, quadEBO;

void setupQuad() {
    const float vertices[] = {
        // positions        // normals      // texcoords
        1.0f,  1.0f, 0.0f,  0,0,1,          1.0f, 1.0f,
       -1.0f,  1.0f, 0.0f,  0,0,1,          0.0f, 1.0f,
       -1.0f, -1.0f, 0.0f,  0,0,1,          0.0f, 0.0f,
        1.0f, -1.0f, 0.0f,  0,0,1,          1.0f, 0.0f,
   };

    const unsigned int indices[] = {
        0, 1, 2, // First triangle
        0, 2, 3  // Second triangle
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    CHECK_GL_ERRORS();
    //glBindVertexArray(0);
}


void createTransformationMatrix(float* matrix, const float translateX, const float translateY, float scaleX, float scaleY, const float rotation) {
    memset(matrix, 0, sizeof(float) * 16);
    matrix[15] = 1.0f;

    matrix[0] = cosf(rotation) * scaleX;
    matrix[1] = sinf(rotation) * scaleX;
    matrix[4] = -sinf(rotation) * scaleY;
    matrix[5] = cosf(rotation) * scaleY;
    matrix[12] = translateX;
    matrix[13] = translateY;
}

void createOrthographicMatrix(float* matrix, const float left, const float right,const float bottom, const float top, const float near, const float far) {
    memset(matrix, 0, sizeof(float) * 16);
    matrix[0] = 2.0f / (right - left);
    matrix[5] = 2.0f / (top - bottom);
    matrix[10] = -2.0f / (far - near);
    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(far + near) / (far - near);
    matrix[15] = 1.0f;
}

void changeShader(const GLuint* shader, const size_t choose, float width, float height) {
    float projMatrix[16];
    createOrthographicMatrix(projMatrix, 0, width, 0, height, -1.0f, 1.0f);

    glUseProgram(shader[choose]);

    GLint projLoc = glGetUniformLocation(shader[choose], "projection");
    if (projLoc != -1)
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix);

    if (choose == 5) {
        GLint sharpnessLoc = glGetUniformLocation(shader[choose], "u_SharpnessAmount");
        if (sharpnessLoc != -1) {
            glUniform1f(sharpnessLoc, 0.5f);
        }
    }else if (choose == 2 || choose == 7) {
        const GLint scaleFactorLoc = glGetUniformLocation(shader[choose], "u_ScaleFactor");
        if (scaleFactorLoc != -1) {
            glUniform1f(scaleFactorLoc, 4);
        }
        const GLint lanczosALoc = glGetUniformLocation(shader[choose], "u_LanczosA");
        if (lanczosALoc != -1) {
            glUniform1i(lanczosALoc, 2); 
        }
    }

    CHECK_GL_ERRORS();
}

void shuffle_sprites(spite* sprites, const size_t spritec) {
    for (size_t i = 0; i < spritec; i++) {
        spite temp = sprites[i];
        size_t j = rand() % spritec;
        sprites[i] = sprites[j];
        sprites[j] = temp;
    }
}

void calculateViewportWithAspectRatio(const int windowWidth, const int windowHeight, const int targetWidth, const  int targetHeight,  int* viewportX, int* viewportY, int* viewportWidth, int* viewportHeight) {
    //const float targetAspect = (float)targetWidth / (float)targetHeight;
    //const float windowAspect = (float)windowWidth / (float)windowHeight;

    //if (windowAspect > targetAspect) {
    //    *viewportHeight = windowHeight;
    //    *viewportWidth = (int)(windowHeight * targetAspect);
    //    *viewportX = (windowWidth - *viewportWidth) / 2;
    //    *viewportY = 0;
    //} else {
    //    *viewportWidth = windowWidth;
    //    *viewportHeight = (int)(windowWidth / targetAspect);
    //    *viewportX = 0;
    //    *viewportY = (windowHeight - *viewportHeight) / 2;
    //}

    float screenAspect = (float)windowWidth / (float)windowHeight;
    float contentAspect = (float)targetWidth / (float)targetHeight;

    if (contentAspect > screenAspect) {
        *viewportWidth = (float)windowWidth;
        *viewportHeight = windowWidth / contentAspect;
        *viewportX = 0;
        *viewportY = (windowHeight - *viewportHeight) / 2.0f;
    } else {
        *viewportWidth = windowHeight * contentAspect;
        *viewportHeight = (float)windowHeight;
        *viewportX = (windowWidth - *viewportWidth) / 2.0f;
        *viewportY = 0;
    }

}

void createFBOs(framebuffer* normalFBO, framebuffer* msaaFBO, const int width, const int height) {
    if (normalFBO->bufferId < 0)
        glDeleteFramebuffers(1, &normalFBO->bufferId);
    if (normalFBO->colorTexture < 0)
        glDeleteTextures(1, &normalFBO->colorTexture);

    if (msaaFBO->bufferId < 0)
        glDeleteFramebuffers(1, &msaaFBO->bufferId);
    if (msaaFBO->colorTexture < 0)
        glDeleteTextures(1, &msaaFBO->colorTexture);


    normalFBO->renderWidth = width;
    normalFBO->renderHeight = height;

    msaaFBO->renderWidth = width;
    msaaFBO->renderHeight = height;

    GlobalScale = (float)width / DEFAULT_DRAW_WIDTH;

    glGenFramebuffers(1, &normalFBO->bufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, normalFBO->bufferId);

    glGenTextures(1, &normalFBO->colorTexture);
    glBindTexture(GL_TEXTURE_2D, normalFBO->colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, normalFBO->colorTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer is not complete: %d\n", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        glDeleteFramebuffers(1, &normalFBO->bufferId);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    glGenFramebuffers(1, &msaaFBO->bufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO->bufferId);

    glGenTextures(1, &msaaFBO->colorTexture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaFBO->colorTexture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, drawBuffer.renderWidth, drawBuffer.renderHeight, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaaFBO->colorTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "MSAA Framebuffer is not complete\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

const char* title = "lebron james NOTHING (hot)";

int main(const int argc, char **argv)
{
#ifdef linux
    setenv("SDL_VIDEODRIVER", "wayland", 1);
#endif
    srand((unsigned)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    const char* videoDriver = SDL_GetCurrentVideoDriver();
    printf("Current SDL Video Driver: %s\n", videoDriver);   

    SDL_Window *win = SDL_CreateWindow(
        title,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }


    SDL_GLContext gl_ctx = SDL_GL_CreateContext(win);
    if (!gl_ctx) {
        fprintf(stderr, "SDL_GL_CreateContext error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

     if (!SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1))
        fprintf(stderr, "SDL_GL_MULTISAMPLEBUFFERS error: %s\n", SDL_GetError());

    if (!SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16))
        fprintf(stderr, "SDL_GL_MULTISAMPLESAMPLES error: %s\n", SDL_GetError());


    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (SDL_GL_SetSwapInterval(0) < 0) {
        fprintf(stderr, "Warning: could not disable VSync: %s\n", SDL_GetError());
    }
    CHECK_GL_ERRORS();

    texture* allSprites = loadTextures(images, suki_sprites);

    spite* sprites = (spite*)malloc(sizeof(spite) * SPRITE_COUNT);
    size_t texture = 159;

    for (int i = 0; i < SPRITE_COUNT; i++) {
        sprites[i] = (spite){ 0 };
        sprites[i].texture = allSprites[texture];
        sprites[i].scale = 0.25f ;
        sprites[i].x = rand() % drawBuffer.renderWidth;
        sprites[i].y = rand() % drawBuffer.renderHeight;

        texture = ++texture % suki_sprites;
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, drawBuffer.renderWidth, drawBuffer.renderHeight);
    framebuffer msaaFBO = { 0 };

    createFBOs(&drawBuffer, &msaaFBO, drawBuffer.renderWidth, drawBuffer.renderHeight);

    /*glGenFramebuffers(1, &drawBuffer.bufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, drawBuffer.bufferId);
    glGenTextures(1, &drawBuffer.colorTexture);
    glBindTexture(GL_TEXTURE_2D, drawBuffer.colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, drawBuffer.renderWidth, drawBuffer.renderHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, drawBuffer.colorTexture, 0);
    CHECK_GL_ERRORS();
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer is not complete: %d\n", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        glDeleteFramebuffers(1, &drawBuffer.bufferId);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }

    framebuffer msaaFBO = { 0 };
    glGenFramebuffers(1, &msaaFBO.bufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO.bufferId);

    glGenTextures(1, &msaaFBO.colorTexture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaFBO.colorTexture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, drawBuffer.renderWidth, drawBuffer.renderHeight, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaaFBO.colorTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "MSAA Framebuffer is not complete\n");
    }*/
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    GLuint vertexShader = loadShaderDir(norm_vert_shader, GL_VERTEX_SHADER);

    GLuint shaders[] = {
        makeShaderProgram(loadShaderDir(simple_frag_shader, GL_FRAGMENT_SHADER), vertexShader),
        makeShaderProgram(loadShaderDir(bicubic_frag_shader,GL_FRAGMENT_SHADER), vertexShader),
        makeShaderProgram(loadShaderDir(lanczos_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
        makeShaderProgram(loadShaderDir(mitchell_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
        makeShaderProgram(loadShaderDir(catmull_rom_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
        makeShaderProgram(loadShaderDir(adaptive_sharpen_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
        makeShaderProgram(loadShaderDir(fsr_like_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
        makeShaderProgram(loadShaderDir(sharp_lanczos_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
    };
    size_t shaderUse = 0;

    setupQuad();

    changeShader(shaders, shaderUse = 0, (float)drawBuffer.renderWidth, (float)drawBuffer.renderHeight);
    glBindVertexArray(quadVAO);
    CHECK_GL_ERRORS();

    bool msaaEnabled = true;

    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 last_counter = SDL_GetPerformanceCounter();
    double deltaTime = 0.0;

    double fpsTimer = 0.0;
    int frameCount = 0;

    int running = 1;

    bool freezeSprites = false;

    while (running) {
        Uint64 current_counter = SDL_GetPerformanceCounter();
        deltaTime = (double)(current_counter - last_counter) / (double)perf_freq;
        last_counter = current_counter;

        fpsTimer += deltaTime;
        frameCount++;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = 0;
            }
            else if (ev.type == SDL_EVENT_WINDOW_RESIZED) {
                glViewport(0, 0, WINDOW_WIDTH = ev.window.data1, WINDOW_HEIGHT = ev.window.data2);
                printf("resized (%d,%d)\n", ev.window.data1, ev.window.data2);
            } else if (ev.type == SDL_EVENT_KEY_DOWN) {
                switch (ev.key.key) {
                    case SDLK_R:
                        shuffle_sprites(sprites, SPRITE_COUNT);
                        break;
                    case SDLK_M:
                        msaaEnabled = !msaaEnabled;
                        printf("MSAA %s\n", msaaEnabled ? "enabled" : "disabled");
                        break;
                    case SDLK_F:
                        freezeSprites = !freezeSprites;
                        break;
                    case SDLK_F1:
                        createFBOs(&drawBuffer, &msaaFBO, 1280, 720);
                        printf("Rendering game at 1280x720\n");
                        break;
                    case SDLK_F2:
                        printf("global scale start: %f", GlobalScale);
                        createFBOs(&drawBuffer, &msaaFBO, 1920, 1080);
                        printf(" global scale end: %f\n", GlobalScale);
                        printf("Rendering game at 1920x1080\n");
                        break;
                    case SDLK_F3:
                        createFBOs(&drawBuffer, &msaaFBO, 2560, 1440);
                        printf("Rendering game at 2560x1440\n");
                        break;
                    case SDLK_F4:
                        createFBOs(&drawBuffer, &msaaFBO, 3840, 2160);
                        printf("Rendering game at 3840x2160\n");
                        break;
                    case SDLK_F5:
                        createFBOs(&drawBuffer, &msaaFBO, 7680, 4320);
                        printf("Rendering game at 7680x4320\n");
                        break;
                    case SDLK_F6:
                        createFBOs(&drawBuffer, &msaaFBO, 15360, 8640);
                        printf("Rendering game at 15360x8640\n");
                    break;
                    case SDLK_1:
                        shaderUse = 0;
                        break;
                    case SDLK_2:
                        shaderUse = 1;
                        break;
                    case SDLK_3:
                        shaderUse = 2;
                        break;
                    case SDLK_4:
                        shaderUse = 3;
                        break;
                    case SDLK_5:
                        shaderUse = 4;
                        break;
                    case SDLK_6:
                        shaderUse = 5;
                        break;
                    case SDLK_7:
                         shaderUse = 6;
                        break;
                    case SDLK_8:
                         shaderUse = 7;
                        break;
                    default: ;
                }
            }
        }

        glClearColor(0, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        glBindFramebuffer(GL_FRAMEBUFFER, msaaEnabled ? msaaFBO.bufferId : drawBuffer.bufferId);
        glViewport(0, 0, drawBuffer.renderWidth, drawBuffer.renderHeight);
        glClearColor(100/255.0f, 149/255.0f, 237/255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        CHECK_GL_ERRORS();


        changeShader(shaders, 0, drawBuffer.renderWidth, drawBuffer.renderHeight);

        int i = 0;
        for (int j = 0; j < SPRITE_COUNT; ++j) {
            glBindTexture(GL_TEXTURE_2D, sprites[i].texture.textureID);
            if (!freezeSprites) {
                sprites[i].x += ((rand() % 2 == 0 ? 1 : -1)) *((rand() % drawBuffer.renderWidth) / 5000.0f - 0.01f) * (float)(deltaTime * 60.0f);
                if (sprites[i].x > drawBuffer.renderWidth) sprites[i].x = 0;
                if (sprites[i].x < 0) sprites[i].x = drawBuffer.renderWidth;

                sprites[i].y += ((rand() % 2 == 0 ? 1 : -1)) * ((rand() % drawBuffer.renderHeight) / 5000.0f - 0.01f) * (float)(deltaTime * 60.0f);
                if (sprites[i].y > drawBuffer.renderHeight) sprites[i].y = 0;
                if (sprites[i].y < 0) sprites[i].y = drawBuffer.renderHeight;
                sprites[i].rot += ((rand() % 100) / 500.0f - 0.1f) * (float)(deltaTime * 30.0f);
            }
            float modelMatrix[16];
            createTransformationMatrix(modelMatrix, sprites[i].x * GlobalScale, sprites[i].y * GlobalScale, sprites[i].texture.width * sprites[i].scale* GlobalScale, -sprites[i].texture.height * sprites[i].scale* GlobalScale, sprites[i].rot);

            const GLint modelLoc = glGetUniformLocation(shaders[shaderUse], "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
            CHECK_GL_ERRORS();

            i = ++i % SPRITE_COUNT;
        }

        if (msaaEnabled) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO.bufferId);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawBuffer.bufferId);
            glBlitFramebuffer(
                0, 0, drawBuffer.renderWidth, drawBuffer.renderHeight,
                0, 0, drawBuffer.renderWidth, drawBuffer.renderHeight,
                GL_COLOR_BUFFER_BIT, GL_LINEAR
            );
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindTexture(GL_TEXTURE_2D, drawBuffer.colorTexture);
        CHECK_GL_ERRORS();

        int viewportX, viewportY, viewportWidth, viewportHeight;
        calculateViewportWithAspectRatio(WINDOW_WIDTH, WINDOW_HEIGHT, drawBuffer.renderWidth, drawBuffer.renderHeight, &viewportX, &viewportY, &viewportWidth, &viewportHeight);

        changeShader(shaders, shaderUse, viewportWidth, viewportHeight);
        glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

        float modelMatrix[16];
        createTransformationMatrix(modelMatrix, 0, 0, viewportWidth, viewportHeight, 0);
        const GLint modelLoc = glGetUniformLocation(shaders[shaderUse], "model");
        if (modelLoc != -1)
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix);
        else
            fprintf(stderr, "modelLoc not found");

        if (shaderUse != 0) {
            const GLint texSizeLoc = glGetUniformLocation(shaders[shaderUse], "u_TextureSize");
            if (texSizeLoc != -1) {
                glUniform2f(texSizeLoc, (float)viewportWidth, (float)viewportHeight);
            }
        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
        CHECK_GL_ERRORS();


        SDL_GL_SwapWindow(win);

        if (fpsTimer >= 1.0) {
            double fps = frameCount / fpsTimer;
            char windowTitle[256];
            snprintf(windowTitle, sizeof(windowTitle), "%s FPS: %.2f", title, fps);
            printf("FPS: %.2f\n", fps);
            SDL_SetWindowTitle(win, windowTitle);
            frameCount = 0;
            fpsTimer = 0.0;
        }
    }
    glDeleteTextures(1, &drawBuffer.colorTexture);
    glDeleteFramebuffers(1, &drawBuffer.bufferId);

    free(allSprites);
    free(sprites);
//    glDeleteTextures(suki_sprites, allSprites);
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
