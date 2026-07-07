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
#include "blockTRC.hpp"

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

data_t Block::Profile::lambda(data_t time, data_t &s) {
    if (time < 0) {
        s = fs * 60.0;
        current_acc = 0.0;
        return 0.0;
    }

    if (time >= dt) {
        s = fe * 60.0;
        current_acc = 0.0;
        return 1.0;
    }

    // 7 phases jerk
    data_t jerks[7] = { j, 0.0, -j, 0.0, -j, 0.0, j };
    
    data_t current_time = time;
    data_t pos = 0.0;
    data_t vel = fs;
    data_t acc = 0.0;

    for (int i = 0; i < 7; i++) {
        if (t[i] == 0.0) continue;

        data_t delta_t = (current_time > t[i]) ? t[i] : current_time;
        pos += vel * delta_t + 0.5 * acc * pow(delta_t, 2) + (jerks[i] * pow(delta_t, 3)) / 6.0;
        vel += acc * delta_t + 0.5 * jerks[i] * pow(delta_t, 2);
        acc += jerks[i] * delta_t;

        current_time -= delta_t;
        if (current_time <= 0) break;
    }

    current_acc = acc;
    s = vel * 60.0; // to mm/min or deg/min)
    return pos / l;
}


/*
--- PUBLIC METHODS ---
*/
Block::Block(string line) : _line(line), _n(0), prev(nullptr), next(nullptr) {} // everything else is calculated in the parsing function

Block::Block(string line, BlockTRC &p) : Block(line) {
  
  _tool = p._tool;
  _feedrate = p._feedrate;
  _spindle = p._spindle;

  p.next = static_cast<BlockTRC*>(this);    
  prev = &p;

  _type = BlockType::NO_MOTION;
  _target.reset();
  _n = prev -> n() + 1; 
  _line = line; 
  _parsed = false;
  _m = 0;
}

Block::Block(string line, BlockTRC *b) : Block(line){
  this -> _tool = b -> _tool;
  this -> _feedrate = b -> _feedrate;
  this -> _spindle = b -> _spindle;
  this -> _n = b -> _n + 1;

  
  this -> _target.reset();
  b -> next = static_cast<BlockTRC*>(this);
  prev = b;

  // reset non-modal parameters
  _type = BlockType::NO_MOTION;
  _target.reset();
  _n = prev -> n() + 1;
  _line = line; 
  _parsed = false;
  _m = 0;

  // _pitch = _prev -> pitch();
  // _yaw = _prev -> yaw();
}


Block::~Block(){

  if(_debug)
    cerr << style::italic << fmt::format("Block {:>3} destroyed [{:p}].", _n, (void *)(this)) << style::reset << endl;
}


/*
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
*/

string Block::desc(bool colored) const{
  
  if(!_parsed){
    return fmt::format("[{:>3}] {:} (not parsed yet)", _n, _line);
  }

  stringstream ss;
  
  auto color = color::red;
  // ss << fmt::format("[{:>3}] ", _n);
  
  if(_type == BlockType::NO_MOTION)
    color = color::gray;
  else if (_type == BlockType::RAPID)
    color = color::magenta;
  
  ss << fmt::format("[{:>3}] ", _n);
  if (colored)
    ss << fmt::format("G{:0>2} ", styled(static_cast<int>(_type), fmt::fg(color)));
  else
    ss << fmt::format("G{:0>2} ", static_cast<int>(_type));
  ss << fmt::format("({:-^9}) ", Block::types.at(_type)) << _target.desc();
  ss << fmt::format(" F{:>5.0f} S{:>4.0f} ", _feedrate, _spindle);
  ss << fmt::format("T{:0>2} M{:0>2} ", _tool, _m);
  ss << fmt::format("L{:>6.2f}mm DT{:>6.2f}s", _length, _profile.dt);
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
      if(!parse_token(token)) break;
    
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

    case BlockType::RAPID:{
      if(_delta.z() != 0 || _delta.a() != 0 || _delta.c() != 0){
        _acc = _machine -> A_stepper(); // lower acceleration for positioning axes
      }

      data_t max_angle = max(fabs(_delta.a()), fabs(_delta.c()));
      if(max_angle > 0){

        _length = max_angle;   // if the movement is 0, let's use _lenght as a measure of positioning rotation
      }
      break;
    }

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
  result.a(p0.a() + _delta.a() * lambda);
  result.c(p0.c() + _delta.c() * lambda);

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

bool Block::parse_token(string token){

  // we want to support both capital and lower cases, let's put all capital
  char cmd = toupper(token[0]);   // token[0] is the first letter in the token

  string arg = token.substr(1);   // sub string that starts at the element
  if (cmd == '#' || cmd == ';')
    return false;

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

  case 'A':
    _target.a(stod(arg));
    break;
  
  case 'C':
    _target.c(stod(arg));
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

  return true;
}


Point Block::start_point(){

  return prev ? prev -> target() : _machine -> zero();
}


void Block::compute() {

  _profile.l = _length;
  data_t A = _acc;   
  data_t J = _machine->J(); 
  if(prev && (this->_target.a() != prev->_target.a() || this->_target.c() != prev->_target.c())){
    A = _machine->A_stepper();
  }

  _profile.j = J;
  _profile.a_max = A;
  _profile.d_max = A;
  data_t &fs = _profile.fs;
  data_t &fe = _profile.fe;
  data_t v_cruise = _arc_feedrate / 60.0;     
  data_t L = _length;

  // Helping lambda function to calculate the S-curve profile for acceleration or deceleration. 3 phases:
  // Jerk + (t_j1), Constant Acceleration (t_const), Jerk - (t_j2)
  // The area under the S-curve is equal to the area of a trapezoid with the same duration and speed.
  auto calc_s_curve = [&](data_t v_start, data_t v_end, data_t max_acc, data_t &t_j1, data_t &t_const, data_t &t_j2) -> data_t {
    if (fabs(v_start - v_end) < 1e-6) {
      t_j1 = t_const = t_j2 = 0.0;
      return 0.0;
    }
    
    data_t delta_v = fabs(v_end - v_start);
    data_t tj_ideal = max_acc / J;
    data_t dv_ideal = J * pow(tj_ideal, 2);

    if (delta_v <= dv_ideal) { // acceleration triangle
      t_j1 = sqrt(delta_v / J);
      t_const = 0.0;
      t_j2 = t_j1;

    } else { // acceleration trapezoid
      t_j1 = tj_ideal;
      t_j2 = tj_ideal;
      t_const = (delta_v - dv_ideal) / max_acc;
    }
    
    data_t total_time = t_j1 + t_const + t_j2;
    return ((v_start + v_end) / 2.0) * total_time;
  };

  data_t d_acc, d_dec;
  auto try_cruise_vel = [&](data_t v_target) -> data_t {
    d_acc = calc_s_curve(fs, v_target, _profile.a_max, _profile.t[0], _profile.t[1], _profile.t[2]);
    d_dec = calc_s_curve(v_target, fe, _profile.d_max, _profile.t[4], _profile.t[5], _profile.t[6]);
    return d_acc + d_dec;
  };

  data_t d_req = try_cruise_vel(v_cruise);

  // 2 casess: long profile (we can reach v_cruise) and short profile (we can't reach v_cruise)
  if (d_req <= L) { // long profile
    _profile.f = v_cruise;
    _profile.t[3] = (L - d_req) / v_cruise;

  } else { // short profile
    data_t v_low = max(fs, fe);
    data_t v_high = v_cruise;
    data_t v_peak = v_low;
    
    for (int i = 0; i < 20; i++) {
      v_peak = (v_low + v_high) / 2.0;
      if (try_cruise_vel(v_peak) > L) {
        v_high = v_peak;
      } else {
        v_low = v_peak;
      }
    }
    
    _profile.f = v_peak;
    _profile.t[3] = 0.0;
    try_cruise_vel(v_peak);
  }

  // total time for quantization
  data_t t_total = 0.0;
  for (int i = 0; i < 7; i++) {
    t_total += _profile.t[i];
  }
  data_t dq;
  _profile.dt = _machine -> quantize(t_total, dq);
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
    cout << fmt::format("{:} {:} {:} {:} {:} {:}", t, l, s, pos.x(), pos.y(), pos.z()) << endl;
  });


  return 0;
}

#endif
