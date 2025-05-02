// running the program and load it into the machine. A program is the full list of blocks
// it loads the configuration file of the cnc -> in our case is a .yaml file
// it's good to have parameters in a separate file in order to change them and the software will "adapt" by simply loading them
/*
  __  __            _     _                   _               
 |  \/  | __ _  ___| |__ (_)_ __   ___    ___| | __ _ ___ ___ 
 | |\/| |/ _` |/ __| '_ \| | '_ \ / _ \  / __| |/ _` / __/ __|
 | |  | | (_| | (__| | | | | | | |  __/ | (__| | (_| \__ \__ \
 |_|  |_|\__,_|\___|_| |_|_|_| |_|\___|  \___|_|\__,_|___/___/
                                                              
*/

#ifndef MACHINE_HPP
#define MACHINE_HPP

#include "defines.hpp"
#include "point.hpp"

using namespace std;

namespace cncpp{

  class Machine final : Object{

    public:

      /*
        _     _  __                      _      
       | |   (_)/ _| ___  ___ _   _  ___| | ___ 
       | |   | | |_ / _ \/ __| | | |/ __| |/ _ \
       | |___| |  _|  __/ (__| |_| | (__| |  __/
       |_____|_|_|  \___|\___|\__, |\___|_|\___|
                              |___/             
      */

      /**
       * 
       * @brief
       * @param settings_file name of the configuration file that should be loaded
       * 
       */
      Machine(const string &settings_file);
      Machine(){ }
      ~Machine() { }

      /*
        ____        _     _ _                       _   _               _     
       |  _ \ _   _| |__ | (_) ___   _ __ ___   ___| |_| |__   ___   __| |___ 
       | |_) | | | | '_ \| | |/ __| | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
       |  __/| |_| | |_) | | | (__  | | | | | |  __/ |_| | | | (_) | (_| \__ \
       |_|    \__,_|_.__/|_|_|\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                              
      */

      void load(const string &settings_file);
      string desc(bool colored = true) const override;
      /**
       * 
       * @brief quantize time -> starting from t0 and having a delta t, it returns the number of sample to the next tick after the the delta t. In a nutshell, it's a rounding up to the next multiple of tq
       * @param t time duration of the trapezoid
       * @param dq refernce to the elapsed time (modified in the body)
       * 
       */
      data_t quantize(data_t t, data_t &dq) const;



      /*
           _                                        
          / \   ___ ___ ___  ___ ___  ___  _ __ ___ 
         / _ \ / __/ __/ _ \/ __/ __|/ _ \| '__/ __|
        / ___ \ (_| (_|  __/\__ \__ \ (_) | |  \__ \
       /_/   \_\___\___\___||___/___/\___/|_|  |___/
                                                    
      */
     
      data_t A() const { return _A;}
      Point zero() const { return _zero;}
      Point offset() const{ return _offset;}
      data_t tq() const { return _tq;}
      data_t fmax() const { return _fmax;}
      data_t error() const { return _error;}
      data_t max_error() const { return _max_error;}


    private:  

      // startup inizialization of parameters before file loading

      string _settings_file = "";
      data_t _A = 5.0;
      Point _zero = Point(0, 0, 0);
      Point _offset = Point(0, 0, 0);
      data_t _tq = 0.005;                 // sampling time -> tick
      data_t _fmax;
      data_t _error = 0.0;                // current error
      data_t _max_error = 0.005;          // maximum allowable error -> 5 micrometers

  };
}

#endif // MACHINE_HPP
