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

/* Description: Keyboard emulation
 *
 * Author: Various
 */

/* Adaptation for SDL and POSIX (l) by beom beotiger, Nov-Dec 2007 */


#include "stdafx.h"
#include <iostream>

#define KEY_OLD

#ifdef SDL2
typedef enum {
        /* International keyboard syms */
        SDLK_WORLD_0        = 160,            /* 0xA0 */
        SDLK_WORLD_1        = 161,
        SDLK_WORLD_2        = 162,
        SDLK_WORLD_3        = 163,
        SDLK_WORLD_4        = 164,
        SDLK_WORLD_5        = 165,
        SDLK_WORLD_6        = 166,
        SDLK_WORLD_7        = 167,
        SDLK_WORLD_8        = 168,
        SDLK_WORLD_9        = 169,
        SDLK_WORLD_10      = 170,
        SDLK_WORLD_11      = 171,
        SDLK_WORLD_12      = 172,
        SDLK_WORLD_13      = 173,
        SDLK_WORLD_14      = 174,
        SDLK_WORLD_15      = 175,
        SDLK_WORLD_16      = 176,
        SDLK_WORLD_17      = 177,
        SDLK_WORLD_18      = 178,
        SDLK_WORLD_19      = 179,
        SDLK_WORLD_20      = 180,
        SDLK_WORLD_21      = 181,
        SDLK_WORLD_22      = 182,
        SDLK_WORLD_23      = 183,
        SDLK_WORLD_24      = 184,
        SDLK_WORLD_25      = 185,
        SDLK_WORLD_26      = 186,
        SDLK_WORLD_27      = 187,
        SDLK_WORLD_28      = 188,
        SDLK_WORLD_29      = 189,
        SDLK_WORLD_30      = 190,
        SDLK_WORLD_31      = 191,
        SDLK_WORLD_32      = 192,
        SDLK_WORLD_33      = 193,
        SDLK_WORLD_34      = 194,
        SDLK_WORLD_35      = 195,
        SDLK_WORLD_36      = 196,
        SDLK_WORLD_37      = 197,
        SDLK_WORLD_38      = 198,
        SDLK_WORLD_39      = 199,
        SDLK_WORLD_40      = 200,
        SDLK_WORLD_41      = 201,
        SDLK_WORLD_42      = 202,
        SDLK_WORLD_43      = 203,
        SDLK_WORLD_44      = 204,
        SDLK_WORLD_45      = 205,
        SDLK_WORLD_46      = 206,
        SDLK_WORLD_47      = 207,
        SDLK_WORLD_48      = 208,
        SDLK_WORLD_49      = 209,
        SDLK_WORLD_50      = 210,
        SDLK_WORLD_51      = 211,
        SDLK_WORLD_52      = 212,
        SDLK_WORLD_53      = 213,
        SDLK_WORLD_54      = 214,
        SDLK_WORLD_55      = 215,
        SDLK_WORLD_56      = 216,
        SDLK_WORLD_57      = 217,
        SDLK_WORLD_58      = 218,
        SDLK_WORLD_59      = 219,
        SDLK_WORLD_60      = 220,
        SDLK_WORLD_61      = 221,
        SDLK_WORLD_62      = 222,
        SDLK_WORLD_63      = 223,
        SDLK_WORLD_64      = 224,
        SDLK_WORLD_65      = 225,
        SDLK_WORLD_66      = 226,
        SDLK_WORLD_67      = 227,
        SDLK_WORLD_68      = 228,
        SDLK_WORLD_69      = 229,
        SDLK_WORLD_70      = 230,
        SDLK_WORLD_71      = 231,
        SDLK_WORLD_72      = 232,
        SDLK_WORLD_73      = 233,
        SDLK_WORLD_74      = 234,
        SDLK_WORLD_75      = 235,
        SDLK_WORLD_76      = 236,
        SDLK_WORLD_77      = 237,
        SDLK_WORLD_78      = 238,
        SDLK_WORLD_79      = 239,
        SDLK_WORLD_80      = 240,
        SDLK_WORLD_81      = 241,
        SDLK_WORLD_82      = 242,
        SDLK_WORLD_83      = 243,
        SDLK_WORLD_84      = 244,
        SDLK_WORLD_85      = 245,
        SDLK_WORLD_86      = 246,
        SDLK_WORLD_87      = 247,
        SDLK_WORLD_88      = 248,
        SDLK_WORLD_89      = 249,
        SDLK_WORLD_90      = 250,
        SDLK_WORLD_91      = 251,
        SDLK_WORLD_92      = 252,
        SDLK_WORLD_93      = 253,
        SDLK_WORLD_94      = 254,
        SDLK_WORLD_95      = 255          /* 0xFF */
} SDLKey_SDL2;
#endif

bool g_bShiftKey = false;
bool g_bCtrlKey = false;
bool g_bAltKey = false;
static bool g_bCapsLock = true;
static int lastvirtkey = 0;  // Current PC keycode
static unsigned char keycode = 0;  // Current Apple keycode
static unsigned int keyboardqueries = 0;

KeybLanguage g_KeyboardLanguage = English_US; // default keyboard language
bool         g_KeyboardRockerSwitch = false;  // keyboard/video ROM charset toggle switch (Euro-Apple //e)

#ifdef KEY_OLD
// Original
static bool keywaiting = 0;
#else
// Buffered key input:
// - Needed on faster PCs where aliasing occurs during short/fast bursts of 6502 code.
// - Keyboard only sampled during 6502 execution, so if it's run too fast then key presses will be missed.
const int KEY_BUFFER_MIN_SIZE = 1;
const int KEY_BUFFER_MAX_SIZE = 2;
static int g_nKeyBufferSize = KEY_BUFFER_MAX_SIZE;  // Circ key buffer size
static int g_nNextInIdx = 0;
static int g_nNextOutIdx = 0;
static int g_nKeyBufferCnt = 0;

static struct {
  int nVirtKey;
  unsigned char nAppleKey;
} g_nKeyBuffer[KEY_BUFFER_MAX_SIZE];
#endif

static unsigned char g_nLastKey = 0x00;

// All globally accessible functions are below this line

void KeybReset() {
  #ifdef KEY_OLD
  keywaiting = 0;
  #else
  g_nNextInIdx = 0;
  g_nNextOutIdx = 0;
  g_nKeyBufferCnt = 0;
  g_nLastKey = 0x00;

  g_nKeyBufferSize = g_bKeybBufferEnable ? KEY_BUFFER_MAX_SIZE : KEY_BUFFER_MIN_SIZE;
  #endif
}

bool KeybGetAltStatus() {
  return g_bAltKey;
}

bool KeybGetCapsStatus() {
  return g_bCapsLock;
}

bool KeybGetCtrlStatus() {
  return g_bCtrlKey;
}

bool KeybGetShiftStatus() {
  return g_bShiftKey;
}

void KeybUpdateCtrlShiftStatus() {
  Uint8 *keys;
#ifdef SDL2
  keys = (Uint8*)SDL_GetKeyboardState(NULL);
#else
  keys = SDL_GetKeyState(NULL);
#endif

#ifdef SDL2
  g_bShiftKey = (keys[SDL_SCANCODE_LSHIFT] | keys[SDL_SCANCODE_RSHIFT]); // 0x8000 KF_UP   SHIFT
  g_bCtrlKey = (keys[SDL_SCANCODE_LCTRL] | keys[SDL_SCANCODE_RCTRL]);  // CTRL
  g_bAltKey = (keys[SDL_SCANCODE_LALT] | keys[SDL_SCANCODE_RALT]);  // ALT
#else
  g_bShiftKey = (keys[SDLK_LSHIFT] | keys[SDLK_RSHIFT]); // 0x8000 KF_UP   SHIFT
  g_bCtrlKey = (keys[SDLK_LCTRL] | keys[SDLK_RCTRL]);  // CTRL
  g_bAltKey = (keys[SDLK_LALT] | keys[SDLK_RALT]);  // ALT
#endif
}

unsigned char KeybGetKeycode()    // Used by MemCheckPaging() & VideoCheckMode()
{
  return keycode;
}

unsigned int KeybGetNumQueries()  // Used in determining 'idleness' of Apple system
{
  unsigned int result = keyboardqueries;
  keyboardqueries = 0;
  return result;
}

// decode keys for US-keyboard
int KeybDecodeKeyUS(int key)
{
  if (g_bShiftKey) {
    // convert shifted keys according to Apple // specific US-keyboard layout
    switch (key) {
      case '1':
        return '!';
      case '2':
        return '@';
      case '3':
        return '#';
      case '4':
        return '$';
      case '5':
        return '%';
      case '6':
        return '^';
      case '7':
        return '&';
      case '8':
        return '*';
      case '9':
        return '(';
      case '0':
        return ')';
      case '`':
        return '~';
      case '-':
        return '_';
      case '=':
        return '+';
      case '\\':
        return '|';
      case '[':
        return '{';
      case ']':
        return '}';
      case ';':
        return ':';
      case '\'':
        return '"';
      case ',':
        return '<';
      case '.':
        return '>';
      case '/':
        return '?';
      default:
        break;
    }
  }
  return key;
}

/* Note on keyboard mapping: unfortunately SDL1.2 does not support proper (standardized) scan codes,
 * which identify the physical location of a key on the host keyboard. With SDL1.2 only the key code
 * is usable (which is the character after the host's conversion). Only SDL2 introduced proper scan
 * code support.
 * So, with SDL 1.2 we must rely on the key codes. The following code assumes the host PC's keyboard
 * layout matches the selected target language (i.e. host provides German/French/... converted key codes,
 * when the Apple II emulation is set to German/French/... respectively. The mapping will be incorrect
 * if a host PC with a US (or French) keyboard selected a German/.. Apple II keyboard.
 * The mapping should be reworked once LinApple moved to SDL2.0, so we can support a correct keyboard
 * mapping independently of the host PC's actual keyboard.
 */

// decode keys for UK keyboard to Apple characters
int KeybDecodeKeyUK(int key)
{
  /* UK is almost identical to US layout, except for '#' vs 'the pound' character.
   * Pound and '#' share the same Apple II ASCII code though - and their non-shifted
   * keycode on the host PC is also identical - so no special keyboard mapping is
   * required to achieve the proper mapping. We just rely on the US-conversion
   * (no need to consider the rocker switch here - it only affects video output in
   * UK mode). */
  return KeybDecodeKeyUS(key);
}

// decode keys for German keyboard to Apple characters
int KeybDecodeKeyDE(int key)
{
  //printf("German key: %i\n", key);

  if (!g_KeyboardRockerSwitch)
  {
    /* Rocker switch in US-character-set mode: do a 'reverse' keyboard mapping, assuming the host PC's
     * native keyboard has a German keyboard layout, so converting back to the respective character of a
     * US keyboard layout. */

    // swap Y and Z keys
    if (key == 'z')
      return 'y';
    else
    if (key == 'y')
      return 'z';
    if ((key>='0')&&(key<='9'))
    {
      // use US mapping when shift-0..9 is selected
      return KeybDecodeKeyUS(key);
    }

    // reverse mapping of further German keyboard keys to appropriate US-keyboard keys, also considering the ShiftKey
    switch(key)
    {
      case SDLK_WORLD_63: // German SZ
        return (g_bShiftKey) ? '_' : '-';
      case SDLK_WORLD_20: // key next to the German SZ
        return (g_bShiftKey) ? '+' : '=';
      case SDLK_WORLD_68: // German umlaut-a
        return (g_bShiftKey) ? '"' : '\'';
      case SDLK_WORLD_86: // German umlaut-o
        return (g_bShiftKey) ? ':' : ';';
      case SDLK_WORLD_92: // German umlaut-u
        return (g_bShiftKey) ? '{' : '[';
      case '+':
        return (g_bShiftKey) ? '}' : ']';
      case '#':
        return (g_bShiftKey) ? '~' : '`';
      case ',':
        return (g_bShiftKey) ? '<' : ',';
        break;
      case '.':
        return (g_bShiftKey) ? '>' : '.';
        break;
      case '-':
        return (g_bShiftKey) ? '?' : '/';
        break;
      case '<':
        return (g_bShiftKey) ? '|' : '\\';
      default:
        break;
    }

    return key;
  }

  // rocker switch is in German-character-set mode

  if (g_bShiftKey) {
    // map shifted keys to appropriate Apple II keys
    switch (key) {
      case '1':
        return '!';
      case '2':
        return '"';
      case '3':
        return '@';
      case '4':
        return '$';
      case '5':
        return '%';
      case '6':
        return '&';
      case '7':
        return '/';
      case '8':
        return '(';
      case '9':
        return ')';
      case '0':
        return '=';
      case '-':
        return '_';
      case '+':
        return '*';
      case '#':
        return '^';
      case ',':
        return ';';
      case '.':
        return ':';
      case '<':
        return '>';
      default:
        break;
    }
  }
  // map further keys
  switch (key) {
    case SDLK_WORLD_63: // German S
      return (g_bShiftKey) ? '?' : '~';
    case SDLK_WORLD_20: // key next to the German SZ
      return (g_bShiftKey) ? '`' : '\'';
    case SDLK_WORLD_68: // German umlaut-a
      return (g_bShiftKey || g_bCapsLock) ? '[' : '{';
    case SDLK_WORLD_86: // German umlaut-o
      return (g_bShiftKey || g_bCapsLock) ? '\\' : '|';
    case SDLK_WORLD_92: // German umlaut-u
      return (g_bShiftKey || g_bCapsLock) ? ']' : '}';
    default:
      break;
  }
  return key;
}

// decode keys for French keyboard to Apple characters
int KeybDecodeKeyFR(int key)
{
  //printf("French key: %i\n", key);
  if (!g_KeyboardRockerSwitch)
  {
    /* Rocker switch in US-character-set mode: do a 'reverse' keyboard mapping, assuming the host PC's
     * native keyboard has a French keyboard layout, so converting back to the respective character of a
     * US keyboard layout. */
    switch(key)
    {
      case '&':
        return (g_bShiftKey) ? '!' : '1';
      case SDLK_WORLD_73:
        return (g_bShiftKey) ? '@' : '2';
      case '"':
        return (g_bShiftKey) ? '#' : '3';
      case '\'':
        return (g_bShiftKey) ? '$' : '4';
      case '(':
        return (g_bShiftKey) ? '%' : '5';
      case '-':
        return (g_bShiftKey) ? '^' : '6';
      case SDLK_WORLD_72:
        return (g_bShiftKey) ? '&' : '7';
      case '_':
        return (g_bShiftKey) ? '*' : '8';
      case SDLK_WORLD_71:
        return (g_bShiftKey) ? '(' : '9';
      case SDLK_WORLD_64:
        return (g_bShiftKey) ? ')' : '0';
      case ')':
        return (g_bShiftKey) ? '_' : '-';
      case '=':
        return (g_bShiftKey) ? '+' : '=';
      case 'a':
        return 'q';
      case 'z':
        return 'w';
      case '^':
        return (g_bShiftKey) ? '{' : '[';
      case '$':
        return (g_bShiftKey) ? '}' : ']';
      case 'q':
        return 'a';
      case 'm':
        return (g_bShiftKey) ? ':' : ';';
      case SDLK_WORLD_89:
        return (g_bShiftKey) ? '"' : '\'';
      case '*':
        return '~';
      case '<':
        return (g_bShiftKey) ? '|' : '\\';
      case 'w':
        return 'z';
      case ',':
        return (g_bShiftKey | g_bCapsLock) ? 'M' : 'm';
      case ';':
        return (g_bShiftKey) ? '<' : ',';
      case ':':
        return (g_bShiftKey) ? '>' : '.';
      case '!':
        return (g_bShiftKey) ? '?' : '/';
      default:
        break;
    }
    return key;
  }

  // rocker switch is in French-character-set mode
  switch(key)
  {
    case '&':
      return (g_bShiftKey) ? '1' : '&';
    case SDLK_WORLD_73:
      return (g_bShiftKey) ? '2' : '{';
    case '"':
      return (g_bShiftKey) ? '3' : '"';
    case '\'':
      return (g_bShiftKey) ? '4' : '\'';
    case '(':
      return (g_bShiftKey) ? '5' : '(';
    case '-':
      return (g_bShiftKey) ? '6' : ']';
    case SDLK_WORLD_72:
      return (g_bShiftKey) ? '7' : '}';
    case '_':
      return (g_bShiftKey) ? '8' : '!';
    case SDLK_WORLD_71:
      return (g_bShiftKey) ? '9' : '\\';
    case SDLK_WORLD_64:
      return (g_bShiftKey) ? '0' : '@';
    case ')':
      return (g_bShiftKey) ? '[' : ')';
    case '=':
      return (g_bShiftKey) ? '_' : '-';
    case '^':
      return (g_bShiftKey) ? '~' : '^';
    case '$':
      return (g_bShiftKey) ? '*' : '$';
    case SDLK_WORLD_89:
      return (g_bShiftKey) ? '%' : '|';
    case '*':
      return (g_bShiftKey) ? '`' : '#';
    case '<':
      return (g_bShiftKey) ? '>' : '<';
    case ',':
      return (g_bShiftKey) ? '?' : ',';
    case ';':
      return (g_bShiftKey) ? '.' : ';';
    case ':':
      return (g_bShiftKey) ? '/' : ':';
    case '!':
      return (g_bShiftKey) ? '+' : '=';
    default:
      break;
  }

  return key;
}

// decode keycode for selected keyboard
int KeybDecodeKey(int key)
{
  KeybUpdateCtrlShiftStatus();

  // language dependent keyboard mappings
  switch(g_KeyboardLanguage)
  {
    case English_UK:
      key = KeybDecodeKeyUK(key);
      break;
    case French_FR:
      key = KeybDecodeKeyFR(key);
      break;
    case German_DE:
      key = KeybDecodeKeyDE(key);
      break;
    case English_US:
    default:
      key = KeybDecodeKeyUS(key);
      break;
  }

  return key;
}

void KeybQueueKeypress(int key, bool bASCII)
{
  // language dependent keyboard mappings
  key = KeybDecodeKey(key);

  if ((key>=0)&&(key < 0x80)) {
    if (g_bCtrlKey) {
      if (key >= 'a' && key <= 'z') {
        key = key - 'a' + 1;
      } else {
        switch (key) {
          case '\\':
            key = 28;
            break;
          case '[' :
            key = 27;
            break;
          case ']' :
            key = 29;
            break;
          case SDLK_RETURN:
            key = 10;
            break;

          default:
            break;
        }
      }
    }

    if (!IS_APPLE2) {
      if (g_bCapsLock && (key >= 'a') && (key <= 'z')) {
        keycode = key - 32;
      } else {
        keycode = key;
      }
    } else {
      if (key >= '`') {
        keycode = key - 32;
      } else {
        keycode = key;
      }
    }
    lastvirtkey = key;
  } else {
    if (IS_APPLE2) {
      switch (key) {
        case SDLK_LEFT:
          keycode = 0x08;
          break;
        case SDLK_UP:
          keycode = 0x0D;
          break;
        case SDLK_RIGHT:
          keycode = 0x15;
          break;
        case SDLK_DOWN:
          keycode = 0x2F;
          break;
        case SDLK_DELETE:
          keycode = 0x00;
          break;
        default:
          return;
      }
    } else {
      switch (key) {
        case SDLK_LEFT:
          keycode = 0x08;
          break;
        case SDLK_UP:
          keycode = 0x0B;
          break;
        case SDLK_RIGHT:
          keycode = 0x15;
          break;
        case SDLK_DOWN:
          keycode = 0x0A;
          break;
        case SDLK_DELETE:
          keycode = 0x7F;
          break;
        default:
          return;
      }
    }
    lastvirtkey = key;
  }

  #ifdef KEY_OLD
  keywaiting = 1;
  #else
  bool bOverflow = false;

  if(g_nKeyBufferCnt < g_nKeyBufferSize) {
    g_nKeyBufferCnt++;
  } else {
    bOverflow = true;
  }

  g_nKeyBuffer[g_nNextInIdx].nVirtKey = lastvirtkey;
  g_nKeyBuffer[g_nNextInIdx].nAppleKey = keycode;
  g_nNextInIdx = (g_nNextInIdx + 1) % g_nKeyBufferSize;

  if(bOverflow) {
    g_nNextOutIdx = (g_nNextOutIdx + 1) % g_nKeyBufferSize;
  }
  #endif
}

unsigned char KeybReadData(unsigned short, unsigned short, unsigned char, unsigned char, ULONG) {
  keyboardqueries++;

  #ifdef KEY_OLD
  return keycode | (keywaiting ? 0x80 : 0);
  #else
  unsigned char nKey = g_nKeyBufferCnt ? 0x80 : 0;
  if(g_nKeyBufferCnt)
  {
    nKey |= g_nKeyBuffer[g_nNextOutIdx].nAppleKey;
    g_nLastKey = g_nKeyBuffer[g_nNextOutIdx].nAppleKey;
  }
  else
  {
    nKey |= g_nLastKey;
  }
  return nKey;
  #endif
}

unsigned char KeybReadFlag(unsigned short, unsigned short, unsigned char, unsigned char, ULONG) {
  keyboardqueries++;

  Uint8 *keys;
#ifdef SDL2
  keys = (Uint8*)SDL_GetKeyboardState(NULL);
#else
  keys = SDL_GetKeyState(NULL);
#endif
  #ifdef KEY_OLD
  keywaiting = 0;
  return keycode | (keys[lastvirtkey] ? 0x80 : 0);
  #else
  unsigned char nKey = (keys[g_nKeyBuffer[g_nNextOutIdx].nVirtKey]) ? 0x80 : 0;
  nKey |= g_nKeyBuffer[g_nNextOutIdx].nAppleKey;
  if(g_nKeyBufferCnt) {
    g_nKeyBufferCnt--;
    g_nNextOutIdx = (g_nNextOutIdx + 1) % g_nKeyBufferSize;
  }
  return nKey;
  #endif
}

void KeybToggleCapsLock()
{
  if (!IS_APPLE2) {
    g_bCapsLock = !g_bCapsLock;
    FrameRefreshStatus(DRAW_LEDS);
  }
}

unsigned int KeybGetSnapshot(SS_IO_Keyboard *pSS) {
  pSS->keyboardqueries = keyboardqueries;
  pSS->nLastKey = g_nLastKey;
  return 0;
}

unsigned int KeybSetSnapshot(SS_IO_Keyboard *pSS) {
  keyboardqueries = pSS->keyboardqueries;
  g_nLastKey = pSS->nLastKey;
  return 0;
}
