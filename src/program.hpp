// it is a sequence of blocks

/*
  ____                                             _               
 |  _ \ _ __ ___   __ _ _ __ __ _ _ __ ___     ___| | __ _ ___ ___ 
 | |_) | '__/ _ \ / _` | '__/ _` | '_ ` _ \   / __| |/ _` / __/ __|
 |  __/| | | (_) | (_| | | | (_| | | | | | | | (__| | (_| \__ \__ \
 |_|   |_|  \___/ \__, |_|  \__,_|_| |_| |_|  \___|_|\__,_|___/___/
                  |___/                                            
*/

#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include "block.hpp"
#include "machine.hpp"
#include "defines.hpp"
#include <list>
// we define program as a child class of std::list class

namespace cncpp{

  class Program : Object, public std::list<Block>{

    public:

      /*
        _     _  __                      _      
       | |   (_)/ _| ___  ___ _   _  ___| | ___ 
       | |   | | |_ / _ \/ __| | | |/ __| |/ _ \
       | |___| |  _|  __/ (__| |_| | (__| |  __/
       |_____|_|_|  \___|\___|\__, |\___|_|\___|
                              |___/             
      */

      Program(const std::string &filename, Machine *machine);
      Program(Machine *machine) : _machine(machine){}
      ~Program();

      /*
        ____        _     _ _                       _   _               _     
       |  _ \ _   _| |__ | (_) ___   _ __ ___   ___| |_| |__   ___   __| |___ 
       | |_) | | | | '_ \| | |/ __| | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
       |  __/| |_| | |_) | | | (__  | | | | | |  __/ |_| | | | (_) | (_| \__ \
       |_|    \__,_|_.__/|_|_|\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                              
      */
      
      /**
       * 
       * @brief it loads the gcode file. Here it replace the current filename
       * @param filename
       * @param append
       * 
       */
      void load(const string &filename, bool append = false);

      string desc(bool colored = true) const override;

      /**
       * 
       * @brief it can append a gcode line on the present list -> concatenation
       * @param line
       * 
       */
      Program &operator<<(std::string line);

      using iterator = std::list<Block>::iterator;        // let's define an alias

      /**
       * 
       * @brief when a block is executed, let's use this iterator to point to the next one
       * 
       */
      iterator load_next(){
        _current++;
        _done = _current == end();
        return _current;
      }

      /**
       * 
       * @brief back to the beginnign in order to restart again
       * 
       */
      void rewind(){
        _current = begin();
        _done = false;
      } 

      void reset(){

        clear();    // inherited method of the base class
        rewind();
      }


      /*
           _                                        
          / \   ___ ___ ___  ___ ___  ___  _ __ ___ 
         / _ \ / __/ __/ _ \/ __/ __|/ _ \| '__/ __|
        / ___ \ (_| (_|  __/\__ \__ \ (_) | |  \__ \
       /_/   \_\___\___\___||___/___/\___/|_|  |___/
                                                    
      */

      bool done() const { return _done;}

    private:

      Machine *_machine = nullptr;
      std::string _filename;
      iterator _current = begin();      // pointer to the current element of the list -> begin is the first element in the list and when we move we iterate this iterator
      bool _done = false;

  };

}

#endif

