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

BlockTRC::BlockTRC(string line, BlockTRC &prev) : Block(line, prev), _trc(prev.trc()), _trc_type(prev._trc_type) {}

BlockTRC::BlockTRC(string line, BlockTRC *b) : Block(line, b), _trc(b -> trc()), _trc_type(dynamic_cast<BlockTRC*>(prev) -> _trc_type) {}

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

  if(p -> trc()){
    
    if (p -> type() == BlockType::LINE && type() == BlockType::LINE) {

      line_line_shift(p);

    } else if (p -> type() == BlockType::LINE && (type() == BlockType::CWA || type() == BlockType::CCWA)) {
      
      cerr << "check entering into line_arc" << endl;
      line_arc_shift(p);
    
    } else if ((p -> type() == BlockType::CWA || p -> type() == BlockType::CCWA ) && type() == BlockType::LINE){

      cerr << "\n\n cehck entering arc_line" << endl;
      arc_line_shift(p);

    } else if ((p -> type() == BlockType::CWA || p -> type() == BlockType::CCWA ) && (type() == BlockType::CWA || type() == BlockType::CCWA )){

      arc_arc_shift(p);
    }

  }

  p -> _delta = p -> _target.delta(p -> start_point());
  p -> compute();

}

void BlockTRC::line_line_shift(BlockTRC *prev){

  BlockTRC *p = prev;

  if(!p){
    throw CNCError("Null previous block pointer ", this);
    return;
  }

  Point sp = p -> start_point();                // starting point of the previous block
  Point tp = p -> target();                     // target point of the previous block
  Point sc = tp;                                // starting point of the current block
  Point tc = target();                          // target point of the current block
  data_t r = _machine -> machine_tool_radius(); // current tool radius

  cerr << style::italic << "Starting TRC line-line between: " << endl << style::reset << this -> desc() << endl; 
  cerr << style::italic << "And previous move: " << style::reset << endl;
  cerr << style::italic << p -> desc() << style::reset << endl << endl;

  if(!is_shaping_needed()){

    cerr << "check not shaping" << endl;

    Point v1 = tp.delta(sp);
    Point v2 = tc.delta(sc);
    v1.scale(1 / v1.length());
    v2.scale(1 / v2.length());

    // find line parameters
    data_t offset_side = (p -> _trc_type == TRCType::LEFT) ? 1.0 : -1.0;
    data_t xd = 0, yd = 0;

    // previous line is vertical
    if(v1.x() == 0 || v2.x() == 0){

      Point v2_norm(-v2.y(), v2.x(), v2.z());
      v2_norm.scale(1 / v2_norm.length());

      if(v1.x() == 0){
        
        data_t a2 = v2.y() / v2.x();
        data_t b2 = tc.y() - a2 * tc.x();
        data_t offset_value2 = offset_side * r * sqrt(1 + pow(a2, 2));
        offset_value2 = (tc.x() > tp.x()) ? -offset_value2 : offset_value2;
        b2 += offset_value2;

        xd = -offset_side * r + sp.x();
        yd = a2 * xd + b2;

      } else if(v2.x() == 0){

        data_t a1 = v1.y() / v1.x();
        data_t b1 = tp.y() - a1 * tp.x();

        data_t offset_value1 = offset_side * r * sqrt(1 + pow(a1, 2));
        offset_value1 = (sp.x() > sc.x()) ? -offset_value1 : offset_value1;
        
        b1 += offset_value1;

        xd = -offset_side * r + sc.x();
        yd = a1 * xd + b1;
      
      }  
    
    } else{

      data_t a1 = v1.y() / v1.x();
      data_t a2 = v2.y() / v2.x();
      data_t b1 = tp.y() - a1 * tp.x();
      data_t b2 = tc.y() - a2 * tc.x();

      offset_side = (a1 < 0) ? -offset_side : offset_side;

      data_t offset_value1 = offset_side * r * sqrt(1 + pow(a1, 2));
      data_t offset_value2 = offset_side * r * sqrt(1 + pow(a2, 2));

      b1 += offset_value1;
      b2 += offset_value2;

      // intersection

      if(fabs(a1 - a2) > 0){

        xd = (b2 - b1) / (a1 - a2);
        yd = (b1 * a2 - b2 * a1) / (a2 - a1);

      } else{ // no intersection, parallel lines

        Point normal(-v1.y(), v1.x(), v1.z());
        if (p->_trc_type == TRCType::RIGHT)
          normal.scale(-1);

        normal.scale(r);  // offset direction by tool radius
        xd = (p -> target() + normal).x();
        yd = (p -> target() + normal).y();

      }
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

    if (!p->target().is_complete()) {
      throw CNCError("Target point incomplete", this);
      return;
    }

    // if the junction arc is required, then just shift normally the target
    data_t xd = (p -> target() + normal).x();
    data_t yd = (p -> target() + normal).y();
    cerr << xd << " " << yd << endl;

    p -> update_target(xd, yd); 
    cerr << style::italic << "New target from TRC while angle > PI: " << endl <<  p -> desc() << style::reset << endl << endl;
 
  }

  // update delta without reparsing
  // p -> _delta = p -> _target.delta(p -> start_point());
}


void BlockTRC::line_arc_shift(BlockTRC *p){

  cerr << style::italic << "Starting TRC line-arc between: " << endl << style::reset << this -> desc() << endl;
  cerr << style::italic << "And previous move: " << style::reset << endl;
  cerr << style::italic << p -> desc() << style::reset << endl;

  data_t side = (p -> _trc_type == TRCType::RIGHT) ? 1 : -1;
  side = (type() == BlockType::CCWA) ? side : -side;

  // intersection declaration
  Point i;

  Point sp = p -> start_point();
  Point tp = p -> target();
  data_t r = _machine -> machine_tool_radius();

  cerr << sp.x() << " " << tp.x() << endl;

  if(!(fabs(sp.x() - tp.x() - r) < 0.000001)){

    cerr << "check check check check" << endl;

    Point vec = tp.delta(sp);
    vec.scale(1 / vec.length());
    Point normal(-vec.y(), vec.x(), vec.z());
    normal.scale(-side * r);

    cerr << tp.desc() << " ";
    tp = tp + normal;
    cerr << tp.desc() << endl;
  

    if(!(dynamic_cast<BlockTRC*>(p -> prev) -> trc()))
      sp = sp + normal;

    data_t m = (tp.y() - sp.y()) / (tp.x() - sp.x());
    data_t h = sp.y() - (m * sp.x());

    i = line_circle_intersection(_center, _r + side * r, m, h, tp);

  } else{ // vertical line

  cerr << "check check" << endl;

    data_t ix = sp.x();

    data_t delta = pow(_r + side*r, 2) - pow(ix - _center.x(), 2);
    if(delta < 0)
      throw CNCError("No interseciton between line and arc", this);
 
    data_t iy1 = _center.y() - sqrt(delta);
    data_t iy2 = _center.y() + sqrt(delta);

    Point i1(ix, iy1, tp.z());
    Point i2(ix, iy2, tp.z());

    i = (fabs(i1.delta(tp).length()) < fabs(i2.delta(tp).length())) ? i1 : i2;
  }

  cerr << "previous target: " << p -> target().desc();
  p -> update_target(i.x(), i.y());
  cerr << "upadted target: " << p -> target().desc();
  p -> _delta = p -> _target.delta(p -> start_point());
  set_r(_r + side * r);
}

void BlockTRC::arc_line_shift(BlockTRC *p){

  cerr << style::italic << "Starting TRC arc-line between: " << endl << style::reset << this -> desc();
  cerr << style::italic << "And previous move: " << style::reset << endl;
  cerr << style::italic << p -> desc() << style::reset << endl;

  data_t r = _machine -> machine_tool_radius(); 
  Point tp = p -> target();

  data_t side = (p -> _trc_type == TRCType::RIGHT) ? 1 : -1;
  side = (p -> type() == BlockType::CCWA) ? side : -side;

  Point i;

  if(!(fabs(start_point().x() - target().x()) < 0.000001)){

    Point vec = target().delta(start_point());
    vec.scale(1 / vec.length());
    Point normal(-vec.y(), vec.x(), vec.z());
    normal.scale(-side * r);

    Point sc = start_point() + normal;
    Point tc = target() + normal;

    data_t m = (tc.y() - sc.y()) / (tc.x() - sc.x());
    data_t h = sc.y() - m * sc.x();

    i = line_circle_intersection(p -> _center, p -> _r, m, h, p -> target());

  } else{ //vertical line
    data_t ix = start_point().x() + side * r;

    data_t delta = pow(p -> _r, 2) - pow(ix - p -> _center.x(), 2);
    if(delta < 0) 
      throw CNCError("No interseciton between line and arc", this);

    data_t iy1 = p -> _center.y() - sqrt(delta);
    data_t iy2 = p -> _center.y() + sqrt(delta);

    Point i1(ix, iy1);
    Point i2(ix, iy2);

    i = (fabs(i1.delta(p -> target()).length()) < fabs(i2.delta(p -> target()).length())) ? i1 : i2;
    }

  p -> update_target(i.x(), i.y());
  cerr << "new arc target before line: " << p -> target().desc() << "with raidus" << p -> _r << endl;
  p -> _delta = p -> _target.delta(p -> start_point());
  p -> calc_arc();  

}


void BlockTRC::arc_arc_shift(BlockTRC *p){

  cerr << style::italic << "Starting TRC arc-arc between: " << endl << style::reset << this -> desc() << endl;
  cerr << style::italic << "And previous move: " << style::reset << endl;
  cerr << style::italic << p -> desc() << style::reset << endl;

  data_t side = (p -> _trc_type == TRCType::RIGHT) ? 1 : -1;
  side = (p -> type() == BlockType::CCWA) ? side : -side;
  side = (type() == BlockType::CWA) ? -side : side;

  data_t r = _machine -> machine_tool_radius(); 

  Point c1 = p -> _center;
  Point c2 = _center;
  data_t r1 = p -> _r;
  data_t r2 = _r + side * r;
  data_t aa = 2 * c2.x() - 2 * c1.x();
  data_t bb = 2 * c2.y() - 2 * c1.y();
  data_t cc = pow(r1, 2) - pow(r2, 2) + pow(c2.x(), 2) - pow(c1.x(), 2) + pow(c2.y(), 2) - pow(c1.y(), 2);

  data_t m = -aa / bb;
  data_t h = cc / bb;

  // Point i = circle_circle_intersection(c1, r1, c2, r2, p -> target(), side);
  Point i = line_circle_intersection(c1, r1, m, h, p -> target());
  data_t ix = i.x();
  data_t iy = i.y();

  p -> update_target(ix, iy);
  cerr << "arc arc intersection: " << i.desc() << endl;
  p -> _delta = p -> _target.delta(p -> start_point());
  set_r(r2);

  if(!((type() == BlockType::CCWA && p -> type() == BlockType::CWA) || (p -> type() == BlockType::CCWA && type() == BlockType::CWA)))
    p -> calc_arc();

}

Point BlockTRC::line_circle_intersection(Point cen, data_t r, data_t m, data_t h, Point tp){

  data_t ix, iy;

  data_t a = pow(m, 2) + 1;
  data_t b = -2 * cen.x() + 2 * m * (h - cen.y());
  data_t c = pow(cen.x(), 2) + pow(h - cen.y(), 2) - pow(r, 2);

  data_t delta = pow(b, 2) - 4 * a * c;
  cerr << "m:" << m << " h:" << h << " center in:" << cen.desc() << " rad:" << r << endl;

  if(delta < 0){

    throw CNCError("No line-arc intersection", this);
  }
    

  data_t x1 = (-b - sqrt(delta)) / (2 * a);
  data_t x2 = (-b + sqrt(delta)) / (2 * a);
  
  Point tmp1(x1, m * x1 + h, tp.z());
  Point tmp2(x2, m * x2 + h, tp.z());

  if(tmp1.delta(tp).length() < tmp2.delta(tp).length()){
    ix = tmp1.x();
    iy = tmp1.y();

  } else{
    ix = tmp2.x();
    iy = tmp2.y();
  }

  return Point(ix, iy, tp.z());
}


string BlockTRC::arc_shaping(Point nominal_start) {

  data_t r = _machine -> machine_tool_radius();

  _shaping_required = false;

  if(type() == BlockType::LINE && prev -> type() == BlockType::LINE){

    cerr << "SHAPING BETWEEN ARC AND LINE" << endl;
    
  }

  const Point p0 = prev -> target();
  const Point p1 = target();
  const Point pm = prev -> start_point();

  Point arc_center = nominal_start;

  Point tmp = p1.delta(nominal_start);
  cerr << "delta: " << tmp.desc() << endl;
  tmp.scale(1/tmp.length());
  Point normal_tmp(-tmp.y(), tmp.x(), tmp.z());
  if (dynamic_cast<BlockTRC*>(prev) -> _trc_type == TRCType::RIGHT)
    normal_tmp.scale(-1);

  normal_tmp.scale(r);
  nominal_start = nominal_start + normal_tmp;
  cerr << "\t \t new nominal start: " << nominal_start.desc() << endl;

  Point v1 = p0.delta(pm);
  Point v2 = p1.delta(nominal_start);
  v1.scale(1 / v1.length());
  v2.scale(1 / v2.length());

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

  cerr << "junction arc: " << arc_line << endl;
  return arc_line;
}

/*
  ____       _            _                        _   _               _     
 |  _ \ _ __(_)_   ____ _| |_ ___   _ __ ___   ___| |_| |__   ___   __| |___ 
 | |_) | '__| \ \ / / _` | __/ _ \ | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
 |  __/| |  | |\ V / (_| | ||  __/ | | | | | |  __/ |_| | | | (_) | (_| \__ \
 |_|   |_|  |_| \_/ \__,_|\__\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                             
*/

bool BlockTRC::parse_token(string token){
    // we want to support both capital and lower cases, let's put all capital
  char cmd = toupper(token[0]);   // token[0] is the first letter in the token

  string arg = token.substr(1);   // sub string that starts at the element

  if(cmd == '#' || cmd == ';')
    return false;

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

  return true;
}


data_t BlockTRC::angle_with_prev(){
  if (!prev)
    return 0.0;

  Point p0 = prev -> start_point();
  Point p1 = start_point();
  Point p2 = target();

  Point v1 = p1.delta(p0);
  Point v2 = p2.delta(p1);

  data_t tmp1 = v1.x() * v2.x();
  data_t tmp2 = v1.y() * v2.y();

  data_t cross = v1.x() * v2.y() - v1.y() * v2.x();
  data_t omega = atan2(cross, tmp1 + tmp2); 

  cerr << "\t angle with prev: " << omega << endl;

  return omega;

}

bool BlockTRC::is_shaping_needed(){

  if((prev -> type() == BlockType::CCWA && type() == BlockType::LINE) || (prev -> type() == BlockType::CWA && type() == BlockType::LINE)){

    if(fabs(angle_with_prev()) < PI / 2)
      return true;

    return false;
  }

  if(angle_with_prev() > 0 && _trc_type == TRCType::RIGHT){

    return true;

  } else if (angle_with_prev() < 0 && _trc_type == TRCType::LEFT){

    return true;

  } else{

    return false;
  }

  return false;
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





