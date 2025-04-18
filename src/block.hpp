/*
  ____  _            _           _               
 | __ )| | ___   ___| | __   ___| | __ _ ___ ___ 
 |  _ \| |/ _ \ / __| |/ /  / __| |/ _` / __/ __|
 | |_) | | (_) | (__|   <  | (__| | (_| \__ \__ \
 |____/|_|\___/ \___|_|\_\  \___|_|\__,_|___/___/
                                                 
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
       * @param fe ending param
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


      /**
       * 
       * @brief Constructor. Remember that the gcode put the informations only about the destination block
       * @param line line of the gcode
       * 
       */
      Block(string line);
      Block(string line, Block &prev);
      ~Block();

      /*
      --- METHODS ----
      */
      string desc(bool colored = true) const override;

      /**
       * 
       * @brief parsing method form wich we get all infos
       * @param m pointer to the machine wich is stored internally in the block instance.
       * 
       */
      void parse(const Machine *m);

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
       * @brief x(t) = x0 + lambda(t) * delta_x. Defined depending on lambda function
       * @param lambda lambda function
       */
      Point interpolate(data_t lambda);

      /**
       * 
       * @brief x(t) = x0 + lambda(t) * delta_x. Defined depending on time
       * @param lambda reference to lambda -> output
       * @param speed reference to speed -> output
       */
      Point interpolate(data_t time, data_t &lambda, data_t &speed);

      /**
       * 
       * @brief Declaration of lambda function
       * 
       */
      void walk(std::is_function<void(Block &b, data_t t, data_t l, data_t s)> f);


      /*
      --- ACCESSORS ---
      */
      string line() const { return _line;}
      size_t n() const { return _n;}
      size_t tool() const { return _tool;}
      data_t feedrate() const { return _feedrate;}
      data_t arc_feedrate() const { return _arc_feedrate;}
      data_t spindle() const { return _spindle;} 
      data_t length() const { return _length;}
      Point center() const { return _center;}
      Point target() const { return _target;}
      Point delta() const { return _delta;}
      Profile profile() const { return _profile;}

    private:


      // some of these values will be computed depending of the machine characteristics

      Machine *_machine;
      BlockType _type = BlockType::RAPID;   // default value
      Profile _profile;

      string _line;         // original g-code line
      size_t _n = 0;        // size_t is an unsigned integer of 64bit (?) --> it is the line number
      size_t _tool = 0; 
      data_t _feedrate = 0;
      data_t _arc_feedrate = 0;
      data_t _spindle = 0;

      Point _target();
      Point _center();    // if we are moving along an arc
      Point _delta();

      data_t _length = 0;
      data_t _i = 0, _j = 0, _r = 0;  // i and j are coordinates of the center, r is the radius
      data_t _theta_0 = 0, _dtheta = 0;   // dtheta is the total delta of the angle
      data_t _acc = 0;

  };





}


#endif // BLOCK_HPP