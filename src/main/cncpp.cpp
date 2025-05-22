/*
   ____ _   _  ____ ____  ____                  
  / ___| \ | |/ ___|  _ \|  _ \    _____  _____ 
 | |   |  \| | |   | |_) | |_) |  / _ \ \/ / _ 
 | |___| |\  | |___|  __/|  __/  |  __/>  <  __/
  \____|_| \_|\____|_|   |_|      \___/_/\_\___|

Main executable
*/

#include "../cncpp.hpp"
#include "../fsm.hpp"
#include "../timer.hpp"
#include <iostream>
#include <rang.hpp>
#include <chrono>
#include "../fsm.cpp"

using namespace std;
using namespace rang;
using namespace cncpp;
using namespace chrono;

// FSM structure that will store FSM state and transition functions -> it will store long-lasting objects

struct FSMData{
  Program program;
  Machine machine;
  data_t t_tot, t_blk;
  FSMData(string yaml_file) : machine(yaml_file), program(&machine) {}

};

int main(int argc, const char *argv[]){

  // check arguments
  if(argc != 3){

    cerr << "Usage: " << argv[0] << " <machine.yml> <program.gcpde" << endl;

    return 1;
  }

  FSMData data(argv[1]);
  FiniteStateMachine fsm(&data);

  try{

    data.program.load(argv[2]);

  } catch(exception &e){

    cerr << fg::red << style::bold << "Error: " << e.what() << style::reset << fg::reset << endl;

    return 2;
  }

  cerr << fg::green << style::bold << "MACHINE: " << style::reset << fg::reset << endl
    << data.machine.desc(true) << endl
    << fg::green << style::bold << "PROGRAM: " << style::reset
    << fg::reset << endl
    << data.program.desc(true) << endl;

  // Timer usage
  data_t tq = data.machine.tq();
  Timer timer(duration<double>(tq), duration<double>(tq * 2));
  timer.start();    // enable timer

  fsm.set_timing_function([&timer]() {
    timer.wait();     // waits for a timer event every t   s or maximum 2 * tq
  });

  // more refined:
  fsm.set_timing_function([&timer]() {

    try{
      
      timer.wait_throw();

    } catch(exception &e){

      cerr << fg::red << "Realtime not satisfied, exiting" << fg::reset << endl;
      cerr << "Message: " << e.what() << endl;
      cncpp::stop_requested = true;
    }

  });

  #ifdef DEBUG

  fsm.run([&fsm](FSMData &d){

    cerr << "State: " << fsm.state_name() << endl;

  });

  #else
  fsm.run();

  #endif
  timer.stop();

  return 0;
}



/*

runna il broker di mqtt
compila e runna build/cncpp machine.yml test.gcode e dovremmo avere i print degli stati correnti



*/