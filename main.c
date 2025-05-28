// main.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shaders.h"
#include "image_paths.h"

#define SPRITE_COUNT suki_sprites
// int WINDOW_WIDTH  = 1280;
// int WINDOW_HEIGHT = 720;
int WINDOW_WIDTH  = 1920;
int WINDOW_HEIGHT = 1080;


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


void createTransformationMatrix(float* matrix, float translateX, float translateY, float scaleX, float scaleY, float rotation) {
    memset(matrix, 0, sizeof(float) * 16); // Reset to zero
    matrix[15] = 1.0f; // Homogeneous coordinate

    // Translation
    matrix[0] = cosf(rotation) * scaleX;
    matrix[1] = sinf(rotation) * scaleX;
    matrix[4] = -sinf(rotation) * scaleY;
    matrix[5] = cosf(rotation) * scaleY;
    matrix[12] = translateX; // X-axis translation
    matrix[13] = translateY; // Y-axis translation
}

void createOrthographicMatrix(float* matrix, float left, float right, float bottom, float top, float near, float far) {
    memset(matrix, 0, sizeof(float) * 16);
    matrix[0] = 2.0f / (right - left); // Scale X
    matrix[5] = 2.0f / (top - bottom); // Scale Y
    matrix[10] = -2.0f / (far - near); // Scale Z
    matrix[12] = -(right + left) / (right - left); // Translate X
    matrix[13] = -(top + bottom) / (top - bottom); // Translate Y
    matrix[14] = -(far + near) / (far - near); // Translate Z
    matrix[15] = 1.0f; // Homogeneous coordinate
}

void changeShader(const GLuint* shader, const size_t choose) {
    float projMatrix[16];
    createOrthographicMatrix(projMatrix, 0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1.0f, 1.0f);

    glUseProgram(shader[choose]);

    GLint projLoc = glGetUniformLocation(shader[choose], "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix);

    if (choose == 5) {
        GLint sharpnessLoc = glGetUniformLocation(shader[choose], "u_SharpnessAmount");
        if (sharpnessLoc != -1) {
            glUniform1f(sharpnessLoc, 0.5f);
        }
    }else if (choose == 2) { // lanczos
        GLint scaleFactorLoc = glGetUniformLocation(shader[choose], "u_ScaleFactor");
        if (scaleFactorLoc != -1) {
            //glUniform1f(scaleFactorLoc, (float)sprites[i].texture.width / (float)WINDOW_WIDTH);
            glUniform1f(scaleFactorLoc, 4);
        }
        GLint lanczosALoc = glGetUniformLocation(shader[choose], "u_LanczosA");
        if (lanczosALoc != -1) {
            glUniform1i(lanczosALoc, 2); // Common value for Lanczos-3
        }
    }

    // Add missing uniforms for bicubic and lanczos shaders


    CHECK_GL_ERRORS();
}



int main(const int argc, char **argv)
{
#ifdef linux
    setenv("SDL_VIDEODRIVER", "wayland", 1);
#endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    const char* videoDriver = SDL_GetCurrentVideoDriver();
    printf("Current SDL Video Driver: %s\n", videoDriver);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window *win = SDL_CreateWindow(
        "lebron james feet porn (hot)",
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

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);




   if (SDL_GL_SetSwapInterval(0) < 0) {
       fprintf(stderr, "Warning: could not disable VSync: %s\n", SDL_GetError());
   }
    CHECK_GL_ERRORS();

    texture* allSprites = loadTextures(images, suki_sprites);

    spite* sprites = (spite*)malloc(sizeof(spite) * SPRITE_COUNT);
    size_t texture = 0;

    for (int i = 0; i < SPRITE_COUNT; i++) {
        sprites[i] = (spite){ 0 };
        sprites[i].texture = allSprites[texture];
        sprites[i].scale = -900.0f;
        sprites[i].x = rand() % WINDOW_WIDTH;   // Random position in window
        sprites[i].y = rand() % WINDOW_HEIGHT;

        texture = ++texture % suki_sprites;
    }


    srand((unsigned)time(NULL));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Uint64 lastTime    = SDL_GetTicks();
    Uint64 fpsTimer    = lastTime;
    int    frameCount  = 0;

    GLuint vertexShader = loadShaderDir(norm_vert_shader, GL_VERTEX_SHADER);

    GLuint shaders[] = {
        makeShaderProgram(loadShaderDir(simple_frag_shader, GL_FRAGMENT_SHADER), vertexShader),
        makeShaderProgram(loadShaderDir(bicubic_frag_shader,GL_FRAGMENT_SHADER), vertexShader),
        makeShaderProgram(loadShaderDir(lanczos_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
        makeShaderProgram(loadShaderDir(mitchell_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
        makeShaderProgram(loadShaderDir(catmull_rom_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
        makeShaderProgram(loadShaderDir(adaptive_sharpen_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
        makeShaderProgram(loadShaderDir(fsr_like_frag_shader,GL_FRAGMENT_SHADER) , vertexShader),
    };
    size_t shaderUse = 0;

    setupQuad();

    changeShader(shaders, shaderUse = 0);
    glBindVertexArray(quadVAO);
    CHECK_GL_ERRORS();

    int running = 1;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = 0;
            }
            else if (ev.type == SDL_EVENT_WINDOW_RESIZED) {
                glViewport(0, 0, WINDOW_WIDTH = ev.window.data1, WINDOW_HEIGHT = ev.window.data2);
            } else if (ev.type == SDL_EVENT_KEY_DOWN) {
                switch (ev.key.key) {
                    case SDLK_1:
                        changeShader(shaders, shaderUse = 0);
                        break;
                    case SDLK_2:
                        changeShader(shaders, shaderUse = 1);
                        break;
                    case SDLK_3:
                        changeShader(shaders, shaderUse = 2);
                        break;
                    case SDLK_4:
                        changeShader(shaders, shaderUse = 3);
                        break;
                    case SDLK_5:
                        changeShader(shaders, shaderUse = 4);
                        break;
                    case SDLK_6:
                        changeShader(shaders, shaderUse = 5);
                        break;
                    case SDLK_7:
                        changeShader(shaders, shaderUse = 6);
                        break;
                    default: ;
                }
            }
        }

        glClearColor(100/255.0f, 149/255.0f, 237/255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        int i = 0;

        Uint64 now = SDL_GetTicks ();
        float deltaTime = (now - lastTime) / 1000.0f;
        for (int j = 0; j < SPRITE_COUNT; ++j) {
            glBindTexture(GL_TEXTURE_2D, sprites[i].texture.textureID);

            sprites[i].x += ((rand() % 100) / 5000.0f - 0.01f) * deltaTime * 60.0f;
            if (sprites[i].x > WINDOW_WIDTH) sprites[i].x = 0;
            if (sprites[i].x < 0) sprites[i].x = WINDOW_WIDTH;

            sprites[i].y += ((rand() % 100) / 5000.0f - 0.01f) * deltaTime * 60.0f;
            if (sprites[i].y > WINDOW_HEIGHT) sprites[i].y = 0;
            if (sprites[i].y < 0) sprites[i].y = WINDOW_HEIGHT;


            sprites[i].rot += ((rand() % 100) / 500.0f - 0.1f) * deltaTime * 30.0f;

            float modelMatrix[16];
            createTransformationMatrix(modelMatrix, sprites[i].x, sprites[i].y, sprites[i].scale, sprites[i].scale, sprites[i].rot);


            GLint modelLoc = glGetUniformLocation(shaders[shaderUse], "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix);

            if (shaderUse != 0) { // bicubic or lanczos
                GLint texSizeLoc = glGetUniformLocation(shaders[shaderUse], "u_TextureSize");
                if (texSizeLoc != -1) {
                    glUniform2f(texSizeLoc, (float)sprites[i].texture.width, (float)sprites[i].texture.height);
                }
            }


            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
            CHECK_GL_ERRORS();

            i = ++i % SPRITE_COUNT;
        }

        glBindTexture(GL_TEXTURE_2D, 0);


        SDL_GL_SwapWindow(win);

        // FPS counting
        frameCount++;
        Uint64 now2 = SDL_GetTicks ();
        if (now2 - fpsTimer >= 1000) {
            float fps = frameCount * 1000.0f / (now2 - fpsTimer);
            printf("FPS: %.2f\n", fps);
            frameCount = 0;
            fpsTimer   = now2;
        }
        lastTime = now;
    }

    free(allSprites);
    free(sprites);
    glDeleteTextures(suki_sprites, allSprites);
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
