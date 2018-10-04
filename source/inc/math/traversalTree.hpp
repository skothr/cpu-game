#ifndef TRAVERSAL_TREE_HPP
#define TRAVERSAL_TREE_HPP

#include "vector.hpp"
#include "chunk.hpp"
#include "hashing.hpp"
#include <vector>

struct TreeNode
{
  TreeNode(ChunkPtr chunk = nullptr, TreeNode *parent = nullptr, int depth = 0, bool visible = true);
  
  ChunkPtr chunk;
  Point3i pos;
  hash_t hash;
  
  int depth;
  bool visible = true;
  
  TreeNode *parent = nullptr;
  std::vector<TreeNode> children;

  int maxDepth() const;
  void print();
};

class TraversalTree
{
public:
  TraversalTree();
  ~TraversalTree();
  
private:
  //TreeNode base;
};

#endif // TRAVERSAL_TREE_HPP
