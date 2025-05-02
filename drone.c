#include <math.h>
#include <string.h>

// ----- Types -----
typedef struct { double x, y, z; } Vec3;
typedef struct { double w, x, y, z; } Quaternion;

typedef struct {
    Vec3 position;
    Vec3 velocity;
    Quaternion orientation;
    Vec3 angularVelocity;
} DroneState;

// ----- Physical constants -----
const double MASS = 1.5;           // kg
typedef struct { double xx, yy, zz; } Inertia;
const Inertia I = {0.02, 0.02, 0.04}; // kg·m² (approximate diagonal inertia)
const double ARM_LENGTH = 0.25;    // m (distance from center to rotor)
const double K_THRUST = 5e-6;      // N/(rad/s)^2
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

static inline void quat_normalize(Quaternion* q) {
    double n = sqrt(q->w*q->w + q->x*q->x + q->y*q->y + q->z*q->z);
    q->w /= n; q->x /= n; q->y /= n; q->z /= n;
}

static Vec3 rotate_body_to_world(Quaternion q, Vec3 v) {
    // v' = q * [0,v] * q*
    Quaternion p = {0, v.x, v.y, v.z};
    Quaternion t = quat_mul(q, p);
    Quaternion res = quat_mul(t, quat_conjugate(q));
    return (Vec3){res.x, res.y, res.z};
}

// ----- Update function -----
void updateDrone(DroneState* state, const double rotorSpeeds[4], double dt) {
    // 1. Compute individual thrusts and drag torques
    double thrusts[4];
    for (int i = 0; i < 4; ++i) {
        thrusts[i] = K_THRUST * rotorSpeeds[i] * rotorSpeeds[i];
    }
    // net force in body frame
    double Fz = thrusts[0] + thrusts[1] + thrusts[2] + thrusts[3];
    Vec3 force_body = {0.0, 0.0, Fz};

    // torques in body frame
    // rotor layout: 0 front-left, 1 front-right, 2 rear-right, 3 rear-left
    double tau_x = ARM_LENGTH * (thrusts[1] + thrusts[2] - thrusts[0] - thrusts[3]); // roll
    double tau_y = ARM_LENGTH * (thrusts[0] + thrusts[1] - thrusts[2] - thrusts[3]); // pitch
    double tau_z = K_DRAG * (rotorSpeeds[0] - rotorSpeeds[1] + rotorSpeeds[2] - rotorSpeeds[3]); // yaw
    Vec3 torque = {tau_x, tau_y, tau_z};

    // 2. Convert force to world frame
    Vec3 force_world = rotate_body_to_world(state->orientation, force_body);
    // add gravity
    Vec3 accel = vec3_scale(force_world, 1.0 / MASS);
    accel = vec3_add(accel, GRAVITY);

    // 3. Integrate linear motion
    state->velocity = vec3_add(state->velocity, vec3_scale(accel, dt));
    state->position = vec3_add(state->position, vec3_scale(state->velocity, dt));

    // 4. Integrate angular motion
    // angular acceleration: I^{-1}(torque - omega x I*omega)
    Vec3 Iomega = {I.xx*state->angularVelocity.x,
                   I.yy*state->angularVelocity.y,
                   I.zz*state->angularVelocity.z};
    Vec3 omegaCrossIomega = vec3_cross(state->angularVelocity, Iomega);
    Vec3 angAccel = {(torque.x - omegaCrossIomega.x) / I.xx,
                     (torque.y - omegaCrossIomega.y) / I.yy,
                     (torque.z - omegaCrossIomega.z) / I.zz};
    state->angularVelocity = vec3_add(state->angularVelocity, vec3_scale(angAccel, dt));

    // 5. Update orientation quaternion
    Quaternion omega_q = {0, state->angularVelocity.x, state->angularVelocity.y, state->angularVelocity.z};
    Quaternion q_dot = quat_mul(state->orientation, omega_q);
    q_dot.w *= 0.5; q_dot.x *= 0.5; q_dot.y *= 0.5; q_dot.z *= 0.5;
    state->orientation.w += q_dot.w * dt;
    state->orientation.x += q_dot.x * dt;
    state->orientation.y += q_dot.y * dt;
    state->orientation.z += q_dot.z * dt;
    quat_normalize(&state->orientation);

    if (state->position.z <= 0.0) {
        state->position.z = 0.0; }        // on replace sur le sol
        if (state->velocity.z < 0.0) {
            state->velocity.z = 0.0;     // on stoppe la chute
        }
}