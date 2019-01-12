#ifndef CharTree_hpp
#define CharTree_hpp

/** @file CharTree.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 01/12/19
  */

#include<map>
struct CharTree
{
  protected:
    std::map<char,CharTree> children;
    CharTree* parent = nullptr;

  public:
    int add(const std::string& s);
    int add(std::string::const_iterator b, std::string::const_iterator e);

    const std::map<char,CharTree>& get_children() const;
    const CharTree* get_parent() const;

    const CharTree* match(const std::string& str) const;
    const CharTree* match(std::string::const_iterator b, std::string::const_iterator e) const;
    int depth() const;
};


#endif // include protector
