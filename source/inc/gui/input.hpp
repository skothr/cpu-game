#ifndef INPUT_HPP
#define INPUT_HPP

#include "vector.hpp"
#include "helpers.hpp"

enum class input_t
  {
   NONE = 0,

   // movement
   MOVE_RIGHT,
   MOVE_LEFT,
   MOVE_FORWARD,
   MOVE_BACK,
   MOVE_UP,
   MOVE_DOWN,

   // actions
   ACTION_JUMP,
   ACTION_SNEAK,
   ACTION_RUN,

   // other input
   MOUSE_MOVE,
   MOUSE_DRAG,
   MOUSE_CLICK
  };

enum class mouseButton_t
  {
   NONE = 0x00,
   LEFT = 0x01,
   RIGHT = 0x02,
   MIDDLE = 0x04
  };
ENUM_CLASS_BITWISE_OPERATORS(mouseButton_t)

struct InputData
{
  struct Movement
  {
    float magnitude;
    bool keyDown;
  };
  struct Action
  {
    float magnitude;
    bool keyDown;
  };
  struct MouseMove
  {
    Point2f vPos;
    Vector2i dPos;
    bool drag;
    bool captured;
  };
  struct MouseClick
  {
    mouseButton_t button;
    bool buttonDown;
  };

  
  input_t type;
  Movement movement;
  Action action;
  MouseMove mouseMove;
  MouseClick mouseClick;
};


#endif // INPUT_HPP
