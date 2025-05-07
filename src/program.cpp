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

using namespace cncpp;
using namespace std;
using namespace rang;
using namespace fmt;

Program::Program(const std::string &f, Machine *m) : _filename(f), _machine(m) {
  load(_filename);
}

Program::~Program(){

  if(~_debug)
    cerr << style::italic << format("Destroying program {:} with {:} blocks", _filename, size()) << style::reset << endl;

}

std::string Program::desc(bool colored) const{
  
}

void Program::load(const std::string &f, bool append){
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

    *this << line;
  }

  file.close();
}

Program &Program::operator<<(string line){
  if(size() > 0){

    // the current program is not empty
    emplace_back(line, back());

  } else{
    emplace_back(line);

  }

  back().parse(_machine);
  return *this;
}


