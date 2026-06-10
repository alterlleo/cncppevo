/*
  ____                                             _               
 |  _ \ _ __ ___   __ _ _ __ __ _ _ __ ___     ___| | __ _ ___ ___ 
 | |_) | '__/ _ \ / _` | '__/ _` | '_ ` _ \   / __| |/ _` / __/ __|
 |  __/| | | (_) | (_| | | | (_| | | | | | | | (__| | (_| \__ \__ \
 |_|   |_|  \___/ \__, |_|  \__,_|_| |_| |_|  \___|_|\__,_|___/___/
                  |___/                                            
*/

#include "program.hpp"
#include <rang.hpp>
#include <fmt/core.h>
#include <fstream>
#include <sstream>

using namespace cncpp;
using namespace std;
using namespace rang;
using namespace fmt;

Program::Program(const std::string &f, Machine *m) : _filename(f), _machine(m) {
  load(_filename);
}

Program::~Program(){

  if(_debug)
    cerr << style::italic << format("Destroying program {:} with {:} blocks", _filename, size()) << style::reset << endl;

  /*
  for(auto b : *this){
    delete b;

  }
  */

  clear();
}

std::string Program::desc(bool colored) const{
  
  ostringstream ss;     // output stringstream

  for(auto &b : *this){ // we loop all the blocks inside the current program object

    ss << b.desc();
    ss << format(", previous: {:0>3}", b.prev ? b.prev -> n() : 0);
    ss << endl;

  }

  return ss.str();
}

void Program::load(const string &f, bool append){
  _filename = f;

  // open the file and create a new block with it and add the block to the list
  ifstream file(_filename);
  if( !file.is_open()){
    throw runtime_error("Could not open the file " + _filename);
  }

  if( !append)
    reset();

  // variable representing the current line
  string line;
  while(getline(file, line)){     // we load a line each iteration

    if(line[0] == '#'){           // if there is some empty line just jump it over
      continue;
    }

    *this << line;
    back().desc();

    Point nominal_start;

    if(back().prev && this -> size() > 1){

      BlockTRC last = back();
      nominal_start = last.start_point();
      
      last.shift_prev_target();

      if(last.prev && last.prev -> trc() && last.shaping()){
        
        list<BlockTRC>::iterator iter = end();
        --iter;

        if(1/*!((last -> prev -> type() == Block::BlockType::CCWA || last -> prev -> type() == Block::BlockType::CWA) && (last -> type() == Block::BlockType::CCWA || last -> type() == Block::BlockType::CWA))*/){

          string arc = last.arc_shaping(nominal_start);
          cerr << arc << endl;
          
          BlockTRC *second_to_last = last.prev;

          BlockTRC *corner = new BlockTRC(arc, second_to_last);

          last.prev = corner;
          corner -> next = &last;
          second_to_last -> next = corner;

          this -> insert(iter, *corner);
          corner -> parse(_machine);
        }

      }

    }
  }

  file.close();
}

data_t Program::compute_limit_v(BlockTRC prev, BlockTRC current){


  if (prev.type() == Block::BlockType::NO_MOTION || 
      current.type() == Block::BlockType::NO_MOTION) return 0.0;

  // directions
  std::vector<data_t> d1 = prev.delta().vec();
  std::vector<data_t> d2 = current.delta().vec();
  
  data_t l1 = prev.length();
  data_t l2 = current.length();

  if (l1 == 0 || l2 == 0) return 0.0;

  // scalar product
  data_t dot_product = 0;
  for (size_t i = 0; i < 3; ++i) { // only x y z

      dot_product += (d1[i] / l1) * (d2[i] / l2);
  }

  dot_product = std::max(-1.0, std::min(1.0, dot_product));
  data_t theta = acos(dot_product); 

  // if theta is small enough, let's consider the nominal feedrate
  if (theta < 0.001) return std::min(prev.feedrate(), current.feedrate()) / 60.0;

  data_t accel = std::min(prev.a(), current.a());
  data_t tolerance = 0.01; 


  data_t v_junction = sqrt((accel * tolerance) / tan(theta / 2.0));

  data_t v_max = std::min(prev.feedrate(), current.feedrate()) / 60.0;
  
  return std::min(v_junction, v_max);
}


Program &Program::operator<<(string line){
  if(size() > 0){

    // the current program is not empty
    emplace_back(line, back()); // it's the same as -> emplace_back(Block(line, back()));

  } else{

    emplace_back(line);

  }

  BlockTRC& current = back();
  current.parse(_machine);

  // rise error if A and C fields are present together with movement axes
  if(size() > 1){

    BlockTRC* prev = current.prev;

    if(current.type() != BlockTRC::BlockType::RAPID && (current.target().a() != prev -> target().a() || current.target().c() != prev -> target().c()) && (current.target().x() != prev -> target().x() || current.target().y() != prev -> target().y())){

      throw CNCError("Invalid G-Code command: A and C are not movement axes, they must be sliced as positioning axes", this);
    }

    data_t v_junction = compute_limit_v(*prev, current);

    prev -> set_fe(v_junction);
    current.set_fs(v_junction);

    BlockTRC* b = prev;
    int steps = 0;
    int H = 10;

    while(b && b -> prev && steps < H){

      BlockTRC* p = b -> prev;
      data_t max_fe = sqrt(pow(b -> profile().fe, 2) + 2 * b -> a() * b -> length());

      if (p->profile().fe > max_fe) {

        p -> set_fe(max_fe);
        b -> set_fs(max_fe);
        p -> compute(); 
        b -> compute();

      } else {
        break; 
      }

      b = p;
      steps++;
    }
  }
  return *this;
}


/*
  _____         _                     _       
 |_   _|__  ___| |_   _ __ ___   __ _(_)_ __  
   | |/ _ \/ __| __| | '_ ` _ \ / _` | | '_ \ 
   | |  __/\__ \ |_  | | | | | | (_| | | | | |
   |_|\___||___/\__| |_| |_| |_|\__,_|_|_| |_|
                                              
*/

#ifdef PROGRAM_MAIN

int main(int argc, const char *argv[]){

  if(argc < 2){ // if we don't have enough arguments
    cerr << "Usage: " << argv[0] << " <file.gcode>" << endl;
    return 1;
  }

  Machine machine;

  try{
    machine.load("machine.yml");

  } catch (exception &e){

    cerr << fg::red << style::bold << "Error: " << e.what() << style::reset << fg::reset << endl;
    return 1;
  }

  // if the machine is loaded, then we can load the prorgam
  Program program(&machine);

  try{

    program.load(argv[1]);    // the argument must be passed by command line

  } catch(exception &e){

    cerr << fg::red << style::bold << "Error: " << e.what() << style::reset << fg::reset << endl;
    return 2;
  }

  // try to program << "some line";
  // maybe with a safe position

  cout << program.desc() << endl;

  // test concatenating other lines 
  try {
      program << "N10 G0 X0 Y0 Z0"
              << "N15 M30";
      program << "N20 G1 X10 Y10 Z10 F1000";
      program << "N30 G2 X20 Y20 Z20 I10 J10 K30 F500";
    } catch (CNCError &e) {
      cerr << fg::red << style::bold << "Error: " << e.what() << " in " << e.who()
          << fg::reset << style::reset << endl;
    }

  cout << program.desc(false) << endl;

  for (auto &b : program) {
    cout << b -> n() << ": length " << b -> length() << endl;
  }

  // reset
  program.reset();
  cout << "Program after reset: " << program.desc() << endl;

  return 0;
}

#endif
