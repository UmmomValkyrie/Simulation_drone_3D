#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "./map/map_perlin.h"
#include "drone.h"

// ----- Types -----
// typedef struct { double x, y, z; } Vec3;
// typedef struct { double w, x, y, z; } Quaternion;

// typedef struct {
//     Vec3 position;
//     Vec3 velocity;
//     Quaternion orientation;
//     Vec3 angularVelocity;
// } DroneState;

// ----- Physical constants -----
const double MASS = 1.5;           // kg
typedef struct { double xx, yy, zz; } Inertia;
const Inertia I = {0.02, 0.02, 0.04}; // kg·m² (approximate diagonal inertia)
const double ARM_LENGTH = 0.25;    // m (distance from center to rotor)
const double K_THRUST = 3e-5;      // N/(rad/s)^2
const double K_DRAG = 1e-7;        // N·m/(rad/s)^2
const Vec3 GRAVITY = {0, 0, -9.81};

// ----- Vector ops -----
static inline Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline Vec3 vec3_scale(Vec3 v, double s) { return (Vec3){v.x*s, v.y*s, v.z*s}; }
static inline double vec3_dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}

// ----- Quaternion ops -----
static inline Quaternion quat_mul(Quaternion a, Quaternion b) {
    return (Quaternion){
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w
    };
}

static inline Quaternion quat_conjugate(Quaternion q) {
    return (Quaternion){q.w, -q.x, -q.y, -q.z};
}

void quat_normalize(Quaternion* q) {
    double n = sqrt(q->w*q->w + q->x*q->x + q->y*q->y + q->z*q->z);
    q->w /= n; q->x /= n; q->y /= n; q->z /= n;
}

Vec3 rotate_body_to_world(Quaternion q, Vec3 v) {
    // v' = q * [0,v] * q*
    Quaternion p = {0, v.x, v.y, v.z};
    Quaternion t = quat_mul(q, p);
    Quaternion res = quat_mul(t, quat_conjugate(q));
    return (Vec3){res.x, res.y, res.z};
}
// Ajout Par pierre permet de déterminer la position des hélices dans le monde pour affecté la collision par la suite, l'idée principale et de déterminer la position x, y, z de l'hélice et d'appliquer une répulsion dessus
// Cela gére égalament la colision avec les arbres et rochers

Vec3 get_rotor_world_position(const DroneState* drone, float offsetX, float offsetY) {
    Vec3 local = { offsetX, offsetY, 0.0f };
    Quaternion q = drone->orientation;

    Vec3 result;
    result.x = drone->position.x + (q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z) * local.x
                                 + 2*(q.x*q.y - q.w*q.z) * local.y;
    result.y = drone->position.y + 2*(q.x*q.y + q.w*q.z) * local.x
                                 + (q.w*q.w - q.x*q.x + q.y*q.y - q.z*q.z) * local.y;
    result.z = drone->position.z;
    return result;
}

bool check_collision_with_obstacle(Vec3 pos, Vec3 obstacle, float radius) {
    float dx = pos.x - obstacle.x;
    float dy = pos.y - obstacle.y;
    float dist2 = dx * dx + dy * dy;
    return dist2 < (radius * radius);
}

void apply_rotor_collision(DroneState* state) {
    const tree* trees = get_forest();
    const rocks* rocks = get_rocks();
    Vec3 rotors[4] = {
        get_rotor_world_position(state,  1.0f,  0.0f),
        get_rotor_world_position(state, -1.0f,  0.0f),
        get_rotor_world_position(state,  0.0f,  1.0f),
        get_rotor_world_position(state,  0.0f, -1.0f)
    };
    for (int i = 0; i < 4; i++) {
        float g = get_ground_height_at(rotors[i].x, rotors[i].y);
        if (rotors[i].z <= g + 0.05f) {
            state->velocity.z = 0;
            state->position.z = g + 0.1f;
        }

        for (int j = 0; j < NB_TREES; j++) {
            Vec3 obs = { trees[j].x + 0.5f, trees[j].y + 0.5f, 0.0f };
            if (check_collision_with_obstacle(rotors[i], obs, 0.5f)) {
                state->velocity.z = 0;
            }
        }
        for (int j = 0; j < NB_ROCKS; j++) {
            Vec3 obs = { rocks[j].x + 0.5f, rocks[j].y + 0.5f, 0.0f };
            if (check_collision_with_obstacle(rotors[i], obs, 0.5f)) {
                state->velocity.z = 0;
            }
        }
    }
}

// ----- Update function -----
void updateDrone(DroneState* state, const double rotorSpeeds[4], double dt) {
    // 1. Compute individual thrusts and drag torques
    double thrusts[4];
    for (int i = 0; i < 4; ++i) {
        thrusts[i] = K_THRUST * rotorSpeeds[i] * rotorSpeeds[i];
    }

    Vec3 force_body = {0.0, 0.0, thrusts[0] + thrusts[1] + thrusts[2] + thrusts[3]};

    // Torques (roll, pitch, yaw)
    double tau_x = ARM_LENGTH * (thrusts[1] + thrusts[2] - thrusts[0] - thrusts[3]);
    double tau_y = ARM_LENGTH * (thrusts[0] + thrusts[1] - thrusts[2] - thrusts[3]);
    double tau_z = K_DRAG * (rotorSpeeds[0] - rotorSpeeds[1] + rotorSpeeds[2] - rotorSpeeds[3]);
    Vec3 torque = {tau_x, tau_y, tau_z};

    // 2. Forces → monde
    Vec3 force_world = rotate_body_to_world(state->orientation, force_body);
    Vec3 accel = vec3_add(vec3_scale(force_world, 1.0 / MASS), GRAVITY);

    // 3. Mouvement linéaire
    state->velocity = vec3_add(state->velocity, vec3_scale(accel, dt));
    state->position = vec3_add(state->position, vec3_scale(state->velocity, dt));

    // 4. Mouvement angulaire
    Vec3 Iomega = {I.xx * state->angularVelocity.x, I.yy * state->angularVelocity.y, I.zz * state->angularVelocity.z};
    Vec3 omegaCrossIomega = vec3_cross(state->angularVelocity, Iomega);
    Vec3 angAccel = {
        (torque.x - omegaCrossIomega.x) / I.xx,
        (torque.y - omegaCrossIomega.y) / I.yy,
        (torque.z - omegaCrossIomega.z) / I.zz
    };
    state->angularVelocity = vec3_add(state->angularVelocity, vec3_scale(angAccel, dt));

    // 5. Orientation
    Quaternion omega_q = {0, state->angularVelocity.x, state->angularVelocity.y, state->angularVelocity.z};
    Quaternion q_dot = quat_mul(state->orientation, omega_q);
    q_dot.w *= 0.5; q_dot.x *= 0.5; q_dot.y *= 0.5; q_dot.z *= 0.5;
    state->orientation.w += q_dot.w * dt;
    state->orientation.x += q_dot.x * dt;
    state->orientation.y += q_dot.y * dt;
    state->orientation.z += q_dot.z * dt;
    quat_normalize(&state->orientation);

    // 6. Collision avec le sol
    float ground = get_ground_height_at(state->position.x, state->position.y);
    if (state->position.z <= ground) {
        state->position.z = ground;
        if (state->velocity.z < 0.0)
            state->velocity.z = 0.0;
    }
    // 7. Collision avec les objets bitch 
    apply_rotor_collision(state);
    // Debug
    // printf("Z: %.2f | Vz: %.2f | Ground: %.2f\n", state->position.z, state->velocity.z, ground);
}