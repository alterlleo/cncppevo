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
#include <mosquittopp.h>
#include <mosquittopp.h>
#include <mosquitto.h>
#include <nlohmann/json.hpp>

using namespace std;
using namespace mosqpp;
// mosquitto is a protocol for exchanging data, it's up to us the formatting
// for semplicity we decide to communicate through json -> it's not efficient in terms of bandwidth. In this case we should use binary encoding
// we have just to exchange the setpoint
using json = nlohmann::json;

namespace cncpp{

  class Machine final : Object, mosquittopp{

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
      ~Machine();

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
      Point position() const { return _position;}
      Point setpoint() const { return _setpoint;}
      Point setpoint(Point p) {_setpoint = p; return _setpoint;}      // also writer 
      Point setpoint(data_t x, data_t y, data_t z){                   // also writer
        _setpoint.x(x);
        _setpoint.y(y);
        _setpoint.z(z);

        return _setpoint;
      }


      string mqtt_host() const { return "mqtt://" + _mqtt_host + ":" + to_string(_mqtt_port); }

      /**
        __  __  ___ _____ _____                  _   _               _     
       |  \/  |/ _ \_   _|_   _|  _ __ ___   ___| |_| |__   ___   __| |___ 
       | |\/| | | | || |   | |   | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
       | |  | | |_| || |   | |   | | | | | |  __/ |_| | | | (_) | (_| \__ \
       |_|  |_|\__\_\|_|   |_|   |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                           
      */

      /**
       * 
       * @brief it returns a MQTT-error code
       */
      int connect();

      // if we are not in a rapid motion, we don't care about the actual position of hte machine, so we switch it off when not executing rapid motions

      void listen_start();
      void listen_stop();

      // Overrides

      void on_connect(int rc) override;
      void on_disconnect(int rc) override;
      /**
       * 
       * @brief The acutal qos obtained may be different from the one stored and it is the broker that send it
       * @param mid
       * @param qos_count quality of service counter
       * @param qos quality of service instance pointer
       */
      void on_subscribe(int mid, int qos_count, const int *qos) override;

      void on_unsubscribe(int mid) override;

      /**
       * 
       * @brief callback that run every time some value occurs from the broker in a topic
       * @param message message defined in the mqtt protocol
       */
      void on_message(const struct mosquitto_message *message) override;

      /**
       * 
       * @brief
       * @param rapid flag for rapid motion
       */
      void sync(bool rapid);


    private:  

      // startup inizialization of parameters before file loading

      string _settings_file = "";
      data_t _A = 5.0;
      Point _zero = Point(0, 0, 0);
      Point _offset = Point(0, 0, 0);
      Point _setpoint, _position;         // setpoint is the point given to the machine
                                          // position is the point given from the machine
      data_t _tq = 0.005;                 // sampling time -> tick
      data_t _fmax;
      data_t _error = 0.0;                // current error
      data_t _max_error = 0.005;          // maximum allowable error -> 5 micrometers

      string _mqtt_host = "localhost";    // broker running on the same machine (our assumption)
      int _mqtt_port = 1883;
      int _mqtt_keepalive = 60;           // number of seconds to wait before assuming that the connection is dead
      string _pub_topic;                  // publish setpoints
      string _sub_topic;                  // get current position
      char _msg_buffer[MQTT_BUFLEN];


  };
}

#endif // MACHINE_HPP
