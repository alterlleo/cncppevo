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

BlockTRC::BlockTRC(string line) : Block(line), _trc(false) {}

BlockTRC::BlockTRC(string line, BlockTRC &prev) : Block(line, prev), _trc(false) {}

BlockTRC::BlockTRC(string line, BlockTRC *b) : Block(line, b), _trc(false) {}

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

      _shaping_required = is_shaping_needed(); 
    
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

void BlockTRC::shift_prev_target(){

  BlockTRC *p = dynamic_cast<BlockTRC*>(prev);

  if(p -> trc() /*|| p -> shaping_corner()*/){
    
    if (p -> type() == BlockType::LINE && type() == BlockType::LINE) {

        line_line_shift(p);

      } else if (p -> type() == BlockType::LINE && (type() == BlockType::CWA || type() == BlockType::CCWA)) {
        
        line_arc_shift(p);
      
      } else if ((p -> type() == BlockType::CWA || p -> type() == BlockType::CCWA ) && type() == BlockType::LINE){

        // TODO

      }
  }

}

void BlockTRC::line_line_shift(BlockTRC *prev){

  BlockTRC *p = prev;

  if(!p){
    throw CNCError("Null previous block pointer ", this);
    return;
  }

  Point sp = p -> start_point();                // starting point of the previous block
  Point tp = p -> target();                     // target point of the previous block
  Point sc = start_point();                     // starting point of the current block
  Point tc = target();                          // target point of the current block
  data_t r = _machine -> machine_tool_radius(); // current tool radius

  cerr << style::italic << "Starting TRC line-line between: " << endl << style::reset << this -> desc() << endl; 
  cerr << style::italic << "And previous move: " << style::reset << endl;
  cerr << style::italic << p -> desc() << style::reset << endl << endl;

  // TODO : when angle > pi, then offset it's enough, no intersections because there is arc_shaping

  if(!is_shaping_needed()){

    Point v1 = tp.delta(sp);
    Point v2 = tc.delta(sc);
    v1.scale(1 / v1.length());
    v2.scale(1 / v2.length());

    // find line parameters
    data_t a1 = v1.y() / v1.x();
    data_t a2 = v2.y() / v2.x();
    data_t b1 = tp.y() - a1 * tp.x();
    data_t b2 = tc.y() - a2 * tc.x();

    data_t offset_side = (p -> _trc_type == TRCType::LEFT) ? 1.0 : -1.0;
    data_t offset_value = offset_side * r * sqrt(1 + pow(a1, 2));
    b1 += offset_value;
    b2 += offset_value;

    data_t xd = 0, yd = 0;

    // intersection
    if(fabs(a1 - a2) > 1e-8){

      xd = (b2 - b1) / (a1 - a2);
      yd = a1 * xd + b1;

    } else{ // no intersection, parallel lines

      Point normal(-v1.y(), v1.x());
      if (p->_trc_type == TRCType::RIGHT)
        normal.scale(-1);

      normal.scale(r);  // offset direction by tool radius
      xd = (p -> target() + normal).x();
      yd = (p -> target() + normal).y();

      cerr << xd << endl;
      cerr << yd << endl;
    }
    

    p -> update_target(xd, yd);
    cerr << style::italic << "New target from TRC while angle < PI: " << endl <<  p -> desc() << style::reset << endl << endl;

  } else{


    Point vec = tp.delta(sp);
    vec.scale(1 / vec.length());
    Point normal(-vec.y(), vec.x(), 0.0);
    if(p -> _trc_type == TRCType::RIGHT){

      normal.scale(-1);
    }

    normal.scale(r);

    cerr << p -> target().desc();

    if (!p->target().is_complete()) {
      throw CNCError("Target point incomplete", this);
      return;
    }

    data_t xd = (p -> target() + normal).x();
    data_t yd = (p -> target() + normal).y();
    cerr << xd << " " << yd << endl;

    p -> update_target(xd, yd); 
    cerr << style::italic << "New target from TRC while angle > PI: " << endl <<  p -> desc() << style::reset << endl << endl;
 
  }
}


void BlockTRC::line_arc_shift(BlockTRC *p){
  Point sp = p -> start_point();
  Point tp = p -> target();
  data_t r = _machine -> machine_tool_radius(); 

  // TODO : when angle > pi, then offset it's enough, no intersections because there is arc_shaping

  cerr << style::italic << "Starting TRC line-arc between: " << endl << style::reset << this -> desc();
  cerr << style::italic << "And previous move: " << style::reset << endl;
  cerr << style::italic << p -> desc() << style::reset << endl;

  Point v = tp.delta(sp);
  v.scale(1 / v.length());

  Point normal(-v.y(), v.x());
  if(p -> _trc_type == TRCType::RIGHT)
    normal.scale(-1);

  normal.scale(r);
  Point offset = tp + normal;             // target offset by its normal * radius

  data_t comp_r = _r + r;                 // compensated radius
  Point v2 = offset - _center;            // vector from the center to the offset target of the previous move

  data_t dist = pow(v2.x(), 2) + pow(v2.y(), 2);
  if(dist <= pow(comp_r, 2)){

    // report here calculus from sheets, first conclusive idea
    data_t scaling = sqrt(pow(comp_r, 2) / dist - 1);
    Point intersection1 = _center + Point(v2.x() - scaling * v2.y(), v2.y() + scaling * v2.x());
    Point intersection2 = _center + Point(v2.x() + scaling * v2.y(), v2.y() - scaling * v2.x());

    Point closer_approx = (intersection1.delta(tp).length() < intersection2.delta(tp).length()) ? intersection1 : intersection2;

    p -> update_target(closer_approx.x(), closer_approx.y());
  } 

}

string BlockTRC::arc_shaping(Point nominal_start) {

  data_t r = _machine->machine_tool_radius();

  _shaping_required = false;

  const Point p0 = prev->target();
  const Point p1 = target();
  const Point pm = prev->start_point();

  cerr << nominal_start.desc() << "          ";

  // shift current starting point
  Point tmp = p1.delta(nominal_start);
  tmp.scale(1/tmp.length());
  Point normal_tmp(-tmp.y(), tmp.x(), tmp.z());
  if (_trc_type == TRCType::RIGHT)
    normal_tmp.scale(-1);

  normal_tmp.scale(r);
  nominal_start = nominal_start + normal_tmp;

  cerr << nominal_start.desc();

  Point v1 = p0.delta(pm);
  Point v2 = p1.delta(nominal_start);
  v1.scale(1 / v1.length());
  v2.scale(1 / v2.length());

  // bisector = vector that connectes the prev starting point and the actual target. Thanks to that it's possible to find the center of the junction arc
  Point bisector = v1 + v2;
  bisector.scale(1 / bisector.length());

  Point normal(-bisector.y(), bisector.x(), bisector.z());
  if (_trc_type == TRCType::RIGHT) {
    normal.scale(-1);
  }

  normal.scale(r);
  Point arc_center = p0 + normal;

  data_t i = arc_center.x() - p0.x();
  data_t j = arc_center.y() - p0.y();

  // G2 or G3 depending junction curvature side
  data_t side = v1.x() * v2.y() - v1.y() * v2.x();
  int arc_code = (side < 0) ? 02 : 03;

  std::string arc_line = fmt::format(
    "G{:02d} X{:.3f} Y{:.3f} I{:.3f} J{:.3f} F{:.0f}",
    arc_code,
    nominal_start.x(), nominal_start.y(),
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

  int tmp = 0;

  switch(cmd){

  case 'N':
    _n = stoi(arg);             // stoi takes a string/character and it parses it as an integer
    if(prev && _n <= prev -> n())
      throw CNCError("Block number must be increasing: " + to_string(prev -> n()), this);
    break;

  case 'G':

    tmp = stoi(arg);

    if (tmp == 0) {
      _type = BlockType::RAPID;

    } else if (tmp == 1) {
      _type = BlockType::LINE;

    } else if (tmp == 2) {
      _type = BlockType::CWA;

    } else if (tmp == 3) {
      _type = BlockType::CCWA;

    } else if (tmp == 40) {
      _trc_type = TRCType::NONE;

    } else if (tmp == 41) {
      _trc_type = TRCType::LEFT;

    } else if (tmp == 42) {
      _trc_type = TRCType::RIGHT;

    } else {
      throw CNCError("Unknown G code: G" + arg, this);
    }

    break;

/*
    if(static_cast<BlockType>(stoi(arg)) <= BlockType::NO_MOTION){

      _type = static_cast<BlockType>(stoi(arg));
    
    } else if(static_cast<TRCType>(stoi(arg)) >= TRCType::NONE && static_cast<TRCType>(stoi(arg)) <= TRCType::RIGHT){

      _trc_type = static_cast<TRCType>(stoi(arg));
    
    } else{

      throw CNCError("Unknown G type", this);
    }

    break;
    */

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
  if (!prev)
    return 0.0;

  Point p0 = this -> prev -> start_point();
  Point p1 = this -> prev -> target();
  Point p2 = this -> target();

  Point v1 = p1.delta(p0);
  Point v2 = p2.delta(p1);

  data_t tmp1 = v1.x() * v2.x();
  data_t tmp2 = v1.y() * v2.y();

  data_t cross = v1.x() * v2.y() - v1.y() * v2.x();
  data_t omega = atan2(cross, tmp1 + tmp2); 

  cerr << "omega: " << omega << endl;

  return omega;

}

bool BlockTRC::is_shaping_needed(){
  if(angle_with_prev() > 0 && _trc_type == TRCType::LEFT){

    return true;
  } else if (angle_with_prev() > 0 && _trc_type == TRCType::RIGHT){

    return false;
  } else if (angle_with_prev() < 0 && _trc_type == TRCType::LEFT){

    return false;
  } else if (angle_with_prev() < 0 && _trc_type == TRCType::RIGHT){

    return true;
  } else{

    return false;
  }
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





