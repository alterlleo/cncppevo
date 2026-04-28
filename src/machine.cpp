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
#include <iostream>
#include <rang.hpp>

using namespace rang;

namespace cncpp{

  Machine::Machine(const string &settings_file, Mads::Agent *agent) : _settings_file(settings_file), _agent(agent){ 

    load(settings_file);
    cout << "loaded yml" << endl;

    _selected_tool = static_cast<int>(ToolType::TOOL_1);
  }

  Machine::~Machine(){

    cout << "Machine disconnected" << endl;
  }

  void Machine::load(const string &s){
    _settings_file = s;
    
    auto data = YAML::LoadFile(s);      // data now is an object which has all the methosds and attributes that can be used in order to access to all yaml fields

    auto machine = data["machine"];
    _A         = machine["A"].as<data_t>();
    _tq        = machine["tq"].as<data_t>();
    _max_error = machine["max_error"].as<data_t>();
    _fmax      = machine["fmax"].as<data_t>();
    _zero      = Point(machine["zero"][0].as<data_t>(), machine["zero"][1].as<data_t>(), machine["zero"][2].as<data_t>(), machine["zero"][3].as<data_t>(), machine["zero"][4].as<data_t>());
    _offset    = Point(machine["offset"][0].as<data_t>(), machine["offset"][1].as<data_t>(), machine["offset"][2].as<data_t>(), machine["offset"][3].as<data_t>(), machine["offset"][4].as<data_t>());
    
    // Adding tools parameters
    _tools.emplace_back(machine["tools"][0].as<data_t>());
    _tools.emplace_back(machine["tools"][1].as<data_t>());
    _tools.emplace_back(machine["tools"][2].as<data_t>());
    _tools.emplace_back(machine["tools"][3].as<data_t>());


  }

  void Machine::selected_tool(size_t t){

    cout << t << endl;
    if(t > 0 && t <= 4){

      t--;
      _selected_tool = t;
    } else{

      throw CNCError("Required tool is not available", this);
    }
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

void Machine::feedback(const json input)  {

  json j = input;
  
  _position = Point(j.value<data_t>("xf", 0), j.value<data_t>("yf", 0), j.value<data_t>("zf", 0));
  _error = j.value<data_t>("error", INFINITY);
}

void Machine::sync(bool rapid){ // synchronize the machine with the current values
  Point pos = (_setpoint + _offset);
  json j;
  j["fmu_input"]["x"] = pos.x();
  j["fmu_input"]["y"] = pos.y();
  j["fmu_input"]["z"] = pos.z();
  j["fmu_input"]["a"] = pos.a();
  j["fmu_input"]["c"] = pos.c();

  // output vx and vy
  j["fmu_input"]["vx"] = _vx;
  j["fmu_input"]["vy"] = _vy;
  // j["rapid"] = rapid;         // flag in order to tell if the movement is rapid or not

  _agent -> publish(j);
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
#include <csignal>
#include <chrono>
#include <thread>
using namespace std;

bool Running = true;

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <settings_file.yml>" << endl;
    return 1;
  }

  /*
  cncpp::Machine machine(argv[1]);
  cout << machine.desc() << endl;
  
  cncpp::Machine default_machine = cncpp::Machine();
  cout << "Default machine:" << endl;
  cout << default_machine.desc() << endl;
  default_machine.load(argv[1]);
  cout << "Default machine after loading:" << endl;
  cout << default_machine.desc() << endl;

  // TEST MQTT communicatino -> go to goodies folder and launch with ./broker_start 
  // high priority message that comes from the kernel -> let's deal the signal, because if it not managed, ctrl+c immediately kills the process, so let's do something more clean.
  // SIGINT corresponds to ctr+c
  // the function used in signal can only access global variables, in this case "running"
  signal(SIGINT, [](int s) { Running = false; });

  // in this way we cleanly exit the loop

  machine.setpoint(0, 0, 0);
  machine.connect();
  machine.listen_start();

  // we want to stop the program when using ctrl+c


  while(Running) {
    this_thread::sleep_for(chrono::seconds(1));
    machine.sync(false);        // synchronize the local machine with the remote machine through MQTT
    cout << "Error: " << machine.error() << endl;
  }
  machine.listen_stop();


*/
  return 0;
}

#endif

/*

runna da una finestra con install

usr/bin/mosquitto_pub -t cnc/status    is the topic

usr/bin/mosquitto_pub -t cnc/status -m 'hello' -> we get an error because we are not sending a json object

usr/bin/mosquitto_pub -t cnc/status -m '{"error":0.5}' 

the machin is running in meters and the controller in millimiters -> if we send error 2 the machine will se 2000
But the ocnversion give us an integer but we need also float, so modify it as j.value<data_t>("error", 0) * 1000
So you can send also 0.5

*/