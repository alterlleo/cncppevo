/*

  ____  _            _           _               
 | __ )| | ___   ___| | __   ___| | __ _ ___ ___ 
 |  _ \| |/ _ \ / __| |/ /  / __| |/ _` / __/ __|
 | |_) | | (_) | (__|   <  | (__| | (_| \__ \__ \
 |____/|_|\___/ \___|_|\_\  \___|_|\__,_|___/___/
                                                 

*/

#include <fmt/color.h>
#include <fmt/format.h>
#include <rang.hpp>
#include <sstream>
#include <math.h>

#include "block.hpp"

#ifdef DEBUG_BUILD
#include <iostream> // only for debugging
#endif

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

  data_t r;               // result
  current_acc = 0.0;

  if(t < 0){
    r = 0.0;
    s = 0;

  } else if(t < dt_1){                // acceleration phase

    r = a * pow(t, 2) / 2.0;
    s = a * t;
    current_acc = a;

  } else if(t < dt_1 + dt_m){         // maintenance phase

    r = f * (dt_1 / 2.0 + (t - dt_1));
    s = f;
    current_acc = 0.0;

  } else if(t < dt_1 + dt_m + dt_2){ // deceleration phase

    data_t t_2 = dt_1 + dt_m;
    r = f * dt_1 / 2.0 + f * (dt_m + t - t_2) + d / 2.0 * (pow(t, 2) + pow(t_2, 2)) - d * t * t_2;
    s = f + d * (t - t_2);
    current_acc = d;      // it is negative

  } else{

    r = l;
    s = 0.0;
    current_acc = 0.0;
  }

  r /= l;
  s *= 60;

  return r;
}


/*
--- PUBLIC METHODS ---
*/
Block::Block(string line) : _line(line), _n(0), prev(nullptr), next(nullptr) {} // everything else is calculated in the parsing function

Block::Block(string line, Block &p) : Block(line) {
  
  *this = p;        // copying from previous object

  // by redefining the assignement operator, this stuff is no more needed

  
  p.next = this;    
  prev = &p;

  // reset non-modal parameters
  _type = BlockType::NO_MOTION;
  _target.reset();
  _n = prev -> n() + 1;
  _line = line; 
  _parsed = false;
  _m = 0;
  
}

Block::Block(string line, Block *b) : Block(line){
  this -> _tool = b -> _tool;
  this -> _feedrate = b -> _feedrate;
  this -> _spindle = b -> _spindle;
  this -> _n = b -> _n + 1;
  this -> _target.reset();
  b -> next = this;
  prev = b;

  // reset non-modal parameters
  _type = BlockType::NO_MOTION;
  _target.reset();
  _n = prev -> n() + 1;
  _line = line; 
  _parsed = false;
  _m = 0;
}


Block::~Block(){

  if(_debug)
    cerr << style::italic << format("Block {:>3} destroyed [{:p}].", _n, (void *)(this)) << style::reset << endl;
}


Block &Block::operator=(Block &b){

  if(!b._parsed) throw CNCError("Previous block has not been parsed", this);
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
    return format("[{:>3}] {:} (not parsed yet)", _n, _line);
  }

  stringstream ss;
  
  auto color = color::red;
  // ss << format("[{:>3}] ", _n);
  
  if(_type == BlockType::NO_MOTION)
    color = color::gray;
  else if (_type == BlockType::RAPID)
    color = color::magenta;
  
  ss << format("[{:>3}] ", _n);
  if (colored)
    ss << format("G{:0>2} ", styled(static_cast<int>(_type), fmt::fg(color)));
  else
    ss << format("G{:0>2} ", static_cast<int>(_type));
  ss << format("({:-^9}) ", Block::types.at(_type)) << _target.desc();
  ss << format(" F{:>5.0f} S{:>4.0f} ", _feedrate, _spindle);
  ss << format("T{:0>2} M{:0>2} ", _tool, _m);
  ss << format("L{:>6.2f}mm DT{:>6.2f}s", _length, _profile.dt);
  return ss.str();

}


/*
  ____        _     _ _                       _   _               _     
 |  _ \ _   _| |__ | (_) ___   _ __ ___   ___| |_| |__   ___   __| |___ 
 | |_) | | | | '_ \| | |/ __| | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
 |  __/| |_| | |_) | | | (__  | | | | | |  __/ |_| | | | (_) | (_| \__ \
 |_|    \__,_|_.__/|_|_|\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                        
*/


Block &Block::parse(const Machine *m){
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

  //cout << "check before delta -> " << __LINE__ << endl;

  _delta = _target.delta(start_point());

  //cout << "check after delta" << __LINE__ << endl;

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

  return *this;
}


data_t Block::lambda(data_t time, data_t &speed){

  if(!_parsed) throw CNCError("Block not parsed", this);
  return _profile.lambda(time, speed);

}

Point Block::interpolate(data_t lambda){
  if(!_parsed) throw CNCError("Block not parsed", this);

  Point result = Point();
  Point p0 = start_point();

  // 2 cases:

  if(_type == BlockType::LINE){   // 1 -> the block describes a segment

    result.x(p0.x() + _delta.x() * lambda);
    result.y(p0.y() + _delta.y() * lambda);

  } else if (_type == BlockType::CWA || _type == BlockType::CCWA){

    data_t angle = _theta_0 + _dtheta * lambda;
    result.x(_center.x() + _r * cos(angle));
    result.y(_center.y() + _r * sin(angle));

  }

  // z is good for both cases
  result.z(p0.z() + _delta.z() * lambda);
  return result;

}

Point Block::interpolate(data_t time, data_t &lambda, data_t &speed){
  
  lambda = this -> lambda(time, speed);
  return interpolate(lambda);
}

void Block::walk(function<void(Block &b, data_t t, data_t l, data_t s)> f){

  if(!_parsed) throw CNCError("Block not parsed", this);

  data_t t = 0.0, l, s;

  while(t < _profile.dt /*+ _machine -> tq() / 2.0*/) {     // we add half time sample in order to be more robust. Otherwise the risk would be to miss the last time step

    l = lambda(t, s);
    f(*this, t, l, s);      // l is lambda
    t += _machine -> tq();  // increasing because we are evaluating step by step

  }

}

void Block::update_target(data_t x, data_t y){
  _target.x(x);
  _target.y(y);
}


/*
  ____       _            _                        _   _               _     
 |  _ \ _ __(_)_   ____ _| |_ ___   _ __ ___   ___| |_| |__   ___   __| |___ 
 | |_) | '__| \ \ / / _` | __/ _ \ | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
 |  __/| |  | |\ V / (_| | ||  __/ | | | | | |  __/ |_| | | | (_) | (_| \__ \
 |_|   |_|  |_| \_/ \__,_|\__\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                             
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
    if(prev && _n <= prev -> n())
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


Point Block::start_point(){

  return prev ? prev -> target() : _machine -> zero();
}


void Block::compute(){

  // Referencing to profile fields
  _profile.l = _length;


  data_t &dt = _profile.dt;
  data_t &dt_1 = _profile.dt_1;
  data_t &dt_2 = _profile.dt_2;
  data_t &dt_m = _profile.dt_m;
  data_t &a = _profile.a;
  data_t &d = _profile.d;
  data_t &f_m = _profile.f;
  data_t &l = _profile.l;            // trick, l is like an alias
  data_t &A = _acc;               // nominal accelaration
  data_t dq;

/*
  data_t dt, dt_1, dt_m, dt_2, dq;   // dq is the minimum time step -> the tick
  data_t f_m;                     // real feedrate
  data_t &l = _length;            // trick, l is like an alias
  data_t &A = _acc, a, d;           // nominal accelaration
*/

  // ... Computations ...

  f_m = _arc_feedrate / 60.0;     // gcode use mm and minutes
  dt_1 = f_m / A;
  dt_2 = dt_1;
  dt_m = l / f_m - (dt_1 + dt_2) / 2.0;

  // we want to reshape the trapezoidal in order to keep the area consntant and equal to the lenght during the time quantizing

  if(dt_m > 0){                   // long block

    dt = _machine -> quantize(dt_1 + dt_m + dt_2, dq);
    dt_m = dt_m + dq;
    f_m = (2 * l) / (dt_1 + dt_2 + 2 * dt_m);   // reshaping of the trapezoid height

  } else{                         // short block -> triangle

    dt_1 = dt_2 = sqrt(l / A);
    dt = _machine -> quantize(dt_1 + dt_2, dq);
    dt_m = 0;
    dt_2 = dt_2 + dq;
    f_m = 2 * l / (dt_1 + dt_2);

  }

  a = f_m / dt_1;       // reduced value of the acceleration
  d = -(f_m / dt_2);    // reduced value of the decelration

  /*
  -> if declaring tmp variables as dt, dt_1, dt_m, dt_2, dq, f_m ,a, d
        then after the computations you must assign the results to the profile fields.

  // declarations of temporary variables:
  data_t dt, dt_1, dt_m, dt_2, dq;   // dq is the minimum time step -> the tick
  data_t f_m;                     // real feedrate
  data_t &l = _length;            // trick, l is like an alias
  data_t &A = _acc, a, d;           // nominal accelaration

  // ... computations ...
  
  // set of the profile variables
  _profile.dt_1 = dt_1;
  _profile.dt_2 = dt_2;
  _profile.dt_m = dt_m;
  _profile.a = a;
  _profile.d = d;
  _profile.f = f_m;
  _profile.dt = dt;
  _profile.l = l;

  -> if declaring the same variables but inizializing them as references of the actual profile fields, the computation results refers already to the profile fields (reference assignement). This is the current implementation

  */

}

void Block::calc_arc() {
  data_t x0, y0, z0, xc, yc, xf, yf, zf;
  Point p0 = start_point();
  x0 = p0.x();
  y0 = p0.y();
  z0 = p0.z();
  xf = _target.x();
  yf = _target.y();
  zf = _target.z();

  if (_r) { // if the radius is given
    data_t dx = _delta.x();
    data_t dy = _delta.y();
    data_t dxy2 = pow(dx, 2) + pow(dy, 2);
    data_t delta = -pow(dy, 2) * dxy2 * (dxy2 - 4 * _r * _r);

    if (delta < 0) {
      throw CNCError("Invalid trajectory", this);
    }
    
    data_t sq = sqrt(delta);
    // signs table
    // sign(r) | CW(-1) | CCW(+1)
    // --------------------------
    //      -1 |     +  |    -
    //      +1 |     -  |    +
    int s = (_r > 0) - (_r < 0);
    s *= (_type == BlockType::CCWA ? 1 : -1);
    xc = x0 + (dx - s * sq / dxy2) / 2.0;
    yc = y0 + dy / 2.0 + s * (dx * sq) / (2 * dy * dxy2);
  } else { // if I,J are given
    data_t r2;
    _r = hypot(_i, _j);
    xc = x0 + _i;
    yc = y0 + _j;
    r2 = hypot(xf - xc, yf - yc);
    if (fabs(_r - r2) > _machine->error()) {
      throw CNCError(
          fmt::format("Arc endpoints mismatch error ({:})", _r - r2).c_str(),
          this);
    }
  }
  _center.x(xc);
  _center.y(yc);
  _center.z((zf - z0) / 2);
  _theta_0 = atan2(y0 - yc, x0 - xc);
  _dtheta = atan2(yf - yc, xf - xc) - _theta_0;
  // we need the net angle so we take the 2PI complement if negative
  if (_dtheta < 0)
    _dtheta = 2 * M_PI + _dtheta;
  // if CW, take the negative complement
  if (_type == BlockType::CWA)
    _dtheta = -(2 * M_PI - _dtheta);
  //
  _length = hypot(zf - z0, _dtheta * _r);
  // from now on, it's safer to drop the sign of radius angle
  _r = fabs(_r);

}


/*
  _____         _                     _       
 |_   _|__  ___| |_   _ __ ___   __ _(_)_ __  
   | |/ _ \/ __| __| | '_ ` _ \ / _` | | '_ \ 
   | |  __/\__ \ |_  | | | | | | (_| | | | | |
   |_|\___||___/\__| |_| |_| |_|\__,_|_|_| |_|
                                                                          
*/

#ifdef BLOCK_MAIN

#include <iostream>
#include "machine.hpp"

using namespace cncpp;
using namespace std;

int main(){

  cerr << "Version: " << cncpp::version() << endl;
  Machine m = Machine();
  auto b1 = Block("N10 G00 x100 y200 z10 F5000 S5000 T1").parse(&m);
  auto b2 = Block("N20 G01 X10 y20", b1).parse(&m);
  
  cerr << "b1: " << b1.desc() << endl;
  cerr << "b2: " << b2.desc() << endl;
  
  // Walk along b2
  b2.walk([&](Block &b, data_t t, data_t l, data_t s) {
    Point pos = b.interpolate(l);
    cout << format("{:} {:} {:} {:} {:} {:}", t, l, s, pos.x(), pos.y(), pos.z()) << endl;
  });


  return 0;
}

#endif
