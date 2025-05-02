#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "window.h"
#include "drone.h"

int main(int argc, char *argv[]) {
    // Pour la console de debug sur Windows
    #ifdef _WIN32
    AllocConsole();
    freopen("CON", "w", stdout);
    freopen("CON", "w", stderr);
    freopen("CON", "r", stdin);
    #endif

    // Initialisation de SDL et création de la fenêtre
    init_SDL();
    SDL_Surface *ecran = SDL_SetVideoMode(1200, 700, 32, SDL_OPENGL | SDL_DOUBLEBUF);
    if (!ecran) {
        fprintf(stderr, "SDL_SetVideoMode a échoué : %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_WM_SetCaption("Drone 3D", NULL);

    // MANETTE 
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_Joystick *joy = NULL;
    if (SDL_NumJoysticks() > 0) {
        joy = SDL_JoystickOpen(0);
        if (!joy) {
            fprintf(stderr, "Impossible d'ouvrir la manette: %s\n", SDL_GetError());
        } else {
            printf("Manette détectée : %s\n", SDL_JoystickName(0));
        }
    }

    // Configuration d'OpenGL
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 800.0/600.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);

    // État initial du drone
    DroneState drone;
    memset(&drone, 0, sizeof(drone));
    drone.orientation.w = 1.0;
    drone.position.z = 1.0;


    // Vitesses initiales des rotors (rad/s)
    // double rotorSpeeds[4] = {400.0, 400.0, 400.0, 400.0};
    double rotorSpeeds[4] = {0.0, 0.0, 0.0, 0.0};

    // Pas de temps fixe pour la simulation
    const double deltaTime = 5e-3; // 0.05 s

    // Contrôle du throttle
    double throttle = 0.0;         // valeur initiale
    const double throttleStep = 10.0;  // incrément par appui
    const double throttleMax = 1000.0; // borne supérieure
    double pitchInput     = 0.0;        // décalage de gaz pour le pitch
    const double pitchStep =  20.0;     // pas de modification par appui
    const double pitchMax  = throttleMax; // on ne dépasse pas la même borne
    // Contrôle du roll
    double rollInput   = 0.0;
    const double rollStep      = 20.0;    // ajuste si tu veux un contrôle plus fin
    const double rollMax       = throttleMax;

    // Contrôle du yaw
    double yawInput    = 0.0;
    const double yawStep       = 20.0;
    const double yawMax        = throttleMax;

    SDL_EnableKeyRepeat(200, SDL_DEFAULT_REPEAT_INTERVAL);

    int running = 1;
    SDL_Event event;

    // Boucle principale
    while (running) {
        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
    
                case SDL_QUIT:
                    running = 0;
                    break;
    
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            running = 0;
                            break;
                        case SDLK_w:
                            // augmenter les gaz
                            throttle += throttleStep;
                            if (throttle > throttleMax) throttle = throttleMax;
                            break;
                        case SDLK_s:
                            // réduire les gaz (optionnel)
                            throttle -= throttleStep;
                            if (throttle < 0.0) throttle = 0.0;
                            break;
                        case SDLK_r:
                            // RESET LA POSITION DU DRONE DANS LA SIMU
                            drone.position = (Vec3){0,0,0};
                            drone.velocity = (Vec3){0,0,0};
                            drone.orientation = (Quaternion){1,0,0,0};
                            drone.angularVelocity = (Vec3){0,0,0};
                            if (throttle < 0.0) throttle = 0.0;
                            break;
                        case SDLK_UP:   // flèche Haut : penche vers l’avant
                            pitchInput += pitchStep;
                            if (pitchInput > pitchMax) pitchInput = pitchMax;
                            break;
                        case SDLK_DOWN: // flèche Bas : penche vers l’arrière
                            pitchInput -= pitchStep;
                            if (pitchInput < -pitchMax) pitchInput = -pitchMax;
                            break;
                        default:
                            break;
                    }
                    break;
    
                case SDL_JOYAXISMOTION:
                    // event.jaxis.axis = 0:left stick X, 1:left stick Y, 2:right stick X, 3:right stick Y
                    // event.jaxis.value ∈ [-32768,32767]

                        // dead zone
                    const int DEAD = 8000;
                    int v = event.jaxis.value;
                    if (abs(v) < DEAD) v = 0;
                    double norm = v / 32767.0;


                    switch (event.jaxis.axis) {
                        case 0: // stick gauche X → roll
                          rollInput = norm * rollMax;
                          break;
                        case 1: // stick gauche Y → pitch (inversé souvent)
                          pitchInput = -norm * pitchMax;
                          break;
                        case 2: // stick droit X → yaw
                          yawInput = norm * yawMax;
                          break;
                        case 3: // stick droit Y → throttle
                          throttle = ((-norm + 1.0) * 0.5) * throttleMax;
                          break;
                      }
    
                default:
                    break;
            }
        }
        
    

        // Num rotors
        // 0   3
        //   x
        // 2   1


        // Mettre à jour rotorSpeeds selon le throttle global
        // printf("Vitesse des rotors : %lf\n", throttle);
        // for (int i = 0; i < 4; ++i) {
        //     rotorSpeeds[i] = throttle;
        // }


        rotorSpeeds[0] = throttle - pitchInput - rollInput - yawInput;  // front-left
        rotorSpeeds[3] = throttle - pitchInput + rollInput + yawInput;  // front-right
        rotorSpeeds[1] = throttle + pitchInput + rollInput - yawInput;  // rear-right
        rotorSpeeds[2] = throttle + pitchInput - rollInput + yawInput;  // rear-left
        

        //DEBUG 
        printf("%d\n",event.jaxis.value);

        // // On compose l’effort global throttle + pitchInput
        // rotorSpeeds[0] = throttle - pitchInput;  // rotor avant-gauche
        // rotorSpeeds[1] = throttle - pitchInput;  // rotor avant-droit
        // rotorSpeeds[2] = throttle + pitchInput;  // rotor arrière-droit
        // rotorSpeeds[3] = throttle + pitchInput;  // rotor arrière-gauche

        // Mise à jour physique du drone avec pas de temps fixe
        updateDrone(&drone, rotorSpeeds, deltaTime);
        camera_x = (int)drone.position.x;
        camera_y = (int)drone.position.z; // on suit l'altitude comme axe Y dans window.c


        // Rendu de la scène
        affichage(ecran, &drone);

        // Limiteur de boucle
        SDL_Delay(5);
    }

    SDL_Quit();
    return EXIT_SUCCESS;
}
