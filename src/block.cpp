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
#include <sstream>
#include <math.h>

using namespace std;
using namespace cncpp;
using namespace fmt;
using namespace rang;

// types[BlockType::RAPID] => "Rapid", a way to translatin a blocktype to a string
const map<Block::BlockType, string> Block::types = {
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

  stringstream ss;
  
  auto color = color::red;
  ss << format("[{:>3}] ", _n);
  
  if(_type == BlockType::NO_MOTION)
    color = color::gray;
  else if (_type == BlockType::RAPID)
    color = color::magenta;
  
  ss << format("G{:0>2} ", styled(static_cast<int>(_type), fmt::fg(color)));
  ss << format("({:-9}) ", Block::types.at(_type)) << _target.desc();
  ss << format(" F{:>5.0f} S{:>4.0f} ", _feedrate, _spindle);
  ss << format("T{:0>2} M{:0>2} ", _tool, _m);
  ss << format("L{:>6.2f}mm DT{:>6.2f}s ", _length, _profile.dt);

  return ss.str();
}

/*
--- METHODS ---
*/
void Block::parse(const Machine *m){
  _machine = m;

  stringstream ss(_line);
  string token;             // token = word = part of the string

  while(ss >> token){       // when we reach the end of the string, the token is false. Each token is composed by the letter and the argument that is the number, so dependending on the letter the number has different meanings
    
    try{
      parse_token(token);
    
    } catch(CNCError &e){
      stringstream ss;
      
      ss << "Parsing error at line: " << _line << endl;
      ss << "Token: " << token << endl;
      ss << "Exception: " << e.what() << endl;

      // if there is an error here, we don't want to keep the program working
      throw CNCError(ss.str(), this);
    }
  }

  // modal fields
  _target.modal(start_point());
  _delta = _target.delta(start_point);
  _acc = _machine -> A();               // A is the max acceleration provided by the machine
  _length = _delta.length();

  switch(_type){

    case BlockType::LINE:
      _acc = _machine -> A();
      _arc_feedrate = _feedrate;
      compute();
      break;

    case BlockType::CWA: 
    case BlockType::CCWA: 
      // in these cases we have to compute the arc radius
      calc_arc();

      _arc_feedrate = min(
        _feedrate,      // nominal one
        pow(3.0 / 4.0 * pow(_machine -> A(), 2) * pow(_r, 2), 0.25) * 60
      );

      compute();
      break;
    
    default:
      break;
  }

  _parsed = true;
}


data_t Block::lambda(data_t time, data_t &speed){
  
  return _profile.lambda(time, speed);
}



/*
--- PRIVATE METHODS ---
*/
void Block::parse_token(string token){

}

Point Block::start_point(){

}

void compute(){

}

void calc_arc(){

}
