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

  // by redefining the assignement operator, this stuff is no more needed

  /*
  p.next = this;    
  prev = &p;

  // reset non-modal parameters
  _type = BlockType::NO_MOTION;
  _target.reset();
  _n = prev -> n() + 1;
  _line = line; 
  */
}

Block::~Block(){

  if(_debug)
    cerr << style::italic << format("Block {:>3} destroyed. ", _n) << style::reset << endl;
}

Block &Block::operator=(Block &b){
  // here we assign all the parameters that must be automatically inherited
  _tool = b._tool;      // we can access the private field because we are working on the same class
  _feedrate = b._feedrate;
  _spindle = b._spindle;
  _n = b._n + 1;

  _target.reset();
  b.next = this;
  prev = &b;

  return *this;
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
  _delta = _target.delta(start_point());
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
      );1

      compute();
      break;
    
    default:
      break;
  }

  _parsed = true;
}


data_t Block::lambda(data_t time, data_t &speed){
  
  if(!_parsed)
    throw CNCError("Block not parsed", this);

  return _profile.lambda(time, speed);
}


/*
--- PRIVATE METHODS ---
*/

/**
 * 
 * @brief from a line of gcode we extract the informations
 * @param token i-th line of the gcode
 * 
 */
void Block::parse_token(string token){

  // we want to support both capital and lower cases, let's put all capital
  char cmd = toupper(token[0]);   // token[0] is the first letter in the token

  string arg = token.substr(1);   // sub string that starts at the element
  if(arg.empty())
    throw CNCError("Empty command argument", this);

  switch(cmd){

  case 'N':
    _n = stoi(arg);             // stoi takes a string/character and it parses it as an integer
    if(prev && _n <= prev -> _n)
      throw CNCError("Block number must be increasing: " + to_string(prev -> _n), this);
    break;

  case 'G':
    _type = static_cast<BlockType>(stoi(arg));
    if(_type > BlockType::NO_MOTION)
      throw CNCError("Unknown G type", this);
    break;

  case 'X':
    _target.x(stod(arg));       //stod is string to double
    break;

  case 'Y':
    _target.y(stod(arg));
    break;

  case 'Z':
    _target.z(stod(arg));
    break;

  case 'I':
    _i = stod(arg);
    break;

  case 'J':
    _j = stod(arg);
    break;

  case 'R':
    _r = stod(arg);
    break;

  case 'F':
    _feedrate = stod(arg);
    break;
  
  case 'S':
    _spindle = stod(arg);
    break;

  case 'T':
    _tool = stoi(arg);
    break;
  
  case 'M':
    _m = stoi(arg);
    break;

  default:
  
    stringstream ss;
    ss << "Unknown / unsopported command: '" << token << "'";
    throw CNCError(ss.str(), this);

  }
}

/**
 * 
 * @brief the previous block if it is present -> the starting point of the machine, so the current positino before the destination point
 * 
 */
Point Block::start_point(){

  return prev ? prev -> target() : _machine -> zero();
}

/**
 * 
 * @brief evaluate the velocity profile, both for the trapezoidal profile and the triangular profile
 * 
 */
void Block::compute(){

  data_t dt, dt_1, dt_m, dt_2, dq;   // dq is the minimum time step -> the tick
  data_t f_m;                     // real feedrate
  data_t &l = _length;            // trick, l is like an alias
  data_t &A = _acc, a, d;           // nominal accelaration

  f_m = _arc_feedrate / 60.0;     // gcode use mm and minutes
  dt_1 = f_m / A;
  dt_2 = dt_1;
  dt_m = l / f_m - (dt_1 + dt_2) / 2.0;

  // we want to reshape the trapezoidal in order to keep the area consntant adn equal to the lenght

  if(dt_m > 0){                   // long block


  } else{                         // short block -> triangle


  }


}

void calc_arc(){

}
