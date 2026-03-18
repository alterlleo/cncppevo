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

  Machine::Machine(const string &settings_file) : _settings_file(settings_file){ 

    load(settings_file);
    mosqpp::lib_init();

    _selected_tool = static_cast<int>(ToolType::TOOL_1);
  }

  Machine::~Machine(){

    bool res = disconnect();

    if(_connected && res != MOSQ_ERR_SUCCESS){
      cerr << fg::red << "Cannot disconnect from MQTT broker " << fg::reset << endl;
    }
    mosqpp::lib_cleanup();
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
    _offset    = Point(machine["offset"][0].as<data_t>(), machine["offset"][1].as<data_t>(), machine["offset"][2].as<data_t>(), machine["zero"][3].as<data_t>(), machine["zero"][4].as<data_t>());
    
    // Adding tools parameters
    _tools.emplace_back(machine["tools"][0].as<data_t>());
    _tools.emplace_back(machine["tools"][1].as<data_t>());
    _tools.emplace_back(machine["tools"][2].as<data_t>());
    _tools.emplace_back(machine["tools"][3].as<data_t>());

    //MQTT parameters from yml file
    _mqtt_host = machine["mqtt"]["host"].as<string>("localhost");    // inside () there is the default
    _mqtt_port = machine["mqtt"]["port"].as<int>(1883);
    _mqtt_keepalive = machine["mqtt"]["keepalive"].as<int>(60);
    _pub_topic = machine["mqtt"]["topics"]["pub"].as<string>("cnc/setpoint");
    _sub_topic = machine["mqtt"]["topics"]["sub"].as<string>("cnc/status/#");  // # is a wildcard

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
    ss << "MQTT host = " << mqtt_host() << endl;

    return ss.str();
  }

  data_t Machine::quantize(data_t t, data_t &dq) const{

      data_t q;
      q = static_cast<size_t>((t / _tq) + 1) * _tq;
      dq = q - t;

      return q;
  }

  int Machine::connect(){
    // call the mosquittopp method
    int rc = mosquittopp::connect(_mqtt_host.c_str(), _mqtt_port, _mqtt_keepalive);
    if(rc != MOSQ_ERR_SUCCESS){
      
      throw CNCError("Cannot connect to MQTT broker ", this);
    }

    return rc;
  }

  void Machine::listen_start(){

    if(subscribe(NULL, _sub_topic.c_str()) != MOSQ_ERR_SUCCESS){
      
      throw CNCError("Cannot subscribe to topic " + _sub_topic, this);
    }
  }

  void Machine::listen_stop(){

    if(unsubscribe(NULL, _sub_topic.c_str()) != MOSQ_ERR_SUCCESS){
      
      throw CNCError("Cannot unsubscribe to topic " + _sub_topic, this);
    }

  }

  void Machine::on_connect(int rc){

    if(_debug){
      cerr << fg::yellow << style::italic << "Connected to broker " << mqtt_host() << fg::reset << endl;
    }

    _position.reset();
    _error = INFINITY;
    _connected = true;
  }

  void Machine::on_disconnect(int rc){

    if(_debug){
        cerr << fg::yellow << style::italic << "Disconnected to broker " << mqtt_host() << fg::reset << endl;
    }

    _connected = false;
  }

  void Machine::on_subscribe(int mid, int qos_count, const int *qos){

    if(_debug){
        cerr << fg::yellow << style::italic << "Subscribed to topic " << mqtt_host() << fg::reset << endl;
    }

    _position.reset();
    _error = INFINITY;
  }

  void Machine::on_unsubscribe(int mid){

    if(_debug){
        cerr << fg::yellow << style::italic << "Unsubscribed from topic " << mqtt_host() << fg::reset << endl;
    }

    _position.reset();
    _error = INFINITY;

  }

void Machine::on_message(const struct mosquitto_message *message)  {
  string payload((char *)message->payload, message->payloadlen);
  json j;

  try {
    j = json::parse(payload);

  } catch (json::parse_error &e) {

    cerr << fg::red << "Cannot parse JSON payload: " << e.what() << endl
         << "Payload was: " << style::bold << payload
         << style::reset << fg::reset << endl;

    return;
  }
  
  _position = Point(j.value<data_t>("x", 0)*1000, j.value<data_t>("y", 0) * 1000, j.value<data_t>("z", 0) * 1000);
  _error = j.value<data_t>("error", 0) * 1000;
}


  void Machine::sync(bool rapid){ // synchronize the machine with the current values
    Point pos = (_setpoint + _offset);
    json j;
    j["x"] = pos.x();
    j["y"] = pos.y();
    j["z"] = pos.z();
    j["rapid"] = rapid;         // flag in order to tell if the movement is rapid or not
    string payload = j.dump();  // dump method convert a json structure into a string
    int rc = publish(NULL, _pub_topic.c_str(), payload.length(), payload.c_str(), 0, false);
    if (rc != MOSQ_ERR_SUCCESS) {
      throw CNCError("Cannot publish to topic " + _pub_topic, this);
    }
    
    loop();                     // every sync call the mqtt communication updates thanks to loop() function

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