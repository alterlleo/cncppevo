/*

  ____  _            _      _____ ____   ____        _               
 | __ )| | ___   ___| | __ |_   _|  _ \ / ___|   ___| | __ _ ___ ___ 
 |  _ \| |/ _ \ / __| |/ /   | | | |_) | |      / __| |/ _` / __/ __|
 | |_) | | (_) | (__|   <    | | |  _ <| |___  | (__| | (_| \__ \__ \
 |____/|_|\___/ \___|_|\_\   |_| |_| \_\\____|  \___|_|\__,_|___/___/

  It represents an extension of the block class. It implements the Tool Radius Compensation feature.

 */

#ifndef BLOCK_TRC_HPP
#define BLOCK_TRC_HPP

#include "defines.hpp"
#include "block.hpp"

using namespace std;

namespace cncpp{

  class BlockTRC : public Block{

    public:

      enum class TRCType{
        NONE = 40,
        LEFT = 41,
        RIGHT = 42
      };

      static const map<TRCType, string> trc_types;

      /*
        _     _  __                      _      
       | |   (_)/ _| ___  ___ _   _  ___| | ___ 
       | |   | | |_ / _ \/ __| | | |/ __| |/ _ \
       | |___| |  _|  __/ (__| |_| | (__| |  __/
       |_____|_|_|  \___|\___|\__, |\___|_|\___|
                              |___/             
      */
      BlockTRC(string line);
      BlockTRC(string line, BlockTRC &prev);
      BlockTRC(string line, BlockTRC *prev);
      ~BlockTRC();

      BlockTRC &parse(const Machine *m);

      /*
           _                                        
          / \   ___ ___ ___  ___ ___  ___  _ __ ___ 
         / _ \ / __/ __/ _ \/ __/ __|/ _ \| '__/ __|
        / ___ \ (_| (_|  __/\__ \__ \ (_) | |  \__ \
       /_/   \_\___\___\___||___/___/\___/|_|  |___/
                                                    
      */
      
      bool trc() const { return _trc; }
      bool shaping() const { return _shaping_required; }
      void set_r(data_t t){_r = t;}
      bool last() const {return _last; }
      /*
        ____        _     _ _                       _   _               _     
       |  _ \ _   _| |__ | (_) ___   _ __ ___   ___| |_| |__   ___   __| |___ 
       | |_) | | | | '_ \| | |/ __| | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
       |  __/| |_| | |_) | | | (__  | | | | | |  __/ |_| | | | (_) | (_| \__ \
       |_|    \__,_|_.__/|_|_|\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                              
      */

      BlockTRC &operator=(BlockTRC &b);

      /**
       * 
       * @brief method that returns the required arc block to be added between the current block instance and the previous one. Advanced TRC method.
       * The center of the arc is defined as the nominal corner of the trajectory
       * @param nominal_start means that it requires the starting point of the current move without considering the offset due to TRC. As it is implemented, it's required to store the target point of the previous block before editing it
       * @return string corresponding to new gcode line
       * 
       */
      string arc_shaping(Point nominal_start);
      
      string desc(bool colored = true) const override;

      /**
       * 
       * @brief main TRC function which shifts moves properly
       * 
       */
      void shift_prev_target();

      /**
       * 
       * @brief Find the intersction point between 2 consecutive lines while considering the tool radius. If the intersection requires an arc junction, the previous segment target is only shifted along its normal
       * @param p = previous block pointer, in this case a line
       * 
       */
      void line_line_shift(BlockTRC *p);

      /**
       * 
       * @brief Find the intersection point between a line and an arc while considering the tool radius. If the intersecction requirese an arc junction, the line target is only shifted along its normal
       * @param p = previous block pointer, in this case a line
       * 
       */
      void line_arc_shift(BlockTRC *p);

      /**
       * 
       * @brief Find the intersection point between an arc and a line while considering the tool radius. If the intersection requires an arc junction, the arc target is only shifted along the direction defined by its target and its center.
       * @param p = previous block pointer, in this case an arc
       * 
       */
      void arc_line_shift(BlockTRC *p);

      /**
       * 
       * @brief Find the intersection point between an arc and an arc while considering the tool radius. If the intersection requires an arc junction, the arc target is only shifted along the direction defined by its target and its center.
       * @param p = previous block pointer, in this case an arc
       * 
       */
      void arc_arc_shift(BlockTRC *p);

      /**
       * 
       * @brief Consider if the arc junction is required depending on angle with the previous move and type of move
       * 
       */
      bool is_shaping_needed();

    private:

      TRCType _trc_type = TRCType::NONE;
      bool _trc = false;
      bool _shaping_required = false;
      bool _last = false;

      /*
        ____       _            _                        _   _               _     
       |  _ \ _ __(_)_   ____ _| |_ ___   _ __ ___   ___| |_| |__   ___   __| |___ 
       | |_) | '__| \ \ / / _` | __/ _ \ | '_ ` _ \ / _ \ __| '_ \ / _ \ / _` / __|
       |  __/| |  | |\ V / (_| | ||  __/ | | | | | |  __/ |_| | | | (_) | (_| \__ \
       |_|   |_|  |_| \_/ \__,_|\__\___| |_| |_| |_|\___|\__|_| |_|\___/ \__,_|___/
                                                                                   
      */

      /**
       * 
       * @brief it compute the angle between the current move and the previous one. It uses the atan2 of the cross product and the dot product. The resulting angle is included in the range [-pi pi] and:
       * -- it is > 0 if the angle between the previous and the current move is CCW
       * -- it is < 0 if the angle between the previous and the current move is CW
       * -- 0 if the segments are collinear
       * 
       */
      data_t angle_with_prev();

      bool parse_token(string token);

      Point line_circle_intersection(Point c, data_t r, data_t m, data_t h, Point tp);

  };
}


#endif
