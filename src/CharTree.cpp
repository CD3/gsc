#include "./CharTree.hpp"

int CharTree::add(const std::string &str)
{
  return add(str.begin(),str.end());
}

int CharTree::add(std::string::const_iterator beg, std::string::const_iterator end)
{
  int added = 0;
  if(beg == end)
    return added;

  if( !children.count(*beg) )
  {
    children[*beg] = CharTree();
    children[*beg].parent = this;
    added++;
  }

  return added + children[*beg].add(beg+1,end);
  
}

const std::map<char,CharTree>& CharTree::get_children() const
{
  return children;
}

const CharTree* CharTree::get_parent() const
{
  return parent;
}


int CharTree::depth() const
{
  if( parent == nullptr )
    return 0;

  return parent->depth() + 1;
}

const CharTree* CharTree::match(const std::string& str) const
{
  return match(str.begin(), str.end());
}

const CharTree* CharTree::match(std::string::const_iterator beg, std::string::const_iterator end) const
{
  if( children.size() == 0)
    return this;

  if( beg == end )
    return nullptr;

  if( !children.count(*beg) )
    return nullptr;

  return children.at(*beg).match(beg+1,end);
}
