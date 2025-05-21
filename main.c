#define SDL_MAIN_HANDLED
#include <time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "drone.h"

#include "./map/map_perlin.h"
int mode_fps = 0;
DroneState drone;
int main(int argc, char *argv[]) {

    SDL_WM_SetCaption("Simulation Drone", NULL);

    // Paramètres de base OpenGL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(WIDTH, HEIGHT, 32, SDL_OPENGL);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.2f, 0.4f, 1.0f, 1.f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat light_pos[] = { 0.0f, 100.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

    // Génération de la map (appelé une seule fois)
    srand(time(NULL));
    if (!menu()) {
    fprintf(stderr, "Erreur lors du chargement/génération du chunk. Quitte.\n");
    return EXIT_FAILURE;
    }
    printf("ok avant perlin \n");
    init_perlin();
    printf("ok après perlin \n");
    // État initial du drone
    memset(&drone, 0, sizeof(drone));
    printf("ok après memset \n");
    drone.orientation.w = 10.;
    drone.position.x = CHUNK_SIZE / 2;;  // ou un autre point central
    drone.position.y = CHUNK_SIZE / 2;;
    printf("[main] Drone pos avant get_height: %.2f %.2f\n", drone.position.x, drone.position.y);
    drone.position.z = get_ground_height_at(drone.position.x, drone.position.y) + 2 ;

    printf("[main] Drone pos près get_height: %.2f %.2f %.2f\n", drone.position.x, drone.position.y, drone.position.z);
    // Vitesses initiales des rotors (rad/s)
    // double rotorSpeeds[4] = {400.0, 400.0, 400.0, 400.0};
    double rotorSpeeds[4] = {0.0, 0.0, 0.0, 0.0};

    // Pas de temps fixe pour la simulation
    const double deltaTime = 5e-3; // 0.05 s
    int mouseRightDown = 0;
    float camYaw = 0.0f;    // angle horizontal
    float camPitch = 0.3f;  // angle vertical (0 = horizontal, pi/2 = au-dessus)
    float camRadius = 10.0f;
    int lastMouseX = 0, lastMouseY = 0;

    // Contrôle du throttle
    double throttle = 0.0;         // valeur initiale
    const double throttleStep = 10.0;  // incrément par appui
    const double throttleMax = 1000.0; // borne supérieure
    double pitchInput     = 200.0;        // décalage de gaz pour le pitch
    const double pitchStep =  100.0;     // pas de modification par appui
    const double pitchMax  = throttleMax; // on ne dépasse pas la même borne
    // Contrôle du roll
    double rollInput   = 0.0;
    const double rollStep      = 100.0;    // ajuste si tu veux un contrôle plus fin
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
        const Uint8* keystates = SDL_GetKeyState(NULL);
        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
    
                case SDL_QUIT:
                    running = 0;
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    mouseRightDown = 1;
                    lastMouseX = event.button.x;
                    lastMouseY = event.button.y;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    mouseRightDown = 0;
                }
                break;

            case SDL_MOUSEMOTION:
                if (mouseRightDown) {
                    int dx = event.motion.x - lastMouseX;
                    int dy = event.motion.y - lastMouseY;

                    camYaw   += dx * 0.005f;               // vitesse de rotation horizontale
                    camPitch -= dy * 0.005f;               // vitesse de rotation verticale

                    // Clamp pitch entre [0.1, PI/2]
                    if (camPitch < 0.1f) camPitch = 0.1f;
                    if (camPitch > 1.5f) camPitch = 1.5f;

                    lastMouseX = event.motion.x;
                    lastMouseY = event.motion.y;
                }
                break;

                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_l:
                            mode_fps = !mode_fps;
                            SDL_ShowCursor(mode_fps ? SDL_DISABLE : SDL_ENABLE);
                            break;
                        case SDLK_ESCAPE:
                            running = 0;
                            break;
                        case SDLK_SPACE:
                            throttle += throttleStep;
                            if (throttle > throttleMax) throttle = throttleMax;
                            break;
                        case SDLK_LCTRL:
                            throttle -= throttleStep;
                            if (throttle < 0.0) throttle = 0.0;
                            break;
                        case SDLK_r:
                            // RESET LA POSITION DU DRONE DANS LA SIMU
                            drone.position = (Vec3){0, 0, 0};
                            drone.velocity = (Vec3){0, 0, 0};
                            drone.orientation = (Quaternion){1, 0, 0, 0};
                            drone.angularVelocity = (Vec3){0, 0, 0};
                            throttle = 0.0;
                            pitchInput = 0.0;
                            rollInput = 0.0;
                            yawInput = 0.0;
                            break;

                        // ZQSD pour contrôle du pitch et roll
                        case SDLK_z: // Avancer (pitch avant)
                            pitchInput += pitchStep;
                            if (pitchInput > pitchMax) pitchInput = pitchMax;
                            break;
                        case SDLK_s: // Reculer (pitch arrière)
                            pitchInput -= pitchStep;
                            if (pitchInput < -pitchMax) pitchInput = -pitchMax;
                            break;
                        case SDLK_q: // Roulis gauche
                            rollInput -= rollStep;
                            if (rollInput < -rollMax) rollInput = -rollMax;
                            break;
                        case SDLK_d: // Roulis droite
                            rollInput += rollStep;
                            if (rollInput > rollMax) rollInput = rollMax;
                            break;

                        // Optionnel : Yaw avec A/E ?
                        case SDLK_a: // Torsion gauche
                            yawInput -= yawStep;
                            if (yawInput < -yawMax) yawInput = -yawMax;
                            break;
                        case SDLK_e: // Torsion droite
                            yawInput += yawStep;
                            if (yawInput > yawMax) yawInput = yawMax;
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

        if (mode_fps){
            update_camera_fps(keystates, deltaTime);
            affichage(NULL, 0, 0, 0); // drone inutile ici
        }
        else{
            updateDrone(&drone, rotorSpeeds, deltaTime);
            affichage(&drone, camRadius, camPitch, camYaw);
        }
        updateChunks();
        // printf("Position: %.2f %.2f %.2f | Vitesse: %.2f %.2f %.2f\n",
        // drone.position.x, drone.position.y, drone.position.z,
        // drone.velocity.x, drone.velocity.y, drone.velocity.z);
        // Limiteur de boucle
        SDL_Delay(5);
    }

    SDL_Quit();
    return EXIT_SUCCESS;
}
