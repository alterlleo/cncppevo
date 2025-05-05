/*

  __  __            _     _                   _               
 |  \/  | __ _  ___| |__ (_)_ __   ___    ___| | __ _ ___ ___ 
 | |\/| |/ _` |/ __| '_ \| | '_ \ / _ \  / __| |/ _` / __/ __|
 | |  | | (_| | (__| | | | | | | |  __/ | (__| | (_| \__ \__ \
 |_|  |_|\__,_|\___|_| |_|_|_| |_|\___|  \___|_|\__,_|___/___/

class implementation                                                        
*/

#include "machine.hpp"
#include <yaml-cpp/yaml.h>
#include <sstream>

namespace cncpp{

  Machine::Machine(const string &settings_file) : _settings_file(settings_file){ 

    load(settings_file);
  }


  void Machine::load(const string &s){
    _settings_file = s;
    
    auto data = YAML::LoadFile(s);      // data now is an object which has all the methosds and attributes that can be used in order to access to all yaml fields

    auto machine = data["machine"];
    _A         = machine["A"].as<data_t>();
    _tq        = machine["tq"].as<data_t>();
    _max_error = machine["max_error"].as<data_t>();
    _fmax      = machine["fmax"].as<data_t>();
    _zero      = Point(machine["zero"][0].as<data_t>(), machine["zero"][1].as<data_t>(), machine["zero"][2].as<data_t>());
    _offset    = Point(machine["offset"][0].as<data_t>(), machine["offset"][1].as<data_t>(), machine["offset"][2].as<data_t>());

  }

  string Machine::desc(bool colored) const{

    stringstream ss;

    ss << "A = " << _A << ", ";
    ss << "tq = " << _tq << ", ";
    ss << "max_error = " << _max_error << ", ";
    ss << "fmax = " << _fmax << ", " << endl;

    ss << "zero = " << _zero.desc(colored) << endl;
    ss << "offset = " << _offset.desc(colored) << endl;

    return ss.str();
  }


  data_t Machine::quantize(data_t t, data_t &dq) const{

      data_t q;
      q = static_cast<size_t>((t / _tq) + 1) * _tq;
      dq = q - t;

      return q;
  }


}




/*
  _____         _                     _       
 |_   _|__  ___| |_   _ __ ___   __ _(_)_ __  
   | |/ _ \/ __| __| | '_ ` _ \ / _` | | '_ \ 
   | |  __/\__ \ |_  | | | | | | (_| | | | | |
   |_|\___||___/\__| |_| |_| |_|\__,_|_|_| |_|
                                              
*/

#ifdef MACHINE_MAIN

#include <iostream>
using namespace cncpp;

int main(int argc, const char *argv[]){

  if(argc < 2){ //we need at least 2 arguments, so 

    // argv[0] is the name of the exucatble
    // argv[1] is the first argument of the executable

    cerr << "Usage: " << argv[0] << " <settings_file" << endl;
    return 1;

  }

  cncpp::Machine machine(argv[1]);
  cout << "Inizialized machine: " << endl << machine.desc() << endl;

  cncpp::Machine default_machine = Machine();
  cout << "Default machine: " << endl << default_machine.desc() << endl;

  default_machine.load(argv[1]);
  cout << "Default machine after loading: " << endl << default_machine.desc() << endl;

  return 0;


}


#endif