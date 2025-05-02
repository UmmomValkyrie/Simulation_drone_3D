#ifndef WINDOW_H
#define WINDOW_H

#include <SDL/SDL.h>
#include "drone.h"

// VARIABLES
extern const int hauteur;
extern const int largeur;
extern const int terrain;
extern const char * titre;

extern int camera_x, camera_y;

extern SDL_Surface * ecran;

// FONCTIONS
double world_to_camera_x(double coord);
double world_to_camera_y(double coord);
void affichage(SDL_Surface * ecran, const DroneState *state); 
void init_SDL();


#endif // WINDOW_H