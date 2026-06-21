#include <mads.hpp>
#include <agent.hpp>
#include <thread>
#include <chrono>
#include <filesystem>
// #include <nlohmann/json.hpp>
#include "../cncpp.hpp"
#include "../fsm.hpp"
#include "../timer.hpp"
#include <iostream>
#include <rang.hpp>

using namespace chrono_literals;
using json = nlohmann::json;

using namespace std;
using namespace rang;
using namespace cncpp;
using namespace chrono;

using double_d = std::chrono::duration<double>;

struct FsmData {
  std::unique_ptr<Mads::Agent> agent;

  // cncpp fields
  Program program;
  Machine machine;
  unique_ptr<Timer<double_d, false>> timer;
  data_t t_tot, t_blk;
  FsmData(string name, string settings, string yaml) 
    : agent(std::make_unique<Mads::Agent>(name, settings)), 
      machine(yaml, agent.get()), 
      program(&machine) {}

};

int main(int argc, char *argv[]) {

  if(argc != 3){

    cerr << "Usage: " << argv[0] << " <machine.yml> <program.gcpde" << endl;

    return 1;
  }

  std::filesystem::path exec = argv[0];
  std::string agent_name = exec.stem().string();
  std::string settings_path = "tcp://localhost:9092";
  std::chrono::duration loop_period = 1ms;
  std::chrono::duration receive_timeout = 50ms;
  bool non_blocking = false;

  FsmData data(agent_name, settings_path, argv[1]); // argv[1] is machine.yml path
  // If crypto is needed, properly load keys and enable it
  data.agent->init(false, false);
  data.agent->enable_remote_control();
  data.agent->connect();

  auto settings = data.agent->get_settings();
  if (settings.contains("period")) {
    loop_period = std::chrono::milliseconds(settings["period"].get<int>());
  }
  if (settings.contains("receive_timeout")) {
    receive_timeout = std::chrono::milliseconds(settings["receive_timeout"].get<int>());
  }
  if (settings.contains("non_blocking")) {
    non_blocking = settings["non_blocking"].get<bool>();
  }
  if (settings.contains("sim")){
    data.machine.is_sim(settings["sim"].get<bool>());
  }
  data.agent->set_receive_timeout(receive_timeout);
  // Deal with further settings as needed
  // Initialize FSM
  auto fsm = FiniteStateMachine(&data);

  // load gcode
  try{
    data.program.load(argv[2]);
  } catch(exception &e){
    cerr << fg::red << style::bold << "Error: " << e.what() << style::reset << fg::reset << endl;
    return 2;
  }

  data.timer = std::make_unique<Timer<double_d, false>>(loop_period, loop_period * 1.5);
  data.timer->start();

  fsm.set_timing_function([&]() {
    try {
      data.timer->wait_throw();
    } catch (const TimerError &e) {
      cerr << "Timer error: " << e.what() << endl;
    }
  });

  fsm.run([&](FsmData &s) {

    data.agent->receive(non_blocking);
    
    auto msg = data.agent->last_message();
    std::string topic = std::get<0>(msg);
    std::string payload = std::get<1>(msg);

    //cout << __LINE__ << endl;
    // data.agent->remote_control(payload);
    //cout << __LINE__ << endl;

    if (data.machine.listening() && !payload.empty() && (topic == "fmu" || topic == "machine")) {
      try {
        auto in = json::parse(payload);
        
        if (in.contains("output") && in["output"].is_object()) {
          data.machine.feedback(in["output"]);
        }
      } catch (const json::exception& e) {
        cerr << fg::yellow << "JSON Parsing Error: " << e.what() << style::reset << endl;
      }
    }
  });

  // Shutdown procedure
  data.agent->register_event(Mads::event_type::shutdown);
  data.agent->disconnect();

  if (data.agent->restart()) {
    auto cmd = string(MADS_PREFIX) + argv[0];
    cout << "Restarting " << cmd << "..." << endl;
    execvp(cmd.c_str(), argv);
  }
  return 0;
}