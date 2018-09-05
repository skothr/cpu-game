#ifndef PARAMS_HPP
#define PARAMS_HPP


#define RENDER_THREAD_SLEEP_MS 0
#define INFO_THREAD_SLEEP_MS   50
#define MESH_THREAD_SLEEP_MS   10
#define MAIN_THREAD_SLEEP_MS   20
#define PHYSICS_TIMESTEP_MS    10
#define BLOCK_TIMESTEP_MS      50

#define PLAYER_FOV 45
#define PLAYER_ASPECT 1.0
#define PLAYER_Z_NEAR 0.2
#define PLAYER_Z_FAR 512.0

#define START_CHUNK Point3i({0, 0, 0})
#define CHUNK_RAD Vector3i({8, 8, 2})
#define START_POS Point3f({START_CHUNK[0]*Chunk::sizeX + Chunk::sizeX/2, \
                           START_CHUNK[1]*Chunk::sizeY + Chunk::sizeY/2, \
                           START_CHUNK[2]*Chunk::sizeZ + 8 })

#define PLAYER_EYE Vector3f{0, 1, 0}
#define PLAYER_UP Vector3f{0, 0, 1}
#define PLAYER_EYE_HEIGHT 1.6f
#define PLAYER_SIZE Vector3f{0.9, 0.9, 1.9}

#define PLAYER_SPEED (80.0 * (20.0f / PHYSICS_TIMESTEP_MS))
#define GRAVITY 25000.0f
#define PLAYER_DRAG 0.98f

#endif // PARAMS_HPP
