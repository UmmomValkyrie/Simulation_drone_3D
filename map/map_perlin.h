#ifndef MAP_PERLIN_H
#define MAP_PERLIN_H

#include "../drone.h" // pour DroneState
#include <SDL/SDL.h> 

#define WIDTH 900
#define HEIGHT 900
#define RENDER_DISTANCE 2
#define CHUNK_SIZE 64
#define MAP_WIDTH 200
#define MAP_HEIGHT 200
#define SCALE 0.04f
#define AMPLITUDE 20.0f
#define NB_TREES 100
#define NB_ROCKS 1000
#define NB_CLOUDS 1000
#define MOUNTAIN_LEVEL 0.6f
#define WATER_LEVEL 0.4f
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

typedef struct chunk chunk;
typedef struct Node Node;
typedef struct tree tree;
typedef struct rocks rocks;
typedef struct clouds clouds;
typedef struct cameraState cameraState;

struct cameraState {
    float x, y, z;
};

struct chunk{
    int chunkX;
    int chunkY;
    float heightmap[CHUNK_SIZE+1][CHUNK_SIZE+1];
};

struct Node{
    int x, y;
    struct Node* left;
    struct Node* right;
};
struct tree {
    int x, y;
    float height_tree;

};

struct rocks {
    int x, y;
    float r;
};
struct clouds{
    int x, y;
    float r;
};

const tree* get_forest();
const rocks* get_rocks();


cameraState get_current_camera_position(void);
void update_camera_fps(const Uint8* keys, double dt);
void init_SDL(void);
void affichage(const DroneState* drone, float camRadius, float camPitch, float camYaw);
float get_ground_height_at(float x, float y);
int menu();
void init_perlin();
void generate_chunk(chunk* chunk, int cx, int cy);
void updateChunks(void);
int load_chunk(chunk* chunk, int cx, int cy);
void save_chunk(chunk* chunk);
void generate_heightmap();
void generate_forest();
void generate_rock();
void generate_clouds();


#endif

