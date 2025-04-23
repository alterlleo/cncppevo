/*

  ____  _            _           _               
 | __ )| | ___   ___| | __   ___| | __ _ ___ ___ 
 |  _ \| |/ _ \ / __| |/ /  / __| |/ _` / __/ __|
 | |_) | | (_) | (__|   <  | (__| | (_| \__ \__ \
 |____/|_|\___/ \___|_|\_\  \___|_|\__,_|___/___/
                                                 

*/

#include "block.hpp"
#include <fmt/color.h>
#include <fmt/format.h>
#include <rang.hpp>

using namespace std;
using namespace cncpp;
using namespace fmt;
using namespace rang;

// types[BlockType::RAPID] => "Rapid", a way to translatin a blocktype to a string
const map<Block::BlockType, string> types = {
  {BlockType::RAPID, "Rapid"},
  {BlockType::LINE, "Line"},
  {BlockType::CWA, "CW Arc"},
  {BlockType::CCWA, "CCW Arc"},
  {BlockType::NO_MOTION, "No motion"}
};

data_t Block::Profile::lambda(data_t t, data_t &s){


  return 0;
}


/*
--- PUBLIC METHODS ---
*/
Block::Block(string line) : _line(line), _n(0) {} // everything else is calculated in the parsing function

Block::Block(string line, Block &p) : Block(line) {
  
  *this = p;        // copying from previous object
  p.next = this;    
  prev = &p;

  // reset non-modal parameters
  _type = BlockType::NO_MOTION;
  _target.reset();
  _n = prev -> n() + 1;
  _line = line;

}

Block::~Block(){

  if(_debug)
    cerr << style::italic << format("Block {:>3} destroyed. ", _n) << style::reset << endl;
}

string Block::desc(bool colored) const{
  
  if(!_parsed){
    return format("[{:>3}] {:} (not parsed yet)", _n, line);
    
  }
}
