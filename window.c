#include "window.h"
#include "drone.h"
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>

const int largeur = 800;
const int hauteur = 700;
const int terrain = 500;
const char * titre = "Survivor";

int camera_x = 0;
int camera_y = 0;


SDL_Surface * ecran = NULL;
//Fonctions camera
double world_to_camera_x(double coord) {
    return (coord - camera_x);
}

double world_to_camera_y(double coord) {
    return (coord - camera_y); 
}



void drawArm(float length, float radius) {
    GLUquadric *q = gluNewQuadric();
    gluCylinder(q, radius, radius, length, 16, 1);
    gluDeleteQuadric(q);
}

void drawCube(float size) {
    float h = size / 2;
    glBegin(GL_QUADS);
      // Face avant
      glVertex3f(-h, -h,  h);
      glVertex3f( h, -h,  h);
      glVertex3f( h,  h,  h);
      glVertex3f(-h,  h,  h);
      // … répéter pour les 5 autres faces …
    glEnd();
}

void drawGround(float size, float y)
{
    // Désactiver le test de face si vous voulez voir le plan des deux côtés
    glDisable(GL_CULL_FACE);

    // Définir la couleur verte pour le sol
    glColor3f(0.0f, 0.6f, 0.0f);

    // On pourrait aussi activer un matériau si vous avez l'éclairage en marche :
    // GLfloat mat_diffuse[] = {0.0f, 0.6f, 0.0f, 1.0f};
    // glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);

    // Dessiner un grand carré centré en (0,y,0)
    float half = size * 0.5f;
    glBegin(GL_QUADS);
      // Normale vers le haut pour l'éclairage
      glNormal3f(0.0f, 1.0f, 0.0f);
      glVertex3f(-half, y, -half);
      glVertex3f( half, y, -half);
      glVertex3f( half, y,  half);
      glVertex3f(-half, y,  half);
    glEnd();

    // Si vous voulez du “grillage” pour mieux visualiser l’échelle :
    glColor3f(0.0f, 0.5f, 0.0f);
    glBegin(GL_LINES);
      for (float i = -half; i <= half; i += size/100) {
        // lignes parallèles X
        glVertex3f(-half, y, i);
        glVertex3f( half, y, i);
        // lignes parallèles Z
        glVertex3f(i, y, -half);
        glVertex3f(i, y,  half);
      }
    glEnd();

    // Remettre la face culling si vous l'utilisez pour le reste
    glEnable(GL_CULL_FACE);
}

static void drawAxes(float len) {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
      // X-axis (rouge)
      glColor3f(1.0f, 0.0f, 0.0f);
      glVertex3f(0, 0, 0);
      glVertex3f(len, 0, 0);
      // Y-axis (vert)
      glColor3f(0.0f, 1.0f, 0.0f);
      glVertex3f(0, 0, 0);
      glVertex3f(0, len, 0);
      // Z-axis (bleu)
      glColor3f(0.0f, 0.0f, 1.0f);
      glVertex3f(0, 0, 0);
      glVertex3f(0, 0, len);
    glEnd();
    // remets la couleur par défaut
    glColor3f(1.0f, 1.0f, 1.0f);
}


void renderDrone(float armLength, float armRadius, float diskRadius) {
    glPushMatrix();

      // 2) On oriente le drone :  
      glRotatef( 45.0f, 0.0f, 1.0f, 0.0f);

      // 3) Ici votre code d’origine, inchangé :
      GLUquadric *q = gluNewQuadric();

      // Bras sur l’axe X
      glColor3f(0.6f, 0.6f, 0.6f);
      glPushMatrix();
        glTranslatef(-armLength/2, 0.0f, 0.0f);
        glRotatef(90.0f, 0, 1, 0);
        gluCylinder(q, armRadius, armRadius, armLength, 16, 1);
      glPopMatrix();

      // Bras sur l’axe Z
      glPushMatrix();
        glTranslatef(0.0f, 0.0f, -armLength/2);
        glRotatef(90.0f, 0, 0, 1);
        gluCylinder(q, armRadius, armRadius, armLength, 16, 1);
      glPopMatrix();

      // 4 disques aux extrémités
      glColor3f(1.0f, 0.2f, 0.2f);
      const float half = armLength/2;
      for (int i = 0; i < 4; ++i) {
          float x = (i==0 ?  half : (i==1 ? -half : 0.0f));
          float z = (i>=2 ? (i==2?  half : -half) : 0.0f);

          glPushMatrix();
            glTranslatef(x, 0.05f, z);
            glRotatef(-90.0f, 1, 0, 0);
            gluDisk(q, 0.0, diskRadius, 16, 1);
          glPopMatrix();
      }

      gluDeleteQuadric(q);

    // 4) On restaure la matrice d’origine
    glPopMatrix();
}



void affichage(SDL_Surface * ecran, const DroneState *state) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    // Positionner la caméra
    // gluLookAt(3,2,3, 0,0,0, 0,1,0);
    float eyeX    = state->position.x;
    float eyeY    = state->position.z + 2.0f;  // 2 m au-dessus
    float eyeZ    = state->position.y - 5.0f;  // 5 m derrière
    float centerX = state->position.x;
    float centerY = state->position.z;         // on regarde au niveau du drone
    float centerZ = state->position.y;
    gluLookAt(
      eyeX,    eyeY,    eyeZ,
      centerX, centerY, centerZ,
      0.0f,    1.0f,    0.0f   // vecteur “up” toujours dans l’axe Y_display
    );
    

    drawGround(40.0f, -0.01f);

    glPushMatrix();
    // glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
    glTranslatef(state->position.x, state->position.z, state->position.y);

    {
        float w = (float)state->orientation.w;
        float x = (float)state->orientation.x;
        float y = (float)state->orientation.y;
        float z = (float)state->orientation.z;
        float angle = 2.0f * acosf(w) * 180.0f / M_PI;
        float s = sqrtf(1.0f - w*w);
        if (s < 0.001f) s = 1.0f;
        glRotatef(angle, x/s, y/s, z/s);
    }
    drawAxes(1.0f);
    renderDrone(2.0f, 0.05f, 0.2f);
    glPopMatrix();

    SDL_GL_SwapBuffers();

    //FLIP
    SDL_Flip(ecran);
}

void init_SDL() {
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error in SDL_Init : %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    if((ecran = SDL_SetVideoMode(largeur, hauteur, 32, SDL_HWSURFACE | SDL_DOUBLEBUF)) == NULL) {
        fprintf(stderr, "Error in SDL_SetVideoMode : %s\n", SDL_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE); 
    }
    SDL_WM_SetCaption(titre, NULL);
}