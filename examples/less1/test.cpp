#include <string>
#include <iostream>

using namespace std;

/*
or else, you can include only interested classes / functions ->
using std::string
*/

template <typename T>
T add(T a, T b); // declaration

template <typename T, unsigned int N>
T sum (T a){
  T tmp = 0.0;
  // N is provided by the template -> it is a template variabile which we can usa to create a for loop
  for(int i = 0; i < N; i++){
    tmp += a;
  }

  return tmp;
}

int main(){

  string my_string("hello");

  cout << my_string << " " << add(1, 2) << endl;

  double n3 = sum<double, 100>(2.2);   // faster implementation
  cout << endl << n3 << endl;

  return 1;
}

template <typename T>
T add(T a, T b){
  return a + b;
}                               // definition

// so we can use the same function for different types as inputs and outputs