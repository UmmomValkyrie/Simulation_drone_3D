#ifndef DRONE_H
#define DRONE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----- Type Definitions -----
typedef struct { double x, y, z; } Vec3;
typedef struct { double w, x, y, z; } Quaternion;

typedef struct {
    Vec3 position;         // Position in world frame (meters)
    Vec3 velocity;         // Linear velocity in world frame (m/s)
    Quaternion orientation; // Orientation as a unit quaternion
    Vec3 angularVelocity;  // Angular velocity in body frame (rad/s)
} DroneState;

// ----- Physical Constants -----
// These can be tuned in the .c file or overridden before compilation
#ifndef DRONE_MASS
#define DRONE_MASS 1.5            // Mass in kg
#endif

#ifndef DRONE_ARM_LENGTH
#define DRONE_ARM_LENGTH 0.25     // Distance from center to rotor in meters
#endif

#ifndef DRONE_I_XX
#define DRONE_I_XX 0.02           // Inertia around X-axis (kg·m^2)
#endif

#ifndef DRONE_I_YY
#define DRONE_I_YY 0.02           // Inertia around Y-axis (kg·m^2)
#endif

#ifndef DRONE_I_ZZ
#define DRONE_I_ZZ 0.04           // Inertia around Z-axis (kg·m^2)
#endif

#ifndef DRONE_K_THRUST
#define DRONE_K_THRUST 5e-6       // Thrust coefficient N/(rad/s)^2
#endif

#ifndef DRONE_K_DRAG
#define DRONE_K_DRAG 1e-7         // Drag torque coefficient N·m/(rad/s)^2
#endif

#ifndef DRONE_GRAVITY
#define DRONE_GRAVITY {0.0, 0.0, -9.81} // Gravity vector (m/s^2)
#endif

// ----- Function Prototypes -----

/**
 * @brief Normalize a quaternion in place to unit length.
 * @param q Pointer to quaternion to normalize.
 */
void quat_normalize(Quaternion* q);

/**
 * @brief Rotate a vector from body frame to world frame using a quaternion.
 * @param q Unit quaternion representing body-to-world orientation.
 * @param v Vector in the body frame.
 * @return Rotated vector in the world frame.
 */
Vec3 rotate_body_to_world(Quaternion q, Vec3 v);

/**
 * @brief Update the drone state given rotor angular speeds.
 *
 * @param state Pointer to the current drone state, updated in place.
 * @param rotorSpeeds Array of 4 rotor angular speeds (rad/s).
 * @param dt Time step for the update (s).
 */
void updateDrone(DroneState* state, const double rotorSpeeds[4], double dt);

#ifdef __cplusplus
}
#endif

#endif // DRONE_H
