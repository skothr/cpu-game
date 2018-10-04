#ifndef CHUNK_STATES_HPP
#define CHUNK_STATES_HPP

enum ChunkState
  {
   UNLOADED = 0,  // chunk not loaded
   LOADING,       // chunk being loaded
   LOADED,        // chunk loaded but not its neighbors
   READY,         // chunk neighbors loaded
   MESHING,       // chunk being meshed
   MESHED,        // chunk is meshed
   EMPTY,         // chunk has no blocks
   VISIBLE,       // chunk is visible (being rendered)

   COUNT
  };

inline Vector3f chunkStateColor(ChunkState state)
{
  switch(state)
    {
    case ChunkState::UNLOADED:
      return Vector3f{0.12,0.12,0.12};
    case ChunkState::LOADING:
      return Vector3f{0.2,0.2,0};
    case ChunkState::LOADED:
      return Vector3f{0,0.15,0.15};
    case ChunkState::READY:
      return Vector3f{0,0.25,0.25};
    case ChunkState::MESHING:
      return Vector3f{0,0.5,0};
    case ChunkState::MESHED:
      return Vector3f{0,1,0};
    case ChunkState::EMPTY:
      return Vector3f{0.6,0.6,0.6};
    case ChunkState::VISIBLE:
      return Vector3f{1,1,1};
    default:
      return Vector3f{1,0,1};
    }
}


#endif // CHUNK_STATES_HPP
