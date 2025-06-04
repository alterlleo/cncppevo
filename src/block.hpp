/*
  ____  _            _           _               
 | __ )| | ___   ___| | __   ___| | __ _ ___ ___ 
 |  _ \| |/ _ \ / __| |/ /  / __| |/ _` / __/ __|
 | |_) | | (_) | (__|   <  | (__| | (_| \__ \__ \
 |____/|_|\___/ \___|_|\_\  \___|_|\__,_|___/___/

 It represents a block of gcode, including parsing and interpolation
                                                 
*/

#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "defines.hpp"
#include "point.hpp"
#include "machine.hpp"
#include <functional>
#include <map>

using namespace std;

namespace cncpp{

  class Block : Object{
    public:

      /**
       * 
       * @brief Profile struct it's defined here because it is used only within this class. It's also defined as struct because by default it has public parameters
       * @param a acceleration
       * @param d deceleration
       * @param fs starting feedrate
       * @param fe ending feedrate
       * @param dt_1 delta time of acceleration phase
       * @param dt_m delta time of maintenance phase
       * @param dt_2 delta time 
       * @param dt total time duration of the block
       * @param current_acc current acceleration
       * 
       */
      struct Profile{
        data_t a, d;
        data_t f, l;
        data_t fs, fe;
        data_t dt_1, dt_m, dt_2;
        data_t dt;
        data_t current_acc;

        // output -> lambda function with time and speed as parameters
        data_t lambda(data_t t, data_t &s);
      };

      Block *prev;      // pointer to the previous block
      Block *next;      // pointer to the next block

      enum class BlockType{
        RAPID = 0,
        LINE = 1,
        CWA = 2,        // clockwise arc
        CCWA = 3,       // anticlockwise arc
        NO_MOTION
      };

      // types[BlockType::LINE] -> "linear". 
      //It is static -> it's a property of the class and not of the single istance!
      static const map<BlockType, string> types;


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
       * @brief Constructor. Remember that the gcode put the informations only about the destination block
       * @param line line of the gcode
       * @param prev previous block reference
       * 
       */
      Block(string line);
      Block(string line, Block &prev);
      ~Block();

      Block &operator=(Block &b); 

      /*
        ____        _     _ _                       _   _               _     
       |  _ \ _   _| |__ | (_) ___   _ __ ___   ___| |_| |__   ___   __| |___ 
       | |_) | | | | '_ \| | |/ __| | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
       |  __/| |_| | |_) | | | (__  | | | | | |  __/ |_| | | | (_) | (_| \__ \
       |_|    \__,_|_.__/|_|_|\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                              
      */
      string desc(bool colored = true) const override;

      /**
       * 
       * @brief parsing method form wich we get all infos. From the line of the gcode, the function parses it in words, where each word is a letter + number
       * @param m pointer to the machine wich is stored internally in the block instance.
       * @return the same block that it's parsing
       * 
       */
      Block &parse(const Machine *m);

      /**
       * 
       * @brief lambda function definition(integral of the trapezoidal profile with Acceleration, Maintenance, Deceleration zones) -> it represents the scalar distance traveled along that direction, relative to total lenght (normalized)
       * @param time
       * @param speed reference to speed profile that the function modify -> it is an output
       * @returns if it is accelerating or decelerating, it is a quadratic form, if cruising, lambda is linear
       * 
       */
      data_t lambda(data_t time, data_t &speed);

      /**
       * 
       * @brief x(t) = x0 + lambda(t) * delta_x. Defined depending on lambda function. There are 2 cases: 1) the block describes a segment -> x(t) = x0 + d_x * lambda(t) and the same for y and z. 2) the block describes an arc. Z increases linearly for both cases
       * @param lambda lambda function
       */
      Point interpolate(data_t lambda);

      /**
       * 
       * @brief x(t) = x0 + lambda(t) * delta_x. Defined depending on time
       * @param time
       * @param lambda reference to lambda -> output
       * @param speed reference to speed -> output
       */
      Point interpolate(data_t time, data_t &lambda, data_t &speed);

      /**
       * 
       * @brief Function for walking along the block -> it converts a block in a sequence of evaluations for each time step
       * 
       */
      void walk(function<void(Block &b, data_t t, data_t l, data_t s)> f);


      /*
           _                                        
          / \   ___ ___ ___  ___ ___  ___  _ __ ___ 
         / _ \ / __/ __/ _ \/ __/ __|/ _ \| '__/ __|
        / ___ \ (_| (_|  __/\__ \__ \ (_) | |  \__ \
       /_/   \_\___\___\___||___/___/\___/|_|  |___/
                                                    
      */

      string line() const { return _line;}
      size_t n() const { return _n;}
      data_t dt() const { return _profile.dt;}
      BlockType type() const { return _type;}
      string type_name() const { return types.at(_type);}
      size_t tool() const { return _tool;}
      data_t feedrate() const { return _feedrate;}
      data_t arc_feedrate() const { return _arc_feedrate;}
      data_t spindle() const { return _spindle;} 
      data_t length() const { return _length;}
      Point center() const { return _center;}
      Point target() const { return _target;}
      Point delta() const { return _delta;}
      size_t m() const{return _m;}

      const Profile &profile() const { return _profile;}    // the output is the reference to the original profile object in order to save more computation resources. The _profile needs to be a constant of the block, because it is modified only during the parsing phase, it's must be coupled

    protected:


      // some of these values will be computed depending of the machine characteristics

      const Machine *_machine = nullptr;                    // pointer to the machine instance
      BlockType _type = BlockType::NO_MOTION;   // default value
      Profile _profile;                     // speed profile of the block

      string _line;                         // original g-code line
      size_t _n = 0;                        // size_t is an unsigned integer of 64bit (?) --> it is the line number
      size_t _tool = 0; 
      data_t _feedrate = 0;
      data_t _arc_feedrate = 0;
      data_t _spindle = 0;

      Point _target = Point();                      // target position
      Point _center = Point();                      // if we are moving along an arc
      Point _delta = Point();                       // 

      data_t _length = 0;
      data_t _i = 0, _j = 0, _r = 0;        // i and j are coordinates of the center, r is the radius
      data_t _theta_0 = 0, _dtheta = 0;     // dtheta is the total delta of the angle
      data_t _acc = 0;

      size_t _m = 0;                    // machine command (M command of the gcode)
      bool _parsed = false;             // flag for checking if the block has been parsed or not


      /*
        ____       _            _                        _   _               _     
       |  _ \ _ __(_)_   ____ _| |_ ___   _ __ ___   ___| |_| |__   ___   __| |___ 
       | |_) | '__| \ \ / / _` | __/ _ \ | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
       |  __/| |  | |\ V / (_| | ||  __/ | | | | | |  __/ |_| | | | (_) | (_| \__ \
       |_|   |_|  |_| \_/ \__,_|\__\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                                   
      */

      /**
       * 
       * @brief from a line of gcode we extract the informations
       * @param token i-th line of the gcode
       * 
       */
      void parse_token(string token);

      /**
       * 
       * @brief the previous block if it is present -> the starting point of the machine, so the current positino before the destination point
       * 
       */
      Point start_point();

      /**
       * 
       * @brief evaluate the velocity profile, both for the trapezoidal profile and the triangular profile
       * 
       */
      void compute();

      /**
       * 
       * @brief Calculating the radius, center and starting / ending angles of the arc
       * 
       */
      void calc_arc();

  };


  /*std::ostream &operator<<(std::ostream &os, const Block &b){
    return os << b.desc(); 
  }*/

}


#endif // BLOCK_HPP