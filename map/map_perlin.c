#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <SDL/SDL_ttf.h>
#include <sys/stat.h>
#include "map_perlin.h"
#include "../drone.h"


// gcc main.c drone.c map/map_perlin.c -o droneSim -lSDL -lGL -lGLU -lm -lSDL_ttf
// sudo apt install libsdl-ttf2.0-dev
int p[512];
int lastChunkX = -9999, lastChunkY = -9999;
extern int mode_fps;
extern DroneState drone;
Vec3 cam_pos = {50.0f, 50.0f, 20.0f};
float cam_yaw = 0.0f, cam_pitch = 0.0f;
char worldName[64] = "world_1";;
chunk* c = NULL;
chunk* loadedChunks[2 * RENDER_DISTANCE + 1][2 * RENDER_DISTANCE + 1];
rocks rocks_place[NB_ROCKS];
tree forest[NB_TREES];
clouds cloud_fields[NB_CLOUDS];


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
    srand(world_seed);  // déterministe avec la seed

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
void generate_chunk(chunk* chunk, int cx, int cy) {
    chunk->chunkX = cx;
    chunk->chunkY = cy;

    for (int y = 0; y <= CHUNK_SIZE; y++) {
        for (int x = 0; x <= CHUNK_SIZE; x++) {
            float worldX = (cx * CHUNK_SIZE + x) * SCALE;
            float worldY = (cy * CHUNK_SIZE + y) * SCALE;
            chunk->heightmap[x][y] = perlin2D(worldX, worldY);
        }
    }
}

// void updatec(void) {
//     cameraState cam = get_current_camera_position();
//     int cx = (int)(cam.x) / CHUNK_SIZE;
//     int cy = (int)(cam.y) / CHUNK_SIZE;

//     if (cx != lastChunkX || cy != lastChunkY) {
//         if (c == NULL) c = malloc(sizeof(chunk));
//         if (!load_chunk(c, cx, cy)) {
//             generate_chunk(c, cx, cy);
//             save_chunk(c);
//         }
//         lastChunkX = cx;
//         lastChunkY = cy;
//     }
// }
void updateChunks() {
    cameraState cam = get_current_camera_position(); 
    int cx = (int)(cam.x) / CHUNK_SIZE;
    int cy = (int)(cam.y) / CHUNK_SIZE;

    chunk* newChunks[2 * RENDER_DISTANCE + 1][2 * RENDER_DISTANCE + 1] = { NULL };

    for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; dx++) {
        for (int dy = -RENDER_DISTANCE; dy <= RENDER_DISTANCE; dy++) {
            int chunkX = cx + dx;
            int chunkY = cy + dy;
            int newIx = dx + RENDER_DISTANCE;
            int newIy = dy + RENDER_DISTANCE;

            // Cherche l'ancien chunk déjà chargé
            int oldIx = chunkX - lastChunkX + RENDER_DISTANCE;
            int oldIy = chunkY - lastChunkY + RENDER_DISTANCE;

            if (oldIx >= 0 && oldIx <= 2 * RENDER_DISTANCE && oldIy >= 0 && oldIy <= 2 * RENDER_DISTANCE) {
                newChunks[newIx][newIy] = loadedChunks[oldIx][oldIy];
                loadedChunks[oldIx][oldIy] = NULL;
            } else {
                chunk* ch = malloc(sizeof(chunk));
                if (!load_chunk(ch, chunkX, chunkY)) {
                    generate_chunk(ch, chunkX, chunkY);
                    save_chunk(ch);
                }
                newChunks[newIx][newIy] = ch;
            }
        }
    }
    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
            if (loadedChunks[i][j]) {
                save_chunk(loadedChunks[i][j]);
                free(loadedChunks[i][j]);
            }
        }
    }
    memcpy(loadedChunks, newChunks, sizeof(newChunks));
    lastChunkX = cx;
    lastChunkY = cy;
}

chunk* get_chunk_at_world(int x, int y) {
    int chunkX = x / CHUNK_SIZE;
    int chunkY = y / CHUNK_SIZE;

    int localX = x % CHUNK_SIZE;
    int localY = y % CHUNK_SIZE;

    if (localX < 0) { chunkX--; localX += CHUNK_SIZE; }
    if (localY < 0) { chunkY--; localY += CHUNK_SIZE; }

    int idxX = chunkX - (lastChunkX - RENDER_DISTANCE);
    int idxY = chunkY - (lastChunkY - RENDER_DISTANCE);

    if (idxX < 0 || idxX >= 2 * RENDER_DISTANCE + 1 || idxY < 0 || idxY >= 2 * RENDER_DISTANCE + 1)
        return NULL;

    return loadedChunks[idxX][idxY];
}

void save_chunk(chunk* chunk) {
    char path[255];
    sprintf(path, "chunks/%s/chunk_%d_%d.bin", worldName, chunk->chunkX, chunk->chunkY);
    mkdir("chunks", 0777);          
    mkdir("chunks/world_1", 0777);
    FILE* f = fopen(path, "wb");
    if (f) {
        fwrite(chunk, sizeof(chunk), 1, f);
        fclose(f);
    }
}

int load_chunk(chunk* chunk, int cx, int cy) {
    char path[255];
    sprintf(path, "chunks/%s/chunk_%d_%d.bin", worldName, cx, cy);
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fread(chunk, sizeof(chunk), 1, f);
    fclose(f);
    return 1;
}
void save_player_state(Vec3 pos) {
    char path[128];
    sprintf(path, "chunks/%s/metadata.txt", worldName);
    FILE* f = fopen(path, "w");
    if (f) {
        fprintf(f, "%lf %lf %lf\n", pos.x, pos.y, pos.z);
        fclose(f);
    }
}

Vec3 load_player_state() {
    Vec3 pos = {CHUNK_SIZE/2, CHUNK_SIZE/2, 10};
    char path[128];
    sprintf(path, "chunks/%s/metadata.txt", worldName);
    FILE* f = fopen(path, "r");
    if (f) {
        fscanf(f, "%lf %lf %lf", &pos.x, &pos.y, &pos.z);
        fclose(f);
    }
    return pos;
}
void generate_heightmap() {
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            float noise = perlin2D(x * SCALE, y * SCALE);
            c->heightmap[x][y] = noise;
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
        int x = rand() % CHUNK_SIZE;
        int y = rand() % CHUNK_SIZE;
        forest[i].height_tree = 4.0f + ((float)rand() / RAND_MAX) * 6.0f;
        if(!search(root, x, y)){
            root = insert (root, x, y);
            forest[count].x = rand() % CHUNK_SIZE;
            forest[count].y = rand() % CHUNK_SIZE;
            count++;
        }
    }
}

void generate_rock() {
    Node* root = NULL;
    int count = 0;
    for (int i = 0; i < NB_TREES; i++) {
        int x = rand() % CHUNK_SIZE;
        int y = rand() % CHUNK_SIZE;
        rocks_place[i].r = 0.45f + (rand() % 10) / 100.0f;
        if(!search(root, x, y)){
            root = insert (root, x, y);
            rocks_place[count].x = rand() % CHUNK_SIZE;
            rocks_place[count].y = rand() % CHUNK_SIZE;
            count++;
        }
    }
}
void generate_clouds() {
    for (int i = 0; i < NB_CLOUDS; i++) {
        cloud_fields[i].x = rand() % CHUNK_SIZE;
        cloud_fields[i].y = rand() % CHUNK_SIZE;
        cloud_fields[i].r = 2.0f + ((float)rand() / RAND_MAX) * 2.0f;
    }
}

const tree* get_forest() {
    return forest;
}
const rocks* get_rocks() {
    return rocks_place;
}
// void draw_terrain_cube(int x, int y, float h) {
//     float h_top = h * AMPLITUDE;
//     float h_bottom = 0.0f; // sol bas
//     set_color(h);
//     glBegin(GL_QUADS);
//     glVertex3f(x,     y,     h_top);
//     glVertex3f(x + 1, y,     h_top);
//     glVertex3f(x + 1, y + 1, h_top);
//     glVertex3f(x,     y + 1, h_top);
//     glEnd();
//     glColor3f(0.1f, 0.1f, 0.1f);
//     glBegin(GL_QUADS);
//     glVertex3f(x,     y,     h_bottom);
//     glVertex3f(x + 1, y,     h_bottom);
//     glVertex3f(x + 1, y + 1, h_bottom);
//     glVertex3f(x,     y + 1, h_bottom);
//     glEnd();
//     // Faces latérales : Nord, Sud, Est, Ouest
//     float neighbor;
//     // N (y+1)
//     if (y + 1 >= CHUNK_SIZE || c->heightmap[x][y + 1] * AMPLITUDE < h_top) {
//         glColor3f(0.15f, 0.15f, 0.15f);
//         glBegin(GL_QUADS);
//         glVertex3f(x,     y + 1, h_bottom);
//         glVertex3f(x + 1, y + 1, h_bottom);
//         glVertex3f(x + 1, y + 1, h_top);
//         glVertex3f(x,     y + 1, h_top);
//         glEnd();
//     }
//     // S (y-1)
//     if (y == 0 || c->heightmap[x][y - 1] * AMPLITUDE < h_top) {
//         glColor3f(0.15f, 0.15f, 0.15f);
//         glBegin(GL_QUADS);
//         glVertex3f(x,     y, h_bottom);
//         glVertex3f(x + 1, y, h_bottom);
//         glVertex3f(x + 1, y, h_top);
//         glVertex3f(x,     y, h_top);
//         glEnd();
//     }
//     // E (x+1)
//     if (x + 1 >= CHUNK_SIZE || c->heightmap[x + 1][y] * AMPLITUDE < h_top) {
//         glColor3f(0.2f, 0.2f, 0.2f);
//         glBegin(GL_QUADS);
//         glVertex3f(x + 1, y,     h_bottom);
//         glVertex3f(x + 1, y + 1, h_bottom);
//         glVertex3f(x + 1, y + 1, h_top);
//         glVertex3f(x + 1, y,     h_top);
//         glEnd();
//     }
//     // O (x-1)
//     if (x == 0 || c->heightmap[x - 1][y] * AMPLITUDE < h_top) {
//         glColor3f(0.2f, 0.2f, 0.2f);
//         glBegin(GL_QUADS);
//         glVertex3f(x, y,     h_bottom);
//         glVertex3f(x, y + 1, h_bottom);
//         glVertex3f(x, y + 1, h_top);
//         glVertex3f(x, y,     h_top);
//         glEnd();
//     }
// }
// void draw_terrain_minecraft() {
//     for (int y = 0; y < CHUNK_SIZE; y++) {
//         for (int x = 0; x < CHUNK_SIZE; x++) {
//             draw_terrain_cube(x, y, c->heightmap[x][y]);
//         }
//     }
// }
void draw_chunk(const chunk* c) {
    int baseX = c->chunkX * CHUNK_SIZE;
    int baseY = c->chunkY * CHUNK_SIZE;

    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            float h1 = c->heightmap[x][y] * AMPLITUDE;
            float h2 = c->heightmap[x+1][y] * AMPLITUDE;
            float h3 = c->heightmap[x][y+1] * AMPLITUDE;
            float h4 = c->heightmap[x+1][y+1] * AMPLITUDE;

            float vx = x + baseX;
            float vy = y + baseY;

            glBegin(GL_TRIANGLES);

            set_color(c->heightmap[x][y]);
            glVertex3f(vx, vy, h1);

            set_color(c->heightmap[x+1][y]);
            glVertex3f(vx + 1, vy, h2);

            set_color(c->heightmap[x][y+1]);
            glVertex3f(vx, vy + 1, h3);

            set_color(c->heightmap[x+1][y]);
            glVertex3f(vx + 1, vy, h2);

            set_color(c->heightmap[x+1][y+1]);
            glVertex3f(vx + 1, vy + 1, h4);

            set_color(c->heightmap[x][y+1]);
            glVertex3f(vx, vy + 1, h3);

            glEnd();
        }
    }
}

void draw_all_chunks() {
    cameraState cam = get_current_camera_position();

    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
            chunk* ch = loadedChunks[i][j];
            if (!ch) continue;

            int dx = ch->chunkX * CHUNK_SIZE + CHUNK_SIZE / 2 - (int)cam.x;
            int dy = ch->chunkY * CHUNK_SIZE + CHUNK_SIZE / 2 - (int)cam.y;
            int dist_sq = dx * dx + dy * dy;

            if (dist_sq < 200 * 200) {
                draw_chunk(ch);
            }
        }
    }
}



void draw_forest() {
    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
            chunk* ch = loadedChunks[i][j];
            if (!ch) continue;

            int baseX = ch->chunkX * CHUNK_SIZE;
            int baseY = ch->chunkY * CHUNK_SIZE;

            for (int k = 0; k < NB_TREES; k++) {
                int lx = forest[k].x;
                int ly = forest[k].y;          
                int wx = baseX + lx;
                int wy = baseY + ly;
                if ((wx / CHUNK_SIZE) != ch->chunkX || (wy / CHUNK_SIZE) != ch->chunkY)
                    continue;

                float h = ch->heightmap[lx][ly];
                if (h > WATER_LEVEL && h < MOUNTAIN_LEVEL) {
                    float height_tree = forest[k].height_tree;

                    glPushMatrix();
                    glTranslatef(wx + 0.5f, wy + 0.2f, h * AMPLITUDE - 0.1f);
                    GLUquadric* quad = gluNewQuadric();

                    glRotatef(0.f, 1.f, 0.f, 0.f);
                    glColor3f(0.4f, 0.2f, 0.0f);
                    gluCylinder(quad, 0.2f, 0.2f, height_tree, 16, 4);

                    float leaf_raduis = 1.6f;
                    glTranslatef(0, 0, (height_tree + leaf_raduis) - 0.2f);
                    glColor3f(0.0f, 0.6f, 0.0f);
                    gluSphere(quad, leaf_raduis, 8, 8);

                    gluDeleteQuadric(quad);
                    glPopMatrix();
                }
            }
        }
    }
}


void draw_rock() {
    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
            chunk* c = loadedChunks[i][j];
            if (!c) continue;

            int baseX = c->chunkX * CHUNK_SIZE;
            int baseY = c->chunkY * CHUNK_SIZE;

            for (int k = 0; k < NB_ROCKS; k++) {
                int lx = rocks_place[k].x;
                int ly = rocks_place[k].y;
                int wx = baseX + lx;
                int wy = baseY + ly;
                if ((wx / CHUNK_SIZE) != c->chunkX || (wy / CHUNK_SIZE) != c->chunkY)
                    continue;

                float h = c->heightmap[lx][ly];
                if (h > WATER_LEVEL && h > MOUNTAIN_LEVEL) {
                    glPushMatrix();
                    glTranslatef(wx + 0.5f, wy + 0.2f, h * AMPLITUDE - 0.1f);

                    GLUquadric* quad = gluNewQuadric();
                    float r = rocks_place[k].r;
                    glColor3f(r, r - 0.05f, r - 0.1f);
                    gluSphere(quad, 0.6, 8, 8);
                    gluDeleteQuadric(quad);
                    glPopMatrix();
                }
            }
        }
    }
}

void draw_water(float level, float time) {
    time = time / 3600.0f;
    float waterHeight = level * AMPLITUDE;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glColor4f(0.2f, 0.4f, 1.0f, 0.5f);

    for (int i = 0; i < 2 * RENDER_DISTANCE + 1; i++) {
        for (int j = 0; j < 2 * RENDER_DISTANCE + 1; j++) {
            chunk* c = loadedChunks[i][j];
            if (!c) continue;

            int baseX = c->chunkX * CHUNK_SIZE;
            int baseY = c->chunkY * CHUNK_SIZE;

            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    if (c->heightmap[x][y] < level || c->heightmap[x+1][y] < level ||
                        c->heightmap[x][y+1] < level || c->heightmap[x+1][y+1] < level) {

                        float z1 = waterHeight + 0.1f * sinf(0.3f * x + 0.3f * y + time);
                        float z2 = waterHeight + 0.1f * sinf(0.3f * (x + 1) + 0.3f * y + time);
                        float z3 = waterHeight + 0.1f * sinf(0.3f * x + 0.3f * (y + 1) + time);
                        float z4 = waterHeight + 0.1f * sinf(0.3f * (x + 1) + 0.3f * (y + 1) + time);

                        glBegin(GL_TRIANGLES);
                        glVertex3f(baseX + x,     baseY + y,     z1);
                        glVertex3f(baseX + x + 1, baseY + y,     z2);
                        glVertex3f(baseX + x,     baseY + y + 1, z3);

                        glVertex3f(baseX + x + 1, baseY + y,     z2);
                        glVertex3f(baseX + x + 1, baseY + y + 1, z4);
                        glVertex3f(baseX + x,     baseY + y + 1, z3);
                        glEnd();
                    }
                }
            }
        }
    }

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
}


void draw_clouds(float time) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING); // Optionnel, à voir si tu veux un style non affecté par lumière
    glColor4f(1.0f, 1.0f, 1.0f, 0.4f); // Blanc translucide

    for (int i = 0; i < NB_CLOUDS; i++) {
        float cx = cloud_fields[i].x + sinf(time * 0.001f + i) * 2.0f;
        float cy = cloud_fields[i].y;
        float cz = AMPLITUDE + 10.0f + sinf(time * 0.002f + i);

        glPushMatrix();
        glTranslatef(cx, cy, cz);
        GLUquadric* quad = gluNewQuadric();
        gluSphere(quad, cloud_fields[i].r, 12, 12);
        gluDeleteQuadric(quad);
        glPopMatrix();
    }

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}


float get_ground_height_at(float x, float y) {
    int xi = (int)floor(x);
    int yi = (int)floor(y);

    chunk* ch = get_chunk_at_world(xi, yi);
    if (!ch) return 0.0f;

    int lx = xi % CHUNK_SIZE;
    int ly = yi % CHUNK_SIZE;
    if (lx < 0) lx += CHUNK_SIZE;
    if (ly < 0) ly += CHUNK_SIZE;

    float dx = x - xi;
    float dy = y - yi;

    float h00 = ch->heightmap[lx][ly] * AMPLITUDE;
    float h10 = ch->heightmap[lx + 1][ly] * AMPLITUDE;
    float h01 = ch->heightmap[lx][ly + 1] * AMPLITUDE;
    float h11 = ch->heightmap[lx + 1][ly + 1] * AMPLITUDE;

    float h;

    if (dx + dy < 1.0f) {
        h = h00 * (1 - dx - dy) + h10 * dx + h01 * dy;
    } else {
        float dx1 = 1 - dx;
        float dy1 = 1 - dy;
        h = h11 * (dx + dy - 1) + h10 * dx1 + h01 * dy1;
    }

    return h;
}


void drawAxes(float len) {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    // Axe X (rouge)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0, 0, 0); glVertex3f(len, 0, 0);
    // Axe Y (vert)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0, 0, 0); glVertex3f(0, len, 0);
    // Axe Z (bleu)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0, 0, 0); glVertex3f(0, 0, len);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f); // réinitialise la couleur
}
Vec3 get_normal_at(float x, float y) {
    int xi = (int)x, yi = (int)y;
    float hL = get_ground_height_at(xi - 1, yi);
    float hR = get_ground_height_at(xi + 1, yi);
    float hD = get_ground_height_at(xi, yi - 1);
    float hU = get_ground_height_at(xi, yi + 1);
    
    Vec3 normal = {
        hL - hR, // pente Est-Ouest
        hD - hU, // pente Nord-Sud
        2.0f     // poids du Z pour garder une normale "vers le haut"
    };

    // Normalisation
    float mag = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
    normal.x /= mag; normal.y /= mag; normal.z /= mag;
    return normal;
}

void renderDrone(float armLength, float armRadius, float diskRadius) {
    glPushMatrix();

    drawAxes(1.0f);

    GLUquadric *q = gluNewQuadric();

    // Bras X (horizontal)
    glColor3f(0.6f, 0.6f, 0.6f);
    glPushMatrix();
        glTranslatef(-armLength/2, 0.0f, 0.0f);
        glRotatef(90.0f, 0, 1, 0); // orienté dans l'axe X
        gluCylinder(q, armRadius, armRadius, armLength, 16, 1);
    glPopMatrix();

    // Bras Y (profondeur vers avant/arrière)
    glPushMatrix();
        glTranslatef(0.0f, -armLength/2, 0.0f);
        glRotatef(90.0f, -1, 0, 0); // orienté dans l'axe Y
        gluCylinder(q, armRadius, armRadius, armLength, 16, 1);
    glPopMatrix();

    // Disques (hélices) aux extrémités des bras
    glColor3f(1.0f, 0.2f, 0.2f);
    float h = 0.1f; // hauteur au-dessus du bras

    float half = armLength / 2;
    struct { float x, y; } positions[4] = {
        { half, 0.0f }, {-half, 0.0f }, { 0.0f, half }, { 0.0f, -half }
    };

    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
            glTranslatef(positions[i].x, positions[i].y, h);
            glRotatef(0.0f, 1, 0, 0); 
            gluDisk(q, 0.0, diskRadius, 16, 1);
        glPopMatrix();
    }

    gluDeleteQuadric(q);
    glPopMatrix();
}

void draw_velocity(const DroneState* drone) {
    Vec3 v = drone->velocity;
    float scale = 0.2f; // adapter selon longueur désirée
    glColor3f(0.0f, 0.5f, 1.0f);
    glBegin(GL_LINES);
        glVertex3f(0, 0, 0);
        glVertex3f(v.x * scale, v.y * scale, v.z * scale);
    glEnd();
}



void drawText(int x, int y, const char* text, TTF_Font* font, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    SDL_Surface* formatted = SDL_CreateRGBSurface(0, surface->w, surface->h, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);  // RGBA
    SDL_BlitSurface(surface, NULL, formatted, NULL);
    SDL_FreeSurface(surface);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formatted->w, formatted->h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, formatted->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);  // Coord écran

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1, 1, 1);  // blanc
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2i(x, y);
        glTexCoord2f(1, 0); glVertex2i(x + formatted->w, y);
        glTexCoord2f(1, 1); glVertex2i(x + formatted->w, y + formatted->h);
        glTexCoord2f(0, 1); glVertex2i(x, y + formatted->h);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    SDL_FreeSurface(formatted);
    glDeleteTextures(1, &texture);
}



int menu() {
    if (TTF_Init() < 0) {
        fprintf(stderr, "Erreur TTF_Init : %s\n", TTF_GetError());
        return 0;
    }

    TTF_Font* font = TTF_OpenFont("./arial.ttf", 24);
    if (!font) {
        fprintf(stderr, "Erreur TTF_OpenFont : %s\n", TTF_GetError());
        TTF_Quit();
        return 0;
    }

    SDL_Color white = {255, 255, 255, 255};
    int choix = 0;
    int running = 1;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                exit(0);
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_1: choix = 1; running = 0; break;
                    case SDLK_2: choix = 2; running = 0; break;
                    case SDLK_3: exit(0); break;
                    case SDLK_4: {
                        cameraState cam = get_current_camera_position();
                        Vec3 pos = { cam.x, cam.y, cam.z };
                        save_player_state(pos);
                        break;
                    }
                    case SDLK_5: {
                        Vec3 pos = load_player_state();
                        if (mode_fps)
                            cam_pos = pos;
                        else
                            drone.position = pos;
                        updateChunks();
                        break;
                    }
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawText(100, 100, "=== MENU ===", font, white);
        drawText(100, 140, "1. Générer un nouveau monde", font, white);
        drawText(100, 180, "2. Charger monde existant", font, white);
        drawText(100, 220, "3. Quitter", font, white);
        drawText(100, 260, "4. Sauvegarder position joueur", font, white);
        drawText(100, 300, "5. Recharger position sauvegardée", font, white);
        SDL_GL_SwapBuffers();
        SDL_Delay(10);
    }

    if (!c)
        c = malloc(sizeof(chunk));

    if (choix == 1) {
        generate_chunk(c, 50, 50);
        save_chunk(c);
        generate_forest();
        generate_rock();
        generate_clouds();
    } else if (choix == 2) {
        if (!load_chunk(c, 50, 50)) {
            generate_chunk(c, 50, 50);
            save_chunk(c);
            generate_forest();
            generate_rock();
            generate_clouds();
        }
    }

    updateChunks();  // Génère chunks autour de la position actuelle

    float cx = CHUNK_SIZE;
    float cy = CHUNK_SIZE;
    float cz = get_ground_height_at(cx, cy) + 2.5f;

    if (mode_fps) {
        cam_pos.x = cx;
        cam_pos.y = cy;
        cam_pos.z = cz;
        cam_yaw = 0.0f;
        cam_pitch = 0.0f;
    } else {
        drone.position.x = cx;
        drone.position.y = cy;
        drone.position.z = cz;
    }

    TTF_CloseFont(font);
    TTF_Quit();

    return (choix == 1 || choix == 2);
}


void update_camera_fps(const Uint8* keystate, double deltaTime) {
    float speed = 200.0f * deltaTime;
    Vec3 dir = {
        cosf(cam_pitch) * cosf(cam_yaw),
        cosf(cam_pitch) * sinf(cam_yaw),
        sinf(cam_pitch)
    };

    Vec3 right = {
        -sinf(cam_yaw),
        cosf(cam_yaw),
        0.0f
    };

    Vec3 up = {0.0f, 0.0f, 1.0f};

    if (keystate[SDLK_z]) { cam_pos.x += dir.x * speed; cam_pos.y += dir.y * speed; cam_pos.z += dir.z * speed; }
    if (keystate[SDLK_s]) { cam_pos.x -= dir.x * speed; cam_pos.y -= dir.y * speed; cam_pos.z -= dir.z * speed; }
    if (keystate[SDLK_q]) { cam_pos.x -= right.x * speed; cam_pos.y -= right.y * speed; }
    if (keystate[SDLK_d]) { cam_pos.x += right.x * speed; cam_pos.y += right.y * speed; }
    if (keystate[SDLK_SPACE]) { cam_pos.z += speed; }
    if (keystate[SDLK_LCTRL]) { cam_pos.z -= speed; }
    if (keystate[SDLK_a]) cam_yaw -= 40.f * deltaTime;
    if (keystate[SDLK_e]) cam_yaw += 40.f * deltaTime;
    float ground = get_ground_height_at(cam_pos.x, cam_pos.y);
    if (cam_pos.z < ground + 2.0f) cam_pos.z = ground + 2.0f;
}

cameraState get_current_camera_position(void) {
    cameraState cam;
    if (mode_fps) {
        cam.x = cam_pos.x;
        cam.y = cam_pos.y;
        cam.z = cam_pos.z;
    } else {
        cam.x = drone.position.x;
        cam.y = drone.position.y;
        cam.z = drone.position.z;
    }
    return cam;
}

void affichage(const DroneState* drone, float camRadius, float camPitch, float camYaw) {
    glViewport(0, 0, WIDTH, HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70.0, (double)WIDTH / HEIGHT, 0.1, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float time = SDL_GetTicks();
    cameraState cam = get_current_camera_position();

    if (mode_fps) {
        Vec3 dir = {
            cosf(cam_pitch) * cosf(cam_yaw),
            cosf(cam_pitch) * sinf(cam_yaw),
            sinf(cam_pitch)
        };
        Vec3 up = {0.0f, 0.0f, 1.0f};

        gluLookAt(
            cam_pos.x, cam_pos.y, cam_pos.z,
            cam_pos.x + dir.x, cam_pos.y + dir.y, cam_pos.z + dir.z,
            up.x, up.y, up.z
        );
    } else if (drone != NULL) {
        float eyeX = drone->position.x + camRadius * cosf(camPitch) * cosf(camYaw);
        float eyeY = drone->position.y + camRadius * cosf(camPitch) * sinf(camYaw);
        float eyeZ = drone->position.z + camRadius * sinf(camPitch);
        Vec3 up = get_normal_at(drone->position.x, drone->position.y);

        gluLookAt(
            eyeX, eyeY, eyeZ,
            drone->position.x, drone->position.y, drone->position.z,
            up.x, up.y, up.z
        );
    }

    draw_all_chunks();
    draw_forest();
    draw_rock();
    draw_water(0.3f, time);
    draw_clouds(time);

    if (!mode_fps && drone != NULL) {
        glPushMatrix();
        glTranslated(drone->position.x, drone->position.y, drone->position.z);

        Quaternion q = drone->orientation;
        double mat[16] = {
            1 - 2*q.y*q.y - 2*q.z*q.z, 2*q.x*q.y - 2*q.z*q.w,     2*q.x*q.z + 2*q.y*q.w,     0,
            2*q.x*q.y + 2*q.z*q.w,     1 - 2*q.x*q.x - 2*q.z*q.z, 2*q.y*q.z - 2*q.x*q.w,     0,
            2*q.x*q.z - 2*q.y*q.w,     2*q.y*q.z + 2*q.x*q.w,     1 - 2*q.x*q.x - 2*q.y*q.y, 0,
            0,                         0,                         0,                         1
        };
        glMultMatrixd(mat);
        draw_velocity(drone); 
        glColor3f(1.0f, 0.0f, 0.0f);
        renderDrone(2.0f, 0.05f, 0.2f); 
        glPopMatrix();
    }

    SDL_GL_SwapBuffers();
}

