#include <math.h>
#include <stdio.h>
#include "vecteurs.h"


// Fonction pour normaliser un vecteur
Vector2D normalize(Vector2D v) {
    double magnitude = sqrt(v.x * v.x + v.y * v.y);
    if (magnitude == 0) return (Vector2D){0, 0}; // Éviter la division par zéro
    return (Vector2D){ v.x / magnitude, v.y / magnitude };
}
