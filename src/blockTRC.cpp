/*
  ____  _            _    _____ ____   ____        _               
 | __ )| | ___   ___| | _|_   _|  _ \ / ___|   ___| | __ _ ___ ___ 
 |  _ \| |/ _ \ / __| |/ / | | | |_) | |      / __| |/ _` / __/ __|
 | |_) | | (_) | (__|   <  | | |  _ <| |___  | (__| | (_| \__ \__ \
 |____/|_|\___/ \___|_|\_\ |_| |_| \_\\____|  \___|_|\__,_|___/___/

IDEA:

Program class should be a list of *Block, so it can contain also pointers to BlockTRC instances. Is the effort worth?

program.cpp -> when adding a new BlockTRC through << operator, parse function of BlockTRC is called. During parsing, check the angle between the destination and the starting point. If angle > pi, then rise a flag (_shaping_required). Then after parsing and adding the block, so after << operator in Program, let's check the flag of the added block. If high, let's rise a flag in Program. This last flag then msut be checked and if high, add the arc "junction" block (this new block should be computed in Program class? TBD)
                                                                   
*/

#include <fmt/color.h>
#include <fmt/format.h>
#include <rang.hpp>
#include <sstream>
#include "blockTRC.hpp"

using namespace std;
using namespace cncpp;
using namespace fmt;
using namespace rang;

const map<BlockTRC::TRCType, string> BlockTRC::trc_types = {

  {TRCType::NONE, "None"},
  {TRCType::LEFT, "Left"},
  {TRCType::RIGHT, "Right"}
};

BlockTRC::BlockTRC(string line) : Block(line) , _trc(false) {}

BlockTRC::BlockTRC(string line, BlockTRC &prev) : Block(line, prev), _trc(false) {}

BlockTRC::~BlockTRC(){}

BlockTRC &BlockTRC::operator=(BlockTRC &b){

  Block::operator=(b);
  _trc = b.trc();

  return *this;
}

BlockTRC &BlockTRC::parse(const Machine *m){
  _machine = m;

  stringstream ss(_line);
  string token;

  while(ss >> token){

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

  _target.modal(start_point());
  _delta = _target.delta(start_point());
  _acc = _machine -> A();
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

  
  switch(_trc_type){

    case TRCType::NONE:
      _trc = false;
      break;

    case TRCType::LEFT:

    case TRCType::RIGHT:

      _trc = true;

      if(angle_with_prev() > 3.1415 && dynamic_cast<BlockTRC*>(prev) -> trc()){   // TODO: #define in defines.hpp for pi
        
        _shaping_required = true;
      } else{

        _shaping_required = false;
      }
    
    default:
      break;
  }

  _parsed = true;

  return *this;
}

string BlockTRC::desc(bool colored) const{

  if(!_parsed){
    return format("[{:>3}] {:} (not parsed yet)", _n, _line);
  }

  stringstream ss;

  auto color = color::red;
  
  if(_type == BlockType::NO_MOTION)
    color = color::gray;
  else if (_type == BlockType::RAPID)
    color = color::magenta;
  
  ss << format("[{:>3}] ", _n);
  if (colored){
    ss << format("G{:0>2} ", styled(static_cast<int>(_type), fmt::fg(color)));

  }else
    ss << format("G{:0>2} ", static_cast<int>(_type));
    ss << format("G{:0>2} ", static_cast<int>(_trc_type));
    ss << format("({:-^9}) ", Block::types.at(_type)) << _target.desc();
    ss << format(" F{:>5.0f} S{:>4.0f} ", _feedrate, _spindle);
    ss << format("T{:0>2} M{:0>2} ", _tool, _m);
    ss << format("L{:>6.2f}mm DT{:>6.2f}s", _length, _profile.dt);
  return ss.str();

}


string BlockTRC::arc_shaping() {

  _shaping_required = false;

  const Point p0 = prev->target();
  const Point p1 = target();
  const Point pm = prev->start_point();

  Point v1 = p0.delta(pm);
  Point v2 = p1.delta(p0);
  v1.scale(1 / v1.length());
  v2.scale(1 / v2.length());

  // bisector = vector that connectes the prev starting point and the actual target. Thanks to that it's possible to find the center of the junction arc
  Point bisector = v1 + v2;
  bisector.scale(1 / bisector.length());

  Point normal(-bisector.y(), bisector.x());
  if (_trc_type == TRCType::RIGHT) {
    normal.scale(-1);
  }

  data_t r = _machine->machine_tool_radius();
  normal.scale(r);
  Point arc_center = p0 + normal;

  data_t i = arc_center.x() - p0.x();
  data_t j = arc_center.y() - p0.y();

  // G2 or G3 depending junction curvature side
  data_t side = v1.x() * v2.y() - v1.y() * v2.x();
  int arc_code = (side < 0) ? 2 : 3;

  std::string arc_line = fmt::format(
    "G{:<02d} X{:.3f} Y{:.3f} I{:.3f} J{:.3f} F{:.0f}",
    arc_code,
    p0.x(), p0.y(),
    i, j,
    _feedrate
  );

  return arc_line;
}


/*
  ____       _            _                        _   _               _     
 |  _ \ _ __(_)_   ____ _| |_ ___   _ __ ___   ___| |_| |__   ___   __| |___ 
 | |_) | '__| \ \ / / _` | __/ _ \ | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
 |  __/| |  | |\ V / (_| | ||  __/ | | | | | |  __/ |_| | | | (_) | (_| \__ \
 |_|   |_|  |_| \_/ \__,_|\__\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                             
*/

void BlockTRC::parse_token(string token){
    // we want to support both capital and lower cases, let's put all capital
  char cmd = toupper(token[0]);   // token[0] is the first letter in the token

  string arg = token.substr(1);   // sub string that starts at the element
  if(arg.empty())
    throw CNCError("Empty command argument", this);

  switch(cmd){

  case 'N':
    _n = stoi(arg);             // stoi takes a string/character and it parses it as an integer
    if(prev && _n <= prev -> n())
      throw CNCError("Block number must be increasing: " + to_string(prev -> n()), this);
    break;

  case 'G':

    if(static_cast<BlockType>(stoi(arg)) <= BlockType::NO_MOTION){

      _type = static_cast<BlockType>(stoi(arg));
    
    } else if(static_cast<TRCType>(stoi(arg)) >= TRCType::NONE && static_cast<TRCType>(stoi(arg)) <= TRCType::RIGHT){

      _trc_type = static_cast<TRCType>(stoi(arg));
    
    } else{

      throw CNCError("Unknown G type", this);
    }

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


data_t BlockTRC::angle_with_prev(){

  return 2.9;
}


#ifdef BLOCKTRC_MAIN

#include "machine.hpp"
#include <iostream>

using namespace cncpp;
using namespace std;

int main(){

  cerr << "Version: " << cncpp::version() << endl;
  Machine m = Machine();
  auto b1 = BlockTRC("N10 G00 x100 y200 z10 F5000 S5000 T1").parse(&m);
  auto b2 = BlockTRC("N20 G01 X10 y20", b1).parse(&m);
  
  cerr << "b1: " << b1.desc() << endl;
  cerr << "b2: " << b2.desc() << endl;
  
  // Walk along b2
  b2.walk([&](Block &b, data_t t, data_t l, data_t s) {
    Point pos = b.interpolate(l);
    cout << format("{:} {:} {:} {:} {:} {:}", t, l, s, pos.x(), pos.y(), pos.z()) << endl;
  });

  cerr << "---- adding g40 block ----" << endl;
  auto b3 = BlockTRC("N20 G41 G01 X10 y20", b1).parse(&m);
  cerr << "b3: " << b3.desc() << endl;

  return 0;
}



#endif





