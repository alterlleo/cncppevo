/*
--- POINT CLASS ---
*/

#include "point.hpp"
#include <exception>
#include <sstream>
#include <cmath>

using namespace std;
using namespace cncpp;


Point::Point(opt_data_t x, opt_data_t y, opt_data_t z) : _x(x), _y(y), _z(z) {}


void Point::reset(){

  _x.reset();
  _y.reset();
  _z.reset();

}


void Point::modal(const Point &p){

  if(p._x && !_x)
    _x = p._x;      // we are inheriting from the other point only if the other point has the interested value

  if(p._y && !_y)
    _y = p._y;

  if(p._z && !_z)
    _z = p._z;

  _x.value_or(p._x.value());    // it returns a value OR a default
  _y.value_or(p._y.value());
  _z.value_or(p._z.value());
}


Point Point::operator+(const Point &other) const{
  // both the points must be complete in order to be sum. If not, rise and excp
  if(!is_complete() || !other.is_complete())
    throw runtime_error("Points are not complete");

  Point out(

    _x.value() + other._x.value(),
    _y.value() + other._y.value(),
    _z.value() + other._z.value()

  );

  return out;
}


Point Point::delta(const Point &other){

  if(!is_complete() || !other.is_complete())
    throw runtime_error("Points are not complete");

  Point out(

    _x.value() - other._x.value(),
    _y.value() - other._y.value(),
    _z.value() - other._z.value()

  );

  return out;
}


data_t Point::length() const{

  if(!is_complete())
    throw runtime_error("Points are not complete");

  return sqrt(
    
    _x.value() * _x.value() +
    _y.value() * _y.value() +
    _z.value() * _z.value()

  );

}


string Point::desc() const{

  stringstream ss;
  ss << "[" << _x.value() << ", " << _y.value() << ", " << _z.value() << "]";

  return ss.str();

}


/*
  _____         _   
 |_   _|__  ___| |_ 
   | |/ _ \/ __| __|
   | |  __/\__ \ |_ 
   |_|\___||___/\__|
                    
*/

#ifdef POINT_MAIN   // normally this constant is not defined so this part is not compiled. We have to add it in the cmakelists

int main(){
  return 0;
}

#endif // POINT_MAIN