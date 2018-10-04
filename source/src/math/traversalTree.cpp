#include "traversalTree.hpp"
#include <iostream>

TreeNode::TreeNode(ChunkPtr chunk, TreeNode *parent, int depth, bool visible)
  : chunk(chunk), pos(chunk ? chunk->pos() : Point3i()), hash(chunk ? Hash::hash(chunk->pos()) : 0),
    parent(parent), depth(depth), visible(visible)
{
  children.reserve(128);
}

int TreeNode::maxDepth() const
{
  int d = depth;
  for(auto &c : children)
    { d = std::max(d, c.maxDepth()); }
  return d;
}

void TreeNode::print()
{
  if(depth >= 0)
    {
      std::cout << "P" << pos << " --> D" << depth << ": (" << children.size() << " children):\n";
      for(auto &c : children)
        {
          if(c.depth >= 0)
            {
              for(int i = 0; i < depth+1; i++)
                { std::cout << "-"; }
              std::cout << "| ";
              c.print();
            }
        }
    }
}


TraversalTree::TraversalTree()
{

}

TraversalTree::~TraversalTree()
{

}


