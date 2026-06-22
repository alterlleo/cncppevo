/*
  ____       _       _          _               
 |  _ \ ___ (_)_ __ | |_    ___| | __ _ ___ ___ 
 | |_) / _ \| | '_ \| __|  / __| |/ _` / __/ __|
 |  __/ (_) | | | | | |_  | (__| | (_| \__ \__ \
 |_|   \___/|_|_| |_|\__|  \___|_|\__,_|___/___/
                                                
*/

#include "point.hpp"
#include <exception>
#include <sstream>
#include <cmath>
#include <vector>
#include <fmt/color.h>
#include <fmt/ranges.h>
#include <fmt/format.h> //format library


using namespace std;
using namespace cncpp;
using namespace fmt;


using col_t = optional<color>;
static string coord_str(opt_data_t const &coord, col_t const &color = nullopt);

Point::Point(opt_data_t x, opt_data_t y, opt_data_t z, opt_data_t a, opt_data_t c) : _x(x), _y(y), _z(z), _a(a), _c(c) {}


void Point::reset(){

  _x.reset();
  _y.reset();
  _z.reset();
  _a.reset();
  _c.reset();

}


/*
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
*/
void Point::modal(const Point &p){

  if (!this->_x && p._x)
    this->_x = p._x;

  if (!this->_y && p._y)
    this->_y = p._y;

  if (!this->_z && p._z)
    this->_z = p._z;

  if(!this -> _a && p._a)
    this -> _a = p._a;

  if(!this -> _c && p._c)
    this -> _c = p._c;
}


Point Point::operator+(const Point &other) const{
  // both the points must be complete in order to be sum. If not, rise and excp
  if(!is_complete() || !other.is_complete())
    throw CNCError("Points are not complete", this);

  Point out(

    _x.value() + other._x.value(),
    _y.value() + other._y.value(),
    _z.value() + other._z.value(),
    _a.value() + other._a.value(),
    _c.value() + other._c.value()
  );

  return out;
}

Point Point::operator-(const Point &other) const{

  if(!is_complete() || !other.is_complete())
    throw CNCError("Points are not complete", this);

  Point out(

    _x.value() - other._x.value(),
    _y.value() - other._y.value(),
    _z.value() - other._z.value(),
    _a.value() - other._a.value(),
    _c.value() - other._c.value()

  );

  return out;
}



Point Point::delta(const Point &other) const {

  if(!is_complete() || !other.is_complete())
    throw CNCError("Points are not complete", this);

  Point out(

    _x.value() - other._x.value(),
    _y.value() - other._y.value(),
    _z.value() - other._z.value(),
    _a.value() - other._a.value(),
    _c.value() - other._c.value()

  );

  return out;
}


data_t Point::length() const{

  if(!is_complete())
    throw CNCError("Points are not complete", this);

  return sqrt(
    
    _x.value() * _x.value() +
    _y.value() * _y.value() +
    _z.value() * _z.value()

  );

}

void Point::scale(data_t factor){
  _x = _x.value() * factor;
  _y = _y.value() * factor;
  _z = _z.value() * factor;
}


static string coord_str(opt_data_t const &coord, col_t const &color){
  // a static function is only visible within its file

  string str;
  
  if(coord && color){
    str = fmt::format("{:" NUMBERS_WIDTH ".3f}", styled(coord.value(), fg(color.value())));

  } else if(coord){
    str = fmt::format("{:" NUMBERS_WIDTH ".3f}", coord.value());

  } 
  else{
    str = fmt::format("{:>" NUMBERS_WIDTH "}", "-");
  }

  return str;
}

string Point::desc(bool col) const{

  stringstream ss;
  // we want to have a function that represent a value or a string if the coordinate is not defined
  ss << "[" << coord_str(_x, col ? col_t(color::red) : nullopt) << ", "
     << coord_str(_y, col ? col_t(color::green) : nullopt) << ", "
     << coord_str(_z, col ? col_t(color::blue) : nullopt) << ", "
     << coord_str(_a, col ? col_t(color::magenta) : nullopt) << ", "
     << coord_str(_c, col ? col_t(color::yellow) : nullopt) << "]";

  return ss.str();

}


vector<data_t> Point::vec() const{

  if(!is_complete())
    throw CNCError("Point is not complete", this);
  
  return vector<data_t>{_x.value(), _y.value(), _z.value(), _a.value(), _c.value()};
} 


/*
  _____         _   
 |_   _|__  ___| |_ 
   | |/ _ \/ __| __|
   | |  __/\__ \ |_ 
   |_|\___||___/\__|
                    
*/

#ifdef POINT_MAIN   // normally this constant is not defined so this part is not compiled. We have to add it in the cmakelists

#include <iostream> // we include it here becouse we don't need in the rest of the code
using namespace std;

int main(){
  Point p1(0, 0, 0);
  Point p2(100);            // x defined, y and z are not
  Point p3(nullopt, 100);   // y defined, x and z are not
  
  cout << "Before any change: " << endl;
  cout << "p1: " << p1.desc() << endl;
  cout << "p2: " << p2.desc() << endl;
  cout << "p3: " << p3.desc(false) << endl;


  /*
  --- MODAL TEST ---
  we are inheriting from the previous block
  */

  p2.modal(p1);
  p3.modal(p2);
  p3.z(100);        // we change the individual component

  cout << "After modal: " << endl;
  cout << "p1: " << p1.desc() << endl;
  cout << "p2: " << p2.desc() << endl;
  cout << "p3: " << p3.desc() << endl;

  cout << "Length of p3: " << p3.length() << endl;
  cout << "Delta of p1 and p3: " << endl;
  cout << p3.delta(p1).desc() << endl;

  /*
  --- ERROR TEST ---
  */
  try{
    Point p4(120, 20);
    cout << "p4 length: " << p4.length() << endl;   

  } catch(CNCError &e){
    cerr << "Error: " << e.what() << endl << "Raised by: " << e.who() << endl;

  } catch(exception &e){

    cerr << "Unexpected error" << endl;
  }

  /*
  exception &e means that it catch all the exceptions, also child one

  if there are expections, the code does not stop, so "Done " is printed
  */

  cout << "Vector from p3: " << endl;
  cout << fmt::format("{}", p3.vec()) << endl;

  cout << "Done. " << endl;
  return 0;
}

#endif // POINT_MAIN