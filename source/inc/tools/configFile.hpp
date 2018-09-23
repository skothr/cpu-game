#ifndef CONFIG_FILE_HPP
#define CONFIG_FILE_HPP

#include "world.hpp"
#include <fstream>
#include <sstream>

#define CONFIG_DIR "./config/"
#define CONFIG_FILE "defaults.cfg"
#define CONFIG_PATH CONFIG_DIR CONFIG_FILE

#define DEFAULT_WORLD_NAME "new-world"

static World::Options getDefaultOptions()
{
  World::Options opt{"", terrain_t::PERLIN_WORLD, 0, {0,0,0}, {8,8,4}, 8, 8};
  
  std::ifstream config(CONFIG_PATH);
  std::string line;
  while(std::getline(config, line))
    {
      if(line.size() > 0 && line[0] != '#')
        {
          std::istringstream ss(line);
          std::string option;
          ss >> option;
          if(option == "WORLD_NAME")
            {
              std::string name;
              ss >> name;
              opt.name = name;
            }
          else if(option == "TERRAIN")
            {
              std::string str;
              std::getline(ss, str);
              str.erase(str.begin(), std::find_if(str.begin(), str.end(),
                                                  [](int c)
                                                  { return !std::isspace(c); }));
              terrain_t terrain = terrainFromString(str);
              if(terrain == terrain_t::INVALID)
                { LOGW("Invalid terrain config!"); }
              opt.terrain = (terrain == terrain_t::INVALID ? terrain_t::PERLIN_WORLD : terrain);
            }
          else if(option == "SEED")
            {
              int seed;
              ss >> seed;
              opt.seed = seed;
            }
          else if(option == "CHUNK_RAD_X")
            {
              int rx;
              ss >> rx;
              opt.chunkRadius[0] = rx;
            }
          else if(option == "CHUNK_RAD_Y")
            {
              int ry;
              ss >> ry;
              opt.chunkRadius[1] = ry;
            }
          else if(option == "CHUNK_RAD_Z")
            {
              int rz;
              ss >> rz;
              opt.chunkRadius[2] = rz;
            }
          else if(option == "LOAD_THREADS")
            {
              int threads;
              ss >> threads;
              opt.loadThreads = threads;
            }
          else if(option == "MESH_THREADS")
            {
              int threads;
              ss >> threads;
              opt.meshThreads = threads;
            }
          else
            {
              LOGW("Unknown option in '%s':  %s", CONFIG_PATH, option.c_str());
            }
        }
    }
  return opt;
}

static void writeDefaultOptions(const World::Options &opt)
{
  std::ofstream config(CONFIG_PATH);
  if(config.is_open())
    {
      config << "WORLD_NAME" << "\t" << DEFAULT_WORLD_NAME << "\n"
             << "TERRAIN" << "\t\t" << toString(opt.terrain) << "\n"
             << "SEED" << "\t\t" << opt.seed << "\n"
             << "CHUNK_RAD_X" << "\t" << opt.chunkRadius[0] << "\n"
             << "CHUNK_RAD_Y" << "\t" << opt.chunkRadius[1] << "\n"
             << "CHUNK_RAD_Z" << "\t" << opt.chunkRadius[2] << "\n"
             << "LOAD_THREADS" << "\t" << opt.loadThreads << "\n"
             << "MESH_THREADS" << "\t" << opt.meshThreads << "\n\n";
    }
  else
    {
      LOGE("Couldn't save defaults!");
    }
}

#endif // CONFIG_FILE_HPP
