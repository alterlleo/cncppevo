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

}

std::string Program::desc(bool colored) const{
  
  ostringstream ss;     // output stringstream

  for(auto &b : *this){ // we loop all the blocks inside the current program object

    ss << b -> desc();
    ss << format(", previous: {:0>3}", b -> prev ? b -> prev -> n() : 0);
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
    back() -> desc();

    BlockTRC* pr = dynamic_cast<BlockTRC*>(back()->prev);
    if(pr && pr -> trc()){

      cerr << "check shift" << endl;
      back() -> desc();

      back() -> shift_prev_target();
    }

    if(back() -> shaping()){

      list<BlockTRC*>::iterator iter = prev(this -> end());
      string arc = back() -> arc_shaping();
      
      BlockTRC *pr_tmp = dynamic_cast<BlockTRC*>(back() -> prev);
      this -> insert(iter, new BlockTRC(arc, pr_tmp));
      *this << back() -> arc_shaping();
    }
  }

  file.close();
}

Program &Program::operator<<(string line){
  if(size() > 0){

    // the current program is not empty
    emplace_back(new BlockTRC(line, back())); // it's the same as -> emplace_back(Block(line, back()));

  } else{

    emplace_back(new BlockTRC(line));

  }

  back() -> BlockTRC::parse(_machine);
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
