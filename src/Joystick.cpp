/*
AppleWin : An Apple //e emulator for Windows

Copyright (C) 1994-1996, Michael O'Brien
Copyright (C) 1999-2001, Oliver Schmidt
Copyright (C) 2002-2005, Tom Charlesworth
Copyright (C) 2006-2007, Tom Charlesworth, Michael Pohoreski

AppleWin is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

AppleWin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AppleWin; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* Description: Joystick emulation via keyboard, PC joystick or mouse
 *
 * Author: Various - hmm, I know him, and you..? ^_^ --bb
 */

// TC: Extended for 2nd joystick:
// Apple joystick #0 can be emulated with: NONE, JOYSTICKID1, KEYBOARD, MOUSE
// Apple joystick #1 can be emulated with: NONE, JOYSTICKID2, KEYBOARD, MOUSE
// If Apple joystick #0 is using {KEYBOARD | MOUSE} then joystick #1 can't use it.
// If Apple joystick #1 is using KEYBOARD, then disable the standard keys that control Apple switches #0/#1.
// - So that in a 2 player game, player 2 can't cheat by controlling player 1's buttons.
// If Apple joystick #1 is not NONE, then Apple joystick #0 only gets the use of Apple switch #0.
// - When using 2 joysticks, button #1 is used by joystick #1 (Archon).
// Apple joystick #1's button now controls Apple switches #1 and #2.
// - This is because the 2-joystick version of Mario Bros expects the 2nd joystick to control Apple switch #2.

/* Adaptation for SDL and POSIX (l) by beom beotiger, Nov-Dec 2007 */


#include "stdafx.h"
#include "MouseInterface.h"

#include <iostream>

#define  BUTTONTIME       5000

#define  DEVICE_NONE      0
#define  DEVICE_JOYSTICK  1
#define  DEVICE_KEYBOARD  2
#define  DEVICE_MOUSE     3

#define  MODE_NONE        0
#define  MODE_STANDARD    1
#define  MODE_CENTERING   2
#define  MODE_SMOOTH      3

typedef struct _joyinforec {
  int device;
  int mode;
} joyinforec, *joyinfoptr;

static const joyinforec joyinfo[5] = {{DEVICE_NONE,     MODE_NONE},
                                      {DEVICE_JOYSTICK, MODE_STANDARD},
                                      {DEVICE_KEYBOARD, MODE_STANDARD},
                                      {DEVICE_KEYBOARD, MODE_CENTERING},
                                      {DEVICE_MOUSE,    MODE_STANDARD}};

// Key pad [1..9]; Key pad 0,Key pad '.'; Left ALT,Right ALT
enum JOYKEY {
  JK_DOWNLEFT = 0,
  JK_DOWN,
  JK_DOWNRIGHT,
  JK_LEFT,
  JK_CENTRE,
  JK_RIGHT,
  JK_UPLEFT,
  JK_UP,
  JK_UPRIGHT,
  JK_BUTTON0,
  JK_BUTTON1,
  JK_OPENAPPLE,
  JK_CLOSEDAPPLE,
  JK_MAX
};

const unsigned int PDL_MIN = 0;
const unsigned int PDL_CENTRAL = 127;
const unsigned int PDL_MAX = 255;

static bool keydown[JK_MAX] = {false};
const int PDL_SMAX = 127;
const int PDL_SCENTRAL = 0;
const int PDL_SMIN = -127;

static POINT keyvalue[9] = {{PDL_SMIN,     PDL_SMAX},
                            {PDL_SCENTRAL, PDL_SMAX},
                            {PDL_SMAX,     PDL_SMAX},
                            {PDL_SMIN,     PDL_SCENTRAL},
                            {PDL_SCENTRAL, PDL_SCENTRAL},
                            {PDL_SMAX,     PDL_SCENTRAL},
                            {PDL_SMIN,     PDL_SMIN},
                            {PDL_SCENTRAL, PDL_SMIN},
                            {PDL_SMAX,     PDL_SMIN}};


static unsigned int buttonlatch[3] = {0, 0, 0};
static bool joybutton[3] = {0, 0, 0};

static int joyshrx[2] = {8, 8};
static int joyshry[2] = {8, 8};
static int joysubx[2] = {0, 0};
static int joysuby[2] = {0, 0};

unsigned int joytype[2] = {DEVICE_JOYSTICK, DEVICE_NONE};  // Emulation Type for joysticks #0 & #1

static bool setbutton[3] = {0, 0, 0}; // Used when a mouse button is pressed/released

static int xpos[2] = {PDL_CENTRAL, PDL_CENTRAL};
static int ypos[2] = {PDL_CENTRAL, PDL_CENTRAL};

static UINT64 g_nJoyCntrResetCycle = 0;  // Abs cycle that joystick counters were reset

static int g_nPdlTrimX = 0;
static int g_nPdlTrimY = 0;

unsigned int joy1index = 0;
unsigned int joy2index = 1;
unsigned int joy1button1 = 0;
unsigned int joy1button2 = 1;
unsigned int joy2button1 = 0;
unsigned int joy1axis0 = 0;
unsigned int joy1axis1 = 1;
unsigned int joy2axis0 = 0;
unsigned int joy2axis1 = 1;
unsigned int joyexitenable = 0;
unsigned int joyexitbutton0 = 8;
unsigned int joyexitbutton1 = 9;
bool joyquitevent = 0;

SDL_Joystick *joy1 = NULL;
SDL_Joystick *joy2 = NULL;

void CheckJoyExit() {
  SDL_JoystickUpdate(); // update all joysticks states
  if (SDL_JoystickGetButton(joy1, joyexitbutton0) && SDL_JoystickGetButton(joy1, joyexitbutton1)) {
    joyquitevent = true;
  } else {
    joyquitevent = false;
  }
}

void CheckJoystick0() {
  if (!joy1) {
    return;
  }
  static unsigned int lastcheck = 0;
  unsigned int currtime = GetTickCount();
  if ((currtime - lastcheck >= 10) || joybutton[0] || joybutton[1]) {
    lastcheck = currtime;
    SDL_JoystickUpdate(); // update all joysticks states
    if ((SDL_JoystickGetButton(joy1, joy1button1)) && !joybutton[0]) {
      buttonlatch[0] = BUTTONTIME;
    }
    if ((SDL_JoystickGetButton(joy1, joy1button2)) && !joybutton[1] &&
        (joyinfo[joytype[1]].device == DEVICE_NONE)  // Only consider 2nd button if NOT emulating a 2nd Apple joystick
      ) {
      buttonlatch[1] = BUTTONTIME;
    }
    joybutton[0] = SDL_JoystickGetButton(joy1, joy1button1);
    if (joyinfo[joytype[1]].device ==
        DEVICE_NONE) { // Only consider 2nd button if NOT emulating a 2nd Apple joystick button
      joybutton[1] = SDL_JoystickGetButton(joy1, joy1button2);
    }

#ifdef SDL2
	// add by trngaje for hat
	switch(SDL_JoystickGetHat(joy1, 0))
	{
	case 1:
		keydown[JK_UP] = true;
		keydown[JK_DOWN] = false;
		break;
    case 2:
		keydown[JK_RIGHT] = true;
		keydown[JK_LEFT] = false;	
		break;
    case 4:
		keydown[JK_UP] = false;
		keydown[JK_DOWN] = true;
		break;
    case 8:
		keydown[JK_RIGHT] = false;
		keydown[JK_LEFT] = true;	
		break;
    default:
		keydown[JK_UP] = false;
		keydown[JK_DOWN] = false;
		keydown[JK_RIGHT] = false;
		keydown[JK_LEFT] = false;			
	
	}
#endif	
	//printf("[trngaje] hat0 = %d\n", SDL_JoystickGetHat(joy1, 0)); // 1 (up),2 (right),4 (down),8 (left)

	

    xpos[0] = (SDL_JoystickGetAxis(joy1, joy1axis0) - joysubx[0]) >> joyshrx[0];
    ypos[0] = (SDL_JoystickGetAxis(joy1, joy1axis1) - joysuby[0]) >> joyshry[0];

    // "Square" a modern analog stick (e.g. Playstation)
    auto x = xpos[0];
    auto y = ypos[0];
    if (y < (int) PDL_CENTRAL / 2) {
      if (x < (int) PDL_CENTRAL / 2) {
        // upper-left quadrant
        x = x - (PDL_CENTRAL / 2 - y) / 2;
        y = y - (PDL_CENTRAL / 2 - xpos[0]) / 2;
        if (x < 0) {
          x = 0;
        }
        if (y < 0) {
          y = 0;
        }
        xpos[0] = x;
        ypos[0] = y;
      } else if (x > (int) (PDL_CENTRAL + PDL_CENTRAL / 2)) {
        // upper-right quadrant
        x = x + (PDL_CENTRAL / 2 - y) / 2;
        y = y - (xpos[0] - (PDL_CENTRAL + PDL_CENTRAL / 2)) / 2;
        if (x > 255) {
          x = 255;
        }
        if (y < 0) {
          y = 0;
        }
        xpos[0] = x;
        ypos[0] = y;
      }
    } else if (y > (int) (PDL_CENTRAL + PDL_CENTRAL / 2)) {
      if (x < (int) PDL_CENTRAL / 2) {
        // lower-left quadrant
        x = x - (y - (PDL_CENTRAL + PDL_CENTRAL / 2)) / 2;
        y = y + (PDL_CENTRAL / 2 - xpos[0]) / 2;
        if (x < 0) {
          x = 0;
        }
        if (y > (int) PDL_MAX) {
          y = PDL_MAX;
        }
        xpos[0] = x;
        ypos[0] = y;
      } else if (x > (int) (PDL_CENTRAL + PDL_CENTRAL / 2)) {
        // lower-right quadrant
        x = x + (y - (int) (PDL_CENTRAL + PDL_CENTRAL / 2)) / 2;
        y = y + (xpos[0] - (PDL_CENTRAL + PDL_CENTRAL / 2)) / 2;
        if (x > 255) {
          x = 255;
        }
        if (y > (int) PDL_MAX) {
          y = PDL_MAX;
        }
        xpos[0] = x;
        ypos[0] = y;
      }
    }




    xpos[0] += g_nPdlTrimX;
    ypos[0] += g_nPdlTrimY;


  }
}

void CheckJoystick1() {
  if (!joy2) {
    return;
  }
  static unsigned int lastcheck = 0;
  unsigned int currtime = GetTickCount();
  if ((currtime - lastcheck >= 10) || joybutton[2]) {
    lastcheck = currtime;
    SDL_JoystickUpdate(); // update all joysticks states
    if (SDL_JoystickGetButton(joy2, joy2button1) && !joybutton[2]) {
      buttonlatch[2] = BUTTONTIME;
      if (joyinfo[joytype[1]].device != DEVICE_NONE) {
        buttonlatch[1] = BUTTONTIME;  // Re-map this button when emulating a 2nd Apple joystick
      }
    }
    joybutton[2] = SDL_JoystickGetButton(joy2, joy2button1);

    if (joyinfo[joytype[1]].device != DEVICE_NONE) {
      joybutton[1] = SDL_JoystickGetButton(joy2, joy2button1); // Re-map this button when emulating a 2nd Apple joystick
    }

    xpos[1] = (SDL_JoystickGetAxis(joy2, joy2axis0) - joysubx[1]) >> joyshrx[1];
    ypos[1] = (SDL_JoystickGetAxis(joy2, joy2axis1) - joysuby[1]) >> joyshry[1];

    // NB. This does not work for analogue joysticks (not self-centreing) - except if Trim=0
    if (xpos[1] == 127 || xpos[1] == 128) {
      xpos[1] += g_nPdlTrimX;
    }
    if (ypos[1] == 127 || ypos[1] == 128) {
      ypos[1] += g_nPdlTrimY;
    }
  }
}

// All globally accessible functions are below this line

void JoyShutDown() {
  // First of all, let's close all existing SDL joysticks
  if (joy1) {
    SDL_JoystickClose(joy1);
  }
  if (joy2) {
    SDL_JoystickClose(joy2);
  }
}

void JoyInitialize()
{
  // Emulated joystick #0 can only use JOYSTICKID1 (if no joystick, then use mouse)
  // Emulated joystick #1 can only use JOYSTICKID2 (if no joystick, then disable)

  // Init for emulated joystick #0:
  #define AXIS_MIN        -32768  /* minimum value for axis coordinate */
  #define AXIS_MAX        32767   /* maximum value for axis coordinate */

  // First of all, let's close all existing SDL joysticks
  if (joy1) {
    SDL_JoystickClose(joy1);
  }
  if (joy2) {
    SDL_JoystickClose(joy2);
  }
  int number_of_joysticks = SDL_NumJoysticks();

  if (joyinfo[joytype[0]].device == DEVICE_JOYSTICK) {
    if (number_of_joysticks > 0) {
      joy1 = SDL_JoystickOpen(joy1index); // open joystick and get its information into SDL_Joystick struct
      joyshrx[0] = 0;
      joyshry[0] = 0;
      joysubx[0] = AXIS_MIN; //just do not know how to get wXmin and alike from SDL joysticks
      joysuby[0] = AXIS_MIN;
      unsigned int xrange = AXIS_MAX - AXIS_MIN;
      unsigned int yrange = AXIS_MAX - AXIS_MIN;
      while (xrange > 256) {
        xrange >>= 1;
        ++joyshrx[0]; // joystick threshold??
      }
      while (yrange > 256) {
        yrange >>= 1;
        ++joyshry[0];
      }
    } else {
      joytype[0] = DEVICE_MOUSE;
    }
  }

  // Init for emulated joystick #1:

  if (joyinfo[joytype[1]].device == DEVICE_JOYSTICK) {
    if (number_of_joysticks > 1) {
      joy2 = SDL_JoystickOpen(joy2index); // open joystick #2 and get its information into SDL_Joystick struct
      joyshrx[1] = 0;
      joyshry[1] = 0;
      joysubx[1] = AXIS_MIN;
      joysuby[1] = AXIS_MIN;
      unsigned int xrange = AXIS_MAX - AXIS_MIN;
      unsigned int yrange = AXIS_MAX - AXIS_MIN;
      while (xrange > 256) {
        xrange >>= 1;
        ++joyshrx[1];
      }
      while (yrange > 256) {
        yrange >>= 1;
        ++joyshry[1];
      }
    }
    else {
      joytype[1] = DEVICE_NONE; // do not use
    }
  }
}

// JoyUpdateTrimViaKey
// This will adjust the trim by one "tick" according to the keypad.
//
// Entry:
void JoyUpdateTrimViaKey(int virtkey) {  // Adjust trim?
  switch (virtkey) {
    case SDLK_DOWN:
#ifdef SDL2
    case SDLK_KP_2:
#else
    case SDLK_KP2:
#endif
      if (g_nPdlTrimY < 64) {
        g_nPdlTrimY++;
      }
      break;
#ifdef SDL2
    case SDLK_KP_4:
#else
    case SDLK_KP4:
#endif
    case SDLK_LEFT:
      if (g_nPdlTrimX > -64) {
        g_nPdlTrimX--;
      }
      break;

#ifdef SDL2
    case SDLK_KP_6:
#else
    case SDLK_KP6:
#endif
    case SDLK_RIGHT:
      if (g_nPdlTrimX < 64) {
        g_nPdlTrimX++;
      }
      break;
#ifdef SDL2
    case SDLK_KP_8:
#else
    case SDLK_KP8:
#endif
    case SDLK_UP:
      if (g_nPdlTrimY > -64) {
        g_nPdlTrimY--;
      }
      break;
#ifdef SDL2
    case SDLK_KP_5:
#else
    case SDLK_KP5:
#endif
    case SDLK_CLEAR:
      g_nPdlTrimX = g_nPdlTrimY = 0;
      break;
      default:
      break;
  }
}

bool JoyProcessKey(int virtkey, bool extended, bool down, bool autorep) {
#if 0
  // GPH Apple as button: This is confusing and results in situations where
  // the solid-apple key (RALT) never gets released.  Going to comment out for
  // now and solicit request for comment.
  bool isALT = ((virtkey == SDLK_LALT) | (virtkey == SDLK_RALT)); //if either ALT key pressed
  if ((joyinfo[joytype[0]].device != DEVICE_KEYBOARD) && (joyinfo[joytype[1]].device != DEVICE_KEYBOARD) && (!isALT)) {
    return 0;
  }
#endif
  // Joystick # which is being emulated by keyboard
  int nJoyNum = (joyinfo[joytype[0]].device == DEVICE_KEYBOARD) ? 0 : 1;
  int nCenteringType = joyinfo[joytype[nJoyNum]].mode;  // MODE_STANDARD or MODE_CENTERING

  bool keychange = !extended;
#if 0
  // GPH Apple-as-button confusion
  if (isALT) {
    keychange = 1;
    keydown[JK_OPENAPPLE + (extended != 0)] = down;
  }
  else
#endif
  if (!extended) {
#ifdef SDL2
    if ((virtkey >= SDLK_KP_1) && (virtkey <= SDLK_KP_9)) {
      keydown[virtkey - SDLK_KP_1] = down;
#else
    if ((virtkey >= SDLK_KP1) && (virtkey <= SDLK_KP9)) {
      keydown[virtkey - SDLK_KP1] = down;
#endif
    } else {
      switch (virtkey) {
#ifdef SDL2
        case SDLK_KP_1:
#else
        case SDLK_KP1:
#endif
        case SDLK_END:
          keydown[0] = down;
          break;
#ifdef SDL2
        case SDLK_KP_2:
#else
        case SDLK_KP2:
#endif
        case SDLK_DOWN:
          keydown[1] = down;
          break;
#ifdef SDL2
        case SDLK_KP_3:
#else
        case SDLK_KP3:
#endif
        case SDLK_PAGEDOWN:
          keydown[2] = down;
          break;
#ifdef SDL2
        case SDLK_KP_4:
#else
        case SDLK_KP4:
#endif
        case SDLK_LEFT:
          keydown[3] = down;
          break;
#ifdef SDL2
        case SDLK_KP_5:
#else
        case SDLK_KP5:
#endif
        case SDLK_CLEAR:
          keydown[4] = down;
          break;
#ifdef SDL2
        case SDLK_KP_6:
#else
        case SDLK_KP6:
#endif
        case SDLK_RIGHT:
          keydown[5] = down;
          break;
#ifdef SDL2
        case SDLK_KP_7:
#else
        case SDLK_KP7:
#endif
        case SDLK_HOME:
          keydown[6] = down;
          break;
#ifdef SDL2
        case SDLK_KP_8:
#else
        case SDLK_KP8:
#endif
        case SDLK_UP:
          keydown[7] = down;
          break;
#ifdef SDL2
        case SDLK_KP_9:
#else
        case SDLK_KP9:
#endif
        case SDLK_PAGEUP:
          keydown[8] = down;
          break;
#ifdef SDL2
        case SDLK_KP_0:
#else
        case SDLK_KP0:
#endif
        case SDLK_INSERT:
          keydown[9] = down;
          break;  // Button #0
        case SDLK_KP_PERIOD:
        case SDLK_DELETE:
          keydown[10] = down;
          break;  // Button #1
        default:
          keychange = 0;
          break;
      }
    }
  }

  if (keychange) {
    // Is it a joystick button 0 or 1 (open-apple or solid apple)?
#ifdef SDL2
    if ((virtkey == SDLK_KP_0) || (virtkey == SDLK_INSERT)) {
#else
    if ((virtkey == SDLK_KP0) || (virtkey == SDLK_INSERT)) {
#endif
      // It's a joystick button...
      if (down) {
        if (joyinfo[joytype[1]].device != DEVICE_KEYBOARD) {
          buttonlatch[0] = BUTTONTIME;
        } else if (joyinfo[joytype[1]].device != DEVICE_NONE) {
          buttonlatch[2] = BUTTONTIME;
          buttonlatch[1] = BUTTONTIME;  // Re-map this button when emulating a 2nd Apple joystick
        }
      }
    } else if ((virtkey == SDLK_KP_PERIOD) || (virtkey == SDLK_DELETE)) {
      // It is joystick button pressed from keypad "." or "delete"...
      if (down) {
        if (joyinfo[joytype[1]].device != DEVICE_KEYBOARD) {
          buttonlatch[1] = BUTTONTIME;
        }
      }
    } else if ((down && !autorep) || (nCenteringType == MODE_CENTERING)) {
      // It is the keypad direction keys 0-9; calculate quantitized
      // PDL(0) and PDL(1) values by taking the mean.
      int xsum = 0;
      int ysum = 0;
      int key_idx = 0;
      int keydown_count = 0;

      // Special-case for corners by side, e.g. left and up, right and down.
      // Convert (for example) left ("4" key) and down ("2") key to the "1" i
      // key (index 0).
      // We are not going to overwrite the keydown status here,
      // but convert and override the calculated value.
      static int corner_convert_lookup[16] =      // 2 ^ 4 key combinations:
      {                                           // "2", "4", "6", and "8"
        -1, -1, -1, 8,
        -1, 6, -1, -1,
        -1, -1, 2, -1,
        0, -1, -1, -1
      };

      int corner_override_idx = -1;
      int corner_idx =
          ((int)(0==keydown[1] /*"2"*/))
        | ((int)(0==keydown[3] /*"4"*/)<<1)
        | ((int)(0==keydown[5] /*"6"*/)<<2)
        | ((int)(0==keydown[7])/*"8"*/<<3);
      if ((corner_override_idx = corner_convert_lookup[corner_idx]) >= 0) {
        xsum = keyvalue[corner_override_idx].x;
        ysum = keyvalue[corner_override_idx].y;
        keydown_count = 1;
      } else {
        // Get the quantitized sum
        while (key_idx < 9) {
          if (keydown[key_idx]) {
            keydown_count++;
            xsum += keyvalue[key_idx].x;
            ysum += keyvalue[key_idx].y;
          }
          key_idx++;
        }
      }
      if (keydown_count) {
        // Get the x mean from the sum
        xpos[nJoyNum] = (xsum / keydown_count) + PDL_CENTRAL + g_nPdlTrimX;
        ypos[nJoyNum] = (ysum / keydown_count) + PDL_CENTRAL + g_nPdlTrimY;
      } else {
        // Can this ever happen?  Yes, in a key-up.
        // Example: was pressing and holding "4" to go left, let up "4" key.
        xpos[nJoyNum] = PDL_CENTRAL + g_nPdlTrimX;
        ypos[nJoyNum] = PDL_CENTRAL + g_nPdlTrimY;
      }
    }
  }

  return keychange;
}

unsigned char JoyReadButton(unsigned short, unsigned short address, unsigned char, unsigned char, ULONG nCyclesLeft) {
  address &= 0xFF;

  if (joyinfo[joytype[0]].device == DEVICE_JOYSTICK) {
    CheckJoystick0();
  }
  if (joyinfo[joytype[1]].device == DEVICE_JOYSTICK) {
    CheckJoystick1();
  }

  bool pressed = 0;
  switch (address) {

    case 0x61:
      pressed = (buttonlatch[0] || joybutton[0] || setbutton[0] || keydown[JK_OPENAPPLE]);
      if (joyinfo[joytype[1]].device != DEVICE_KEYBOARD)
        pressed = (pressed || keydown[JK_BUTTON0]);
      buttonlatch[0] = 0;
      break;

    case 0x62:
      pressed = (buttonlatch[1] || joybutton[1] || setbutton[1] || keydown[JK_CLOSEDAPPLE]);
      if (joyinfo[joytype[1]].device != DEVICE_KEYBOARD)
        pressed = (pressed || keydown[JK_BUTTON1]);
      buttonlatch[1] = 0;
      break;

    case 0x63:
      KeybUpdateCtrlShiftStatus();
      pressed = (buttonlatch[2] || joybutton[2] || setbutton[2] || (g_bShiftKey));// SHIFT is PRESSED
      buttonlatch[2] = 0;
      break;

  }
  return MemReadFloatingBus(pressed, nCyclesLeft);
}

// PREAD:    ; $FB1E
//  AD 70 C0 : (4) LDA $C070
//  A0 00    : (2) LDA #$00
//  EA       : (2) NOP
//  EA       : (2) NOP
// Lbl1:            ; 11 cycles is the normal duration of the loop
//  BD 64 C0 : (4) LDA $C064,X
//  10 04    : (2) BPL Lbl2    ; NB. 3 cycles if branch taken (not likely)
//  C8       : (2) INY
//  D0 F8    : (3) BNE Lbl1    ; NB. 2 cycles if branck not taken (not likely)
//  88       : (2) DEY
// Lbl2:
//  60       : (6) RTS

static const double PDL_CNTR_INTERVAL = 2816.0 / 255.0;  // 11.04 (From KEGS)

unsigned char JoyReadPosition(unsigned short programcounter, unsigned short address, unsigned char, unsigned char, ULONG nCyclesLeft) {
  int nJoyNum = (address & 2) ? 1 : 0;  // $C064..$C067

  CpuCalcCycles(nCyclesLeft);

  ULONG nPdlPos = (address & 1) ? ypos[nJoyNum] : xpos[nJoyNum];

  // This is from KEGS. It helps games like Championship Lode Runner & Boulderdash
  // GPH: Taking this out.   It's illogical and this is an 8-bit emulator, not a IIGS
  //if(nPdlPos >= 255)
  //  nPdlPos = 280;

  bool nPdlCntrActive = (g_nCumulativeCycles <= (g_nJoyCntrResetCycle + (UINT64) ((double) nPdlPos * PDL_CNTR_INTERVAL)));

  return MemReadFloatingBus(nPdlCntrActive, nCyclesLeft);
}

void JoyReset() {
  int loop = 0;
  while (loop < JK_MAX)
    keydown[loop++] = false; // clear all joystick buttons and axis states
}

unsigned char JoyResetPosition(unsigned short, unsigned short, unsigned char, unsigned char, ULONG nCyclesLeft) {
  CpuCalcCycles(nCyclesLeft);
  g_nJoyCntrResetCycle = g_nCumulativeCycles;

  if (joyinfo[joytype[0]].device == DEVICE_JOYSTICK)
    CheckJoystick0();
  if (joyinfo[joytype[1]].device == DEVICE_JOYSTICK)
    CheckJoystick1();

  return MemReadFloatingBus(nCyclesLeft);
}

// Called when mouse is being used as a joystick && mouse button changes
void JoySetButton(eBUTTON number, eBUTTONSTATE down) {
  if (number > 1) { // Sanity check on mouse button #
    return;
  }

  // If 2nd joystick is enabled, then both joysticks only have 1 button
  if ((joyinfo[joytype[1]].device != DEVICE_NONE) && (number != 0)) {
    return;
  }

  // If it is 2nd joystick that is being emulated with mouse, then re-map button #
  if (joyinfo[joytype[1]].device == DEVICE_MOUSE) {
    number = BUTTON1;  // 2nd joystick controls Apple button #1
  }

  setbutton[number] = down;

  if (down) {
    buttonlatch[number] = BUTTONTIME;
  }
}

// Set new joystick type
bool JoySetEmulationType(unsigned int newType, int nJoystickNumber) {
  if (joytype[nJoystickNumber] == newType) {
    return 1;  // Already set to this type. Return OK.
  }

  if (joyinfo[newType].device == DEVICE_JOYSTICK) {
    if (SDL_NumJoysticks() <= nJoystickNumber) {
      fprintf(stderr, "Can not find joystick #%d - disabling it\n", nJoystickNumber);
      return 0;
    }
  } else if ((joyinfo[newType].device == DEVICE_MOUSE) && (joyinfo[joytype[nJoystickNumber]].device != DEVICE_MOUSE)) {
    if (sg_Mouse.Active()) {
      fprintf(stderr, "Mouse interface card is enabled - unable to use mouse for joystick emulation.\n");
      return 0;
    }
    printf("To release mouse cursor, press Ctrl + left mouse button.\n");
  }
  joytype[nJoystickNumber] = newType;
  JoyInitialize();
  JoyReset();
  return 1;
}


// Called when mouse is being used as a joystick && mouse position changes
void JoySetPosition(int xvalue, int xrange, int yvalue, int yrange) {
  int nJoyNum = (joyinfo[joytype[0]].device == DEVICE_MOUSE) ? 0 : 1;
  xpos[nJoyNum] = (xvalue * 255) / xrange;
  ypos[nJoyNum] = (yvalue * 255) / yrange;
}

void JoyUpdatePosition() {
  if (buttonlatch[0]) {
    --buttonlatch[0];
  }
  if (buttonlatch[1]) {
    --buttonlatch[1];
  }
  if (buttonlatch[2]) {
    --buttonlatch[2];
  }
}

bool JoyUsingMouse() {
  return (joyinfo[joytype[0]].device == DEVICE_MOUSE) || (joyinfo[joytype[1]].device == DEVICE_MOUSE);
}

void JoySetTrim(short nValue, bool bAxisX) {
  if (bAxisX) {
    g_nPdlTrimX = nValue;
  } else {
    g_nPdlTrimY = nValue;
  }

  int nJoyNum = -1;

  if (joyinfo[joytype[0]].device == DEVICE_KEYBOARD) {
    nJoyNum = 0;
  } else if (joyinfo[joytype[1]].device == DEVICE_KEYBOARD) {
    nJoyNum = 1;
  }

  if (nJoyNum >= 0) {
    xpos[nJoyNum] = PDL_CENTRAL + g_nPdlTrimX;
    ypos[nJoyNum] = PDL_CENTRAL + g_nPdlTrimY;
  }
}

short JoyGetTrim(bool bAxisX) {
  return bAxisX ? g_nPdlTrimX : g_nPdlTrimY;
}

unsigned int JoyGetSnapshot(SS_IO_Joystick *pSS) {
  pSS->g_nJoyCntrResetCycle = g_nJoyCntrResetCycle;
  return 0;
}

unsigned int JoySetSnapshot(SS_IO_Joystick *pSS) {
  g_nJoyCntrResetCycle = pSS->g_nJoyCntrResetCycle;
  return 0;
}
