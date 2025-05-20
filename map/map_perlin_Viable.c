#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define WIDTH 800
#define HEIGHT 600
#define MAP_WIDTH 200
#define MAP_HEIGHT 200
#define SCALE 0.04f
#define AMPLITUDE 20.0f
#define NB_TREES 1000
#define NB_ROCKS 1000
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

int p[512];
float heightmap[MAP_WIDTH][MAP_HEIGHT];
typedef struct Node Node;
typedef struct tree tree;
typedef struct rocks rocks;

struct Node{
    int x, y;
    struct Node* left;
    struct Node* right;
};

struct tree{
    int x, y;
};
struct rocks{
    int x, y;
};
rocks rocks_place[NB_ROCKS];
tree forest[NB_TREES];

int search(Node* root, int x, int y){
    if(root == NULL) return 0;
    if(x == root->x && y == root->y) return 1;
    if( x < root->x || (x == root->x && y < root->y)) return search(root->left, x, y);
    else return search(root->right, x, y);
};

Node* insert(Node* root, int x, int y){
    if(root == NULL){
        Node* Node = malloc(sizeof(Node));
        Node->x = x;
        Node->y = y;
        Node->left = Node->right = NULL;
        return Node;
    }
    if(x < root->x || ( x == root->x && y < root->y)){
        root->left = insert(root->left, x, y);
    }
    else if(x > root->x || (x == root->x && y > root->y)){
        root->right = insert(root->right, x, y);
    }
    return root;
}

void init_perlin() {
    int perm[256];
    for (int i = 0; i < 256; i++) perm[i] = i;
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = perm[i];
        perm[i] = perm[j];
        perm[j] = tmp;
    }
    for (int i = 0; i < 256; i++) {
        p[i] = p[i + 256] = perm[i];
    }
}

float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float perlin2D(float x, float y) {
    int xi = (int)floor(x) & 255;
    int yi = (int)floor(y) & 255;
    float xf = x - floor(x);
    float yf = y - floor(y);
    float u = fade(xf);
    float v = fade(yf);

    int aa = p[p[xi] + yi];
    int ab = p[p[xi] + yi + 1];
    int ba = p[p[xi + 1] + yi];
    int bb = p[p[xi + 1] + yi + 1];

    float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), u);
    float x2 = lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), u);
    return (lerp(x1, x2, v) + 1.0f) / 2.0f;
}

void generate_heightmap() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            float noise = perlin2D(x * SCALE, y * SCALE);
            heightmap[x][y] = noise;
        }
    }
}
void set_color(float h) {
    if (h < 0.2f) glColor3f(0.2f, 0.4f, 1.0f);       // eau
    else if (h < 0.5f) glColor3f(0.2f, 0.8f, 0.2f);  // herbe
    else if (h < 0.7f) glColor3f(0.6f, 0.4f, 0.2f);  // terre
    else glColor3f(1.0f, 1.0f, 1.0f);               // neige
}

void generate_forest() {
    Node* root = NULL;
    int count = 0;
    for (int i = 0; i < NB_TREES; i++) {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;

        if(!search(root, x, y)){
            root = insert (root, x, y);
            forest[count].x = rand() % MAP_WIDTH;
            forest[count].y = rand() % MAP_HEIGHT;
            count++;
        }
    }
}

void generate_rock() {
    Node* root = NULL;
    int count = 0;
    for (int i = 0; i < NB_TREES; i++) {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;

        if(!search(root, x, y)){
            root = insert (root, x, y);
            rocks_place[count].x = rand() % MAP_WIDTH;
            rocks_place[count].y = rand() % MAP_HEIGHT;
            count++;
        }
    }
}
void draw_terrain() {
    for (int y = 0; y < MAP_HEIGHT - 1; y++) {
        for (int x = 0; x < MAP_WIDTH - 1; x++) {
            float h = heightmap[x][y];
            float h1 = heightmap[x][y] * AMPLITUDE;
            float h2 = heightmap[x+1][y] * AMPLITUDE;
            float h3 = heightmap[x][y+1] * AMPLITUDE;
            float h4 = heightmap[x+1][y+1] * AMPLITUDE;

            glBegin(GL_TRIANGLES);

            // Triangle 1
            set_color(h1 / AMPLITUDE); glVertex3f(x, h1, y);
            set_color(h2 / AMPLITUDE); glVertex3f(x + 1, h2, y);
            set_color(h3 / AMPLITUDE); glVertex3f(x, h3, y + 1);
            
            // Triangle 2
            set_color(h2 / AMPLITUDE); glVertex3f(x + 1, h2, y);
            set_color(h4 / AMPLITUDE); glVertex3f(x + 1, h4, y + 1);
            set_color(h3 / AMPLITUDE); glVertex3f(x, h3, y + 1);
            
            glEnd();
        }
    }
}

void draw_forest() {
    for (int i = 0; i < NB_TREES; i++) {
        int x = forest[i].x;
        int y = forest[i].y;

        float h = heightmap[x][y];
        float water_level = 0.4f; 
        float mountain_level = 0.6f;
        if( h > water_level && h < mountain_level){
            h = h * AMPLITUDE;
            glPushMatrix();
    
            glTranslatef(x + 0.5f, h - 0.1f, y + 0.2f); // Position de base du tronc
            GLUquadric* quad = gluNewQuadric();
    
            // Tronc vertical (axe Y)
            glRotatef(-90.f, 1.f, 0.f, 0.f);
            glColor3f(0.4f, 0.2f, 0.0f);
            gluCylinder(quad, 0.2f, 0.2f, 1.5f, 16, 4);
    
            // Feuillage sphérique
            float leaf_raduis = 0.6f;
            glTranslatef(0, 0, 1.2f + leaf_raduis);
            glColor3f(0.0f, 0.6f, 0.0f);
            gluSphere(quad, leaf_raduis, 8, 8);
            gluDeleteQuadric(quad);
            glPopMatrix();
        }
    }
}

void draw_rock() {
    for(int i = 0; i < NB_ROCKS; i++){
        int x = rocks_place[i].x;
        int y = rocks_place[i].y;

        float h = heightmap[x][y];
        float water_level = 0.4f; 
        float mountain_level = 0.6f;
        if( h > water_level && h > mountain_level){
            h = h * AMPLITUDE;
            glPushMatrix();
    
            glTranslatef(x + 0.5f, h - 0.1f, y + 0.2f); 
            
            GLUquadric* quad = gluNewQuadric();
            float r = 0.45f + (rand() % 10) / 100.0f; // entre 0.45 et 0.55
            glColor3f(r, r - 0.05f, r - 0.1f);
            gluSphere(quad, 0.6, 8, 8);
            gluDeleteQuadric(quad);
            glPopMatrix();

        }
    }
}

void draw_water(float level) {
    float time = SDL_GetTicks() / 1000.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glColor4f(0.2f, 0.4f, 1.0f, 0.5f);

    for (int y = 0; y < MAP_HEIGHT - 1; y++) {
        for (int x = 0; x < MAP_WIDTH - 1; x++) {
            if (heightmap[x][y] < level || heightmap[x+1][y] < level ||
                heightmap[x][y+1] < level || heightmap[x+1][y+1] < level) {

                float w1 = (level + 0.02f * sinf(0.3f * x + 0.3f * y + time)) * AMPLITUDE;
                float w2 = (level + 0.02f * sinf(0.3f * (x+1) + 0.3f * y + time)) * AMPLITUDE;
                float w3 = (level + 0.02f * sinf(0.3f * x + 0.3f * (y+1) + time)) * AMPLITUDE;
                float w4 = (level + 0.02f * sinf(0.3f * (x+1) + 0.3f * (y+1) + time)) * AMPLITUDE;

                glBegin(GL_TRIANGLES);
                glVertex3f(x,   w1, y);
                glVertex3f(x+1, w2, y);
                glVertex3f(x,   w3, y+1);

                glVertex3f(x+1, w2, y);
                glVertex3f(x+1, w4, y+1);
                glVertex3f(x,   w3, y+1);
                glEnd();
            }
        }
    }

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(WIDTH, HEIGHT, 32, SDL_OPENGL);
    SDL_WM_SetCaption("Terrain 3D", NULL);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat light_pos[] = { 0.0f, 100.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    srand(time(NULL));
    init_perlin();
    generate_heightmap();
    generate_forest();
    generate_rock();

    float camX = MAP_WIDTH / 2, camZ = MAP_HEIGHT / 2;
    float angle = 0.0f;
    float camY = 50.0f;
    const float eye_height = 2.0f;
    const float camSpeed = 0.2f;

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
        }

        // Contrôles clavier
        Uint8* keys = SDL_GetKeyState(NULL);
        if (keys[SDLK_LEFT])  angle -= 0.05f;
        if (keys[SDLK_RIGHT]) angle += 0.05f;
        if (keys[SDLK_UP]) {
            camX += sinf(angle) * camSpeed;
            camZ += cosf(angle) * camSpeed;
        }
        if (keys[SDLK_DOWN]) {
            camX -= sinf(angle) * camSpeed;
            camZ -= cosf(angle) * camSpeed;
        }

        // Mise à jour hauteur caméra
        int ix = (int)camX, iz = (int)camZ;
        if (ix >= 0 && ix < MAP_WIDTH && iz >= 0 && iz < MAP_HEIGHT)
            camY = heightmap[ix][iz] * AMPLITUDE + eye_height;

        // Rendu
        glViewport(0, 0, WIDTH, HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(70.0, (double)WIDTH / HEIGHT, 0.1, 1000.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(camX, camY, camZ,
                  camX + sinf(angle), camY, camZ + cosf(angle),
                  0.0f, 1.0f, 0.0f);

        draw_terrain();
        draw_forest();
        draw_rock();
        draw_water(0.3f);
        SDL_GL_SwapBuffers();
        SDL_Delay(16);
    }

    SDL_Quit();
    return 0;
}
