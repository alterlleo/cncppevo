/*
--- POINT.HPP ---
represents a 3-D coordinate object
*/

#ifndef POINT_HPP
#define POINT_HPP

#include "defines.hpp"

using namespace std;

namespace cncpp{

class Point : Object{
  public:

    Point(opt_data_t x = nullopt, opt_data_t y = nullopt, opt_data_t z = nullopt);

    Point& operator=(const Point& other){
      if(this != &other){
        _x = other._x;
        _y = other._y;
        _z = other._z;
      }

      return *this;
    }

    Point operator+(const Point &other) const;

    /**
     * 
     * @brief Calculates the projections
     * @example [1 1 0] and [2 1 0] -> [1 0 0]
     * 
     */
     
    Point delta(const Point &other);

    /**
     * 
     * @brief Inherits from previous points
     * @example [1 - -] and [2 1 3] -> [1 1 3]
     * 
     */
    void modal(const Point &other);

    data_t length() const;

    /**
     * 
     * @brief Set all coordinates to nullopt (undefined)
     * 
     */
    void reset();

    /**
     * 
     * @example Description like: [100.0, 200.0, 123.2]
     */
    string desc() const override;

    /**
     * 
     * @brief Return true value only if all the coordinates are defined
     * 
     */
    bool is_complete() const{

      return _x.has_value() && _y.has_value() && _z.has_value();
    }

    
    // accessors
    data_t x() const{ return _x.value(); }
    data_t y() const{ return _y.value(); }
    data_t z() const{ return _z.value(); }

    data_t x(data_t v){ 
      return (_x = v).value();
    }
    data_t y(data_t v){ 
      return (_y = v).value();
    }
    data_t z(data_t v){ 
      return (_z = v).value();
    }

  private:

    opt_data_t _x = nullopt;      // convention -> class attributes have the _
    opt_data_t _y = nullopt;
    opt_data_t _z = nullopt;

}; // class Point

} // namespace cncpp


#endif // POINT_HPP