#ifndef PARAMS_HPP
#define PARAMS_HPP

#define RENDER_THREAD_SLEEP_MS 0//6
#define INFO_THREAD_SLEEP_MS   50
#define MESH_THREAD_SLEEP_MS   10
#define MAIN_THREAD_SLEEP_MS   20
#define PHYSICS_TIMESTEP_MS    10
#define BLOCK_TIMESTEP_MS      50

#define PLAYER_FOV (M_PI/3.0)
#define PLAYER_ASPECT 1.0
#define PLAYER_Z_NEAR 0.2
#define PLAYER_Z_FAR 1000.0

#define PLAYER_EYE Vector3f{0, 1, 0}
#define PLAYER_UP Vector3f{0, 0, 1}
#define PLAYER_EYE_HEIGHT 1.6f
#define PLAYER_SIZE Vector3f{0.7, 0.7, 1.9}

#define PLAYER_SPEED (80.0 * (20.0f / PHYSICS_TIMESTEP_MS))
#define GRAVITY 10000.0f
#define PLAYER_DRAG 0.97f

#endif // PARAMS_HPP
