// loads machine file, a program, execute the program and generate a table of data in order to be plot
// load a block at time and use walk
// exploit the fact that the program is a list, each element is a block and for each one 

/*
  ____  _                 _       _       
 / ___|(_)_ __ ___  _   _| | __ _| |_ ___ 
 \___ \| | '_ ` _ \| | | | |/ _` | __/ _ \
  ___) | | | | | | | |_| | | (_| | ||  __/
 |____/|_|_| |_| |_|\__,_|_|\__,_|\__\___|
                                          
*/

#include "../cncpp.hpp"
#include <iostream>
#include <rang.hpp>
#include <fmt/core.h>

#include <cstdlib>

using namespace std;
using namespace cncpp;
using namespace rang;
using namespace fmt;

using bt = Block::BlockType;

int main(int argc, char *argv[]){

  if(argc < 3){ // if we don't have enough arguments

    cerr << style::bold << "Usage: " << argv[0] << " <machine.yml> <program.gcode>" << style::reset << endl;
    return 1;
  }

  
  // load the machine
  Machine machine;

  try{

    machine.load(argv[1]);

  } catch (exception &e){

    cerr << fg::red << style::bold << "Error: " << e.what() << style::reset << fg::reset << endl;
    return 2;
  }

  cerr << style::bold << "Machine: " << style::reset << endl;
  cerr << machine.desc() << endl;

  // we are printing all in the stderr, so in the stdout we print all the tabulated data only

  // load the program
  Program program(&machine);

  try{
    program.load(argv[2]);

  } catch(exception &e){

    cerr << fg::red << style::bold << "Error: " << e.what() << style::reset << fg::reset << endl;
    return 3;
  }

  cerr << style::bold << "Parsing program " << argv[2] << endl << style::reset << program.desc() << endl;

  // go through the whole program
  // load one block a time and interpolate / walk --> now we can not do it for rapid blocks because for rapid motion we have to wait the feedback of the machine
  // so now we skip over rapid blocks

  data_t t_tot = 0;
  Point pos;

  // before the tabled data, we must print the columns name
  cout << "n,type,t_tot,t,lambda,feedrate,X,Y,Z" << endl;

  int count = 0;

  for(auto &b : program){

/*
    if(count != 0){
      cerr << "check correct order of blocks: " << endl;
      cerr << b -> desc() << " THAT STARTS FROM " << b -> prev -> desc() << endl;
    }
*/

    // check blocktype in order to skip the rapid block
    if(b -> type() == bt::RAPID || b -> type() == bt::NO_MOTION){

      cerr << fg::yellow << "Skipping block " << b -> line() << fg::reset << endl;
      continue;     //"skip" this iteration
    }
    
    // the & in the [] means that i can have access to the reference of outer parameters
    b -> walk([&](Block &b, data_t t, data_t lambda, data_t feedrate){

      pos = b.interpolate(lambda);
      // t is the current time related to the block, but we also want a total time
      t_tot += machine.tq();

      // CSV format
      cout << format("{:},{:},{:.3f},{:.3f},{:.6f},{:.3f},{:.3f},{:.3f},{:.3f}", b.n(), b.type_name(), t_tot, t, lambda, feedrate, pos.x(), pos.y(), pos.z()) << endl;

      count = 1;

    });

    cerr << b -> target().desc() << endl;
  }

  cerr << style::bold << "Done" << style::reset << endl;

  return 0;
}



// check for running in feal time -> by running the gcode we see that it spend 7.875 seconds or whatever. Runna mettenedo time prima del comando cosi vedi quanto tempo impiega il computer a runnare -> deve essere meno per poter essere real time

