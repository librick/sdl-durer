#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <emscripten.h>

#define SCREEN_FPS 60
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 640
#define WINDOW_TITLE "Melencholia 1.0"
#define ROTATION_SPEED 0.04
#define DURER_SOLID_PATH "./res/durer-solid.obj"
#define HARE_PATH "./res/hare.png"

struct vec3d {
    float x,y,z;
};

struct triangle {
    vec3d p[3];
};

struct mesh {
    std::vector<triangle> tris;

    bool LoadFromObjectFile(std::string filename) {
        std::ifstream f(filename);
        if (!f.is_open())
            return false;

        std::vector<vec3d> verts;

        while(!f.eof()) {
            // Assume no line exceeds 128 chars
            char line[128];
            f.getline(line, 128);

            std::stringstream s;
            s << line;
            
            char junk;
            if(line[0] == 'v') {
                vec3d v;
                s >> junk >> v.x >> v.y >> v.z;
                verts.push_back(v);
            }
            
            if(line[0] == 'f') {
                int f[3];
                s >> junk >> f[0] >> f[1] >> f[2];
                tris.push_back({
                    verts[f[0] - 1],
                    verts[f[1] - 1],
                    verts[f[2] - 1]
                });
            }
        }

        return true;
    } 
};

struct mat4x4 {
    float m[4][4] = { 0 };
};

void multiplyVectorMat(vec3d &i, vec3d &o, mat4x4 &m) {
    o.x = i.x*m.m[0][0] + i.y*m.m[1][0] + i.z*m.m[2][0] + m.m[3][0];
    o.y = i.x*m.m[0][1] + i.y*m.m[1][1] + i.z*m.m[2][1] + m.m[3][1];
    o.z = i.x*m.m[0][2] + i.y*m.m[1][2] + i.z*m.m[2][2] + m.m[3][2];
    float w = i.x*m.m[0][3] + i.y*m.m[1][3] + i.z*m.m[2][3] + m.m[3][3];
    if(w != 0.0f) {
        o.x /= w; o.y /= w; o.z /= w;
    }
}

void shiftVectorX(vec3d &i, vec3d &o, int x) {
    o.x = i.x + x;
    o.y = i.y;
    o.z = i.z;
}

int getColor(int i, int j) {
    if(i == 0 && j == 0) return 255;
    if(i == 0 && j == 1) return 0;
    if(i == 0 && j == 2) return 0;
    if(i == 1 && j == 0) return 0;
    if(i == 1 && j == 1) return 255;
    if(i == 1 && j == 2) return 0;
    if(i == 2 && j == 0) return 0;
    if(i == 2 && j == 1) return 0;
    if(i == 2 && j == 2) return 255;
    return 0;
}

void drawTriangle(SDL_Renderer *renderer, float x1, float y1, float x2, float y2, float x3, float y3) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawLine(renderer, int(x1), int(y1), int(x2), int(y2));
    SDL_RenderDrawLine(renderer, int(x2), int(y2), int(x3), int(y3));
    SDL_RenderDrawLine(renderer, int(x3), int(y3), int(x1), int(y1));

    SDL_Vertex vert[3];
    float trans = 0.80f;
    int alpha = int(trans*255);
    // center
    vert[0].position.x = x1;
    vert[0].position.y = y1;
    vert[0].color.r = getColor(0,0);
    vert[0].color.g = getColor(0,1);
    vert[0].color.b = getColor(0,2);
    vert[0].color.a = alpha;
    // left
    vert[1].position.x = x2;
    vert[1].position.y = y2;
    vert[1].color.r = getColor(1,0);
    vert[1].color.g = getColor(1,1);
    vert[1].color.b = getColor(1,2);
    vert[1].color.a = alpha;
    // right 
    vert[2].position.x = x3;
    vert[2].position.y = y3;
    vert[2].color.r = getColor(2,0);
    vert[2].color.g = getColor(2,1);
    vert[2].color.b = getColor(2,2);
    vert[2].color.a = alpha;
    SDL_RenderGeometry(renderer, NULL, vert, 3, NULL, 0);
}

struct context {
    SDL_Renderer* renderer;
    float fTheta;
    SDL_Texture* hareTex;
    mesh* durerSolidMesh;
    vec3d vCamera;
    mat4x4 matProj;
};

// When transpiled for the web, the infinite while loops
// I would normally use break because we never give control
// back to the browser's process scheduler.
// By isolating the loop function, we can leverage an emscripten-specific
// construct which mimics an infinite loop without blocking the browser.
void mainLoop(void* myContext) {
    // Retrieve params from context void*
    struct context* ctx = (struct context*)myContext;

    SDL_Renderer* renderer = ctx->renderer;
    SDL_Texture* hareTex = ctx->hareTex;
    mesh durerSolidMesh = *(ctx->durerSolidMesh);
    vec3d vCamera = ctx->vCamera;
    mat4x4 matProj = ctx->matProj;

    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return;
        }
    }

    mat4x4 matRotX, matRotY, matRotZ;
    ctx->fTheta += float(ROTATION_SPEED);
    float fTheta = ctx->fTheta;

    // Rotation Z
    matRotZ.m[0][0] = cosf(fTheta);
    matRotZ.m[0][1] = sinf(fTheta);
    matRotZ.m[1][0] = -sinf(fTheta);
    matRotZ.m[1][1] = cosf(fTheta);
    matRotZ.m[2][2] = 1;
    matRotZ.m[3][3] = 1;
    // Rotation X
    matRotX.m[0][0] = 1;
    matRotX.m[1][1] = cosf(fTheta * 0.5f);
    matRotX.m[1][2] = sinf(fTheta * 0.5f);
    matRotX.m[2][1] = -sinf(fTheta * 0.5f);
    matRotX.m[2][2] = cosf(fTheta * 0.5f);
    matRotX.m[3][3] = 1;
    // Rotation Y
    matRotY.m[0][0] = cosf(fTheta * 0.5f);
    matRotY.m[2][0] = sinf(fTheta * 0.5f);
    matRotY.m[1][1] = 1;
    matRotY.m[0][2] = -sinf(fTheta * 0.5f);
    matRotY.m[1][2] = 0;
    matRotY.m[2][2] = cosf(fTheta * 0.5f);

    SDL_RenderCopy(renderer, hareTex, NULL, NULL);

    for(auto tri : durerSolidMesh.tris) {
        triangle triProjected, triRotated, triTranslated;
        // Rotate in Z axis
        multiplyVectorMat(tri.p[0], triRotated.p[0], matRotY);
        multiplyVectorMat(tri.p[1], triRotated.p[1], matRotY);
        multiplyVectorMat(tri.p[2], triRotated.p[2], matRotY);
        // Translate triangle into scene
        triTranslated = triRotated;
        triTranslated.p[0].z = triTranslated.p[0].z + 3.0f;
        triTranslated.p[1].z = triTranslated.p[1].z + 3.0f;
        triTranslated.p[2].z = triTranslated.p[2].z + 3.0f;

        vec3d normal, line1, line2;
        line1.x = triTranslated.p[1].x - triTranslated.p[0].x;
        line1.y = triTranslated.p[1].y - triTranslated.p[0].y;
        line1.z = triTranslated.p[1].z - triTranslated.p[0].z;

        line2.x = triTranslated.p[2].x - triTranslated.p[0].x;
        line2.y = triTranslated.p[2].y - triTranslated.p[0].y;
        line2.z = triTranslated.p[2].z - triTranslated.p[0].z;

        normal.x = line1.y * line2.z - line1.z * line2.y;
        normal.y = line1.z * line2.x - line1.x * line2.z;
        normal.z = line1.x * line2.y - line1.y * line2.x;

        float l = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
        normal.x /= l; normal.y /= l; normal.z /= l;

        bool isTriVisible = (
            normal.x * (triTranslated.p[0].x - vCamera.x) +
            normal.y * (triTranslated.p[0].y - vCamera.y) +
            normal.z * (triTranslated.p[0].z - vCamera.z)
        ) < 0.0f;

        if(isTriVisible) {
            // Project translated triangle to Cartesian space
            multiplyVectorMat(triTranslated.p[0], triProjected.p[0], matProj);
            multiplyVectorMat(triTranslated.p[1], triProjected.p[1], matProj);
            multiplyVectorMat(triTranslated.p[2], triProjected.p[2], matProj);
            // Scale projected triangle into view
            triProjected.p[0].x += 1.0; triProjected.p[0].y += 1.0;
            triProjected.p[1].x += 1.0; triProjected.p[1].y += 1.0;
            triProjected.p[2].x += 1.0; triProjected.p[2].y += 1.0;
            triProjected.p[0].x *= 0.5 * (float)WINDOW_WIDTH;
            triProjected.p[0].y *= 0.5 * (float)WINDOW_HEIGHT;
            triProjected.p[1].x *= 0.5 * (float)WINDOW_WIDTH;
            triProjected.p[1].y *= 0.5 * (float)WINDOW_HEIGHT;
            triProjected.p[2].x *= 0.5 * (float)WINDOW_WIDTH;
            triProjected.p[2].y *= 0.5 * (float)WINDOW_HEIGHT;
            drawTriangle(
                renderer,
                triProjected.p[0].x, triProjected.p[0].y,
                triProjected.p[1].x, triProjected.p[1].y,
                triProjected.p[2].x, triProjected.p[2].y
            );
        }
    }

    // Draw everything
    SDL_RenderPresent(renderer);
    SDL_RenderClear(renderer);
}

int main(int argc, const char * argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Failed to initialise SDL\n";
        return -1;
    }

    vec3d vCamera;
    vCamera.x = 0.0f;
    vCamera.y = 0.0f;
    vCamera.z = 0.0f;
    mesh durerSolidMesh;
    durerSolidMesh.LoadFromObjectFile(DURER_SOLID_PATH);

    // Create window
    std::string title = WINDOW_TITLE;
    const char* ctitle = title.c_str();
    int posX = SDL_WINDOWPOS_UNDEFINED;
    int posY = SDL_WINDOWPOS_UNDEFINED;
    Uint32 flags = SDL_WINDOW_OPENGL ^ SDL_WINDOW_BORDERLESS;
    SDL_Window *window = SDL_CreateWindow(ctitle, posX, posY,
        WINDOW_WIDTH, WINDOW_HEIGHT, flags);

    if (window == nullptr) {
        SDL_Log("Could not create a window: %s", SDL_GetError());
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        SDL_Log("Could not create a renderer: %s", SDL_GetError());
        return -1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Load hare image
    SDL_Surface* hareSurf = IMG_Load(HARE_PATH);
    if (hareSurf == NULL) {
        printf("failed to load hare sprite\n");
        std::cout << "Error loading image: " << IMG_GetError();
        return -1;
    }
    SDL_Texture* hareTex = SDL_CreateTextureFromSurface(renderer, hareSurf); 
	if (hareTex == NULL) {
        printf("failed to create hare texture\n");
        std::cout << "Error creating texture"; return -1;
    }
	SDL_FreeSurface(hareSurf);

    // Projection matrix
    float fNear = 0.1f;
    float fFar = 1000.0f;
    float fFov = 90.0f;
    float aspectRatio = float(WINDOW_HEIGHT)/float(WINDOW_WIDTH);
    float fFovRad = 1.0f / tan(fFov * 0.5f / 180.0f * 3.14159f);

    mat4x4 matProj;
    matProj.m[0][0] = aspectRatio * fFovRad;
    matProj.m[1][1] = fFovRad;
    matProj.m[2][2] = fFar / (fFar - fNear);
    matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
    matProj.m[2][3] = 1.0f;
    matProj.m[3][3] = 0.0f;

    float fTheta = 2.0f;
    struct context ctx;
    ctx.renderer = renderer;
    ctx.fTheta = 2.0f;
    ctx.hareTex = hareTex;
    ctx.vCamera = vCamera;
    ctx.durerSolidMesh = &durerSolidMesh;
    ctx.matProj = matProj;

    int fps = -1; // Auto-negotiate FPS
    int simulateInfiniteLoop = 1;
    emscripten_set_main_loop_arg(mainLoop, &ctx, fps, simulateInfiniteLoop);

    // Tidy up
    SDL_DestroyTexture(hareTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
