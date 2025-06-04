/*
  ____  _            _    _____ ____   ____        _               
 | __ )| | ___   ___| | _|_   _|  _ \ / ___|   ___| | __ _ ___ ___ 
 |  _ \| |/ _ \ / __| |/ / | | | |_) | |      / __| |/ _` / __/ __|
 | |_) | | (_) | (__|   <  | | |  _ <| |___  | (__| | (_| \__ \__ \
 |____/|_|\___/ \___|_|\_\ |_| |_| \_\\____|  \___|_|\__,_|___/___/

IDEA:

Program class should be a list of *Block, so it can contain also pointers to BlockTRC instances. Is the effort worth?

program.cpp -> when adding a new BlockTRC through << operator, parse function of BlockTRC is called. During parsing, check the angle between the destination and the starting point. If angle > pi, then rise a flag (_shaping_required). Then after parsing and adding the block, so after << operator in Program, let's check the flag of the added block. If high, let's rise a flag in Program. This last flag then msut be checked and if high, add the arc "junction" block (this new block should be computed in Program class? TBD)
                                                                   
*/

#include <fmt/color.h>
#include <fmt/format.h>
#include <rang.hpp>
#include <sstream>
#include "blockTRC.hpp"

using namespace std;
using namespace cncpp;
using namespace fmt;
using namespace rang;

const map<BlockTRC::TRCType, string> BlockTRC::trc_types = {

  {TRCType::NONE, "None"},
  {TRCType::LEFT, "Left"},
  {TRCType::RIGHT, "Right"}
};

BlockTRC::BlockTRC(string line) : Block(line) , _trc(false) {}

BlockTRC::BlockTRC(string line, BlockTRC &prev) : Block(line, prev) {}

BlockTRC::~BlockTRC(){}

BlockTRC &BlockTRC::operator=(BlockTRC &b){

  Block::operator=(b);
  _trc = b.trc();

  return *this;
}

BlockTRC &BlockTRC::parse(const Machine *m){
  _machine = m;

  stringstream ss(_line);
  string token;

  while(ss >> token){

    try{

      parse_token(token);
    
    } catch(CNCError &e){

      stringstream ss;
      
      ss << "Parsing error at line: " << _line << endl;
      ss << "Token: " << token << endl;
      ss << "Exception: " << e.what() << endl;

      // if there is an error here, we don't want to keep the program working
      throw CNCError(ss.str(), this);
    }
  }

  _target.modal(start_point());
  _delta = _target.delta(start_point());
  _acc = _machine -> A();
  _length = _delta.length();

  switch(_type){

    case BlockType::LINE:
      _acc = _machine -> A();
      _arc_feedrate = _feedrate;
      compute();
      break;

    case BlockType::CWA: 
    case BlockType::CCWA: 
      // in these cases we have to compute the arc radius
      calc_arc();

      _arc_feedrate = min(
        _feedrate,      // nominal one
        pow(3.0 / 4.0 * pow(_machine -> A(), 2) * pow(_r, 2), 0.25) * 60
      );

      compute();
      break;
    
    default:
      break;
  }

  
  switch(_trc_type){

    case TRCType::NONE:
      _trc = false;

    case TRCType::LEFT:
    case TRCType::RIGHT:

      _trc = true;

      if(angle_with_prev() > 3.1415){   // TODO: #define in defines.hpp for pi
        
        _shaping_required = true;
      } else{

        _shaping_required = false;
      }
    
    default:
      break;
  }

  _parsed = true;

  return *this;
}


BlockTRC BlockTRC::arc_shaping(){

  _shaping_required = false;

  // compute the parameters of the new arc to be added 


}


/*
  ____       _            _                        _   _               _     
 |  _ \ _ __(_)_   ____ _| |_ ___   _ __ ___   ___| |_| |__   ___   __| |___ 
 | |_) | '__| \ \ / / _` | __/ _ \ | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
 |  __/| |  | |\ V / (_| | ||  __/ | | | | | |  __/ |_| | | | (_) | (_| \__ \
 |_|   |_|  |_| \_/ \__,_|\__\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                             
*/

void BlockTRC::parse_token(string token){
    // we want to support both capital and lower cases, let's put all capital
  char cmd = toupper(token[0]);   // token[0] is the first letter in the token

  string arg = token.substr(1);   // sub string that starts at the element
  if(arg.empty())
    throw CNCError("Empty command argument", this);

  switch(cmd){

  case 'N':
    _n = stoi(arg);             // stoi takes a string/character and it parses it as an integer
    if(prev && _n <= prev -> n())
      throw CNCError("Block number must be increasing: " + to_string(prev -> n()), this);
    break;

  case 'G':

    if(static_cast<BlockType>(stoi(arg)) <= BlockType::NO_MOTION){

      _type = static_cast<BlockType>(stoi(arg));
    
    } else if(static_cast<TRCType>(stoi(arg)) >= TRCType::NONE && static_cast<TRCType>(stoi(arg)) <= TRCType::RIGHT){

      _trc_type = static_cast<TRCType>(stoi(arg));
    
    } else{

      throw CNCError("Unknown G type", this);
    }

    break;

  case 'X':
    _target.x(stod(arg));       //stod is string to double
    break;

  case 'Y':
    _target.y(stod(arg));
    break;

  case 'Z':
    _target.z(stod(arg));
    break;

  case 'I':
    _i = stod(arg);
    break;

  case 'J':
    _j = stod(arg);
    break;

  case 'R':
    _r = stod(arg);
    break;

  case 'F':
    _feedrate = stod(arg);
    break;
  
  case 'S':
    _spindle = stod(arg);
    break;

  case 'T':
    _tool = stoi(arg);
    break;
  
  case 'M':
    _m = stoi(arg);
    break;

  default:
  
    stringstream ss;
    ss << "Unknown / unsopported command: '" << token << "'";
    throw CNCError(ss.str(), this);

  }
}


data_t BlockTRC::angle_with_prev(){

}




