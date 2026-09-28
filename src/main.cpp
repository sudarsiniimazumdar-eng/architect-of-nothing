/*
 ====================================================================================================
   ARCHITECT OF NOTHING - 18-BIT RETRO VISUAL NOVEL ENGINE
   Hardware: ST7735 TFT (160x128), 4 Choice Buttons, 1 Toggle Switch (GPIO 15), MOSFET Vibration
 ====================================================================================================
*/

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "driver/gpio.h"

// --------------------------------------------------------------------------------------------------
// 1. HARDWARE PIN DEFINITIONS
// --------------------------------------------------------------------------------------------------
#define TFT_CS                  5
#define TFT_DC                  21
#define TFT_RST                 22

#define BTN_1                   13   // Option 1
#define BTN_2                   12   // Option 2
#define BTN_3                   14   // Option 3
#define BTN_4                   27   // Option 4

#define TOGGLE_PIN              15   // Toggle Switch (System Analysis / Resistance)

#define BUZZER_PIN              4    // Piezoelectric Buzzer (Audio Feedback)
#define VIBRO_PIN               17   // MOSFET Vibration Motor
#define ONBOARD_LED             2    // Status LED

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --------------------------------------------------------------------------------------------------
// 2. STRICT 2-COLOR TEXT HIERARCHY & PALETTE (ST7735 RGB565)
// --------------------------------------------------------------------------------------------------
// Strict 2-Color Text System for Maximum Readability and Minimalist Aesthetic:
#define COL_TXT_PRI             0xFFFF   // Primary: Crisp Pure White (dialogue, feedback, choices)
#define COL_TXT_ACC             0xFFE0   // Accent: Radiant Gold/Amber (titles, names, prompts, numbers)

#define COL_BLACK               0x0000
#define COL_WHITE               0xFFFF
#define COL_DARK_GRAY           0x18C3
#define COL_LIGHT_GRAY          0x6B4D
#define COL_CYAN                0x07FF
#define COL_GREEN               0x07E0
#define COL_RED                 0xF800
#define COL_GOLD                0xFFE0

#define COL_DIALOGUE_BG         0x0821   // Deep Midnight Black-Blue
#define COL_DIALOGUE_BD         0x3186   // Clean Slate Border
#define COL_NAME_BG             0x0000   // Pure Black Header
#define COL_NAME_TXT            COL_TXT_ACC
#define COL_TEXT_MAIN           COL_TXT_PRI
#define COL_ALERT_RED           0xF800
#define COL_ALERT_GOLD          COL_TXT_ACC
#define COL_GRID                0x18C3

// --------------------------------------------------------------------------------------------------
// 3. GAME STRUCTURES & SCENARIO DATABASE (8-STAGE PHILOSOPHICAL ARC)
// --------------------------------------------------------------------------------------------------
enum CharacterID {
  CHAR_YOU,
  CHAR_PARENT,
  CHAR_FRIEND,
  CHAR_PARTNER,
  CHAR_STRANGER,
  CHAR_SYSTEM
};

enum GamePhase {
  PHASE_TITLE,
  PHASE_DIALOGUE,
  PHASE_CHOICE_SELECT,
  PHASE_SYSTEM_REACT,
  PHASE_FINAL_COLLAPSE
};

struct DialogueLine {
  CharacterID speaker;
  const char* speakerName;
  const char* line1;
  const char* line2;
  const char* line3;
};

struct ChoiceOption {
  const char* label;
  int deltaCompliance;
  int deltaResistance;
  int deltaSelfWorth;
  int deltaStatus;
  const char* feedbackComment;
};

struct Scenario {
  int id;
  const char* title;
  int totalPages;
  DialogueLine pages[3];
  ChoiceOption options[4];
  const char* systemEval;
  int vibroDuration;
};

struct GameVariables {
  int compliance = 50;
  int resistance = 10;
  int selfWorth  = 50;
  int socialStatus = 20;
} gVars;

const int TOTAL_SCENARIOS = 8;
const Scenario SCENARIOS[TOTAL_SCENARIOS] = {
  // Scenario 1: The Expectation (Family Conditioning)
  {
    1, "THE EXPECTATION", 2,
    {
      {CHAR_PARENT, "PARENT", "EVERYONE IS WORKING", "HARDER THAN YOU.", "WHY ARE YOU RESTING?"},
      {CHAR_YOU,    "YOU",    "I HAVE BEEN WORKING", "ALL WEEK...", ""},
      {CHAR_PARENT, "PARENT", "RESULTS MATTER.", "NOT EXCUSES.", "DO NOT FALL BEHIND."}
    },
    {
      {"APOLOGIZE & WORK", +4, -3, -2, +2, "Conformity accepted without protest."},
      {"EXPLAIN FATIGUE",  -1, +2, +1, -1, "Vulnerability dismissed as weakness."},
      {"STAY SILENT",      +2, -1, -2,  0, "Guilt quietly internalized."},
      {"WALK AWAY",        -4, +4, +2, -3, "Defiance recorded in system ledger."}
    },
    "EARLY CONDITIONING INITIATED.", 60
  },

  // Scenario 2: The Benchmark (Peer Comparison & Hustle)
  {
    2, "THE BENCHMARK", 2,
    {
      {CHAR_FRIEND, "FRIEND", "I JUST LOGGED 80 HRS.", "MY TITLE GOT UPGRADED.", "WHAT HAVE YOU DONE?"},
      {CHAR_YOU,    "YOU",    "I TRIED TO SLEEP...", "AND READ A BOOK.", ""},
      {CHAR_FRIEND, "FRIEND", "SLEEP DOES NOT PAY.", "WINNERS BUILD EMPIRES.", "KEEP UP OR GET LEFT."}
    },
    {
      {"PRAISE THEIR HUSTLE", +3, -2, -1, +2, "External validation reinforced."},
      {"QUESTION THE COST",   -2, +3, +2, -2, "Value misalignment detected."},
      {"LIE ABOUT PROGRESS",  +4, -2, -3, +3, "Imposter syndrome amplified."},
      {"DISMISS RAT RACE",    -3, +3, +1, -3, "Social alienation triggered."}
    },
    "COMPARISON ENGINE ENGAGED.", 80
  },

  // Scenario 3: The Compromise (Survival vs Ideals)
  {
    3, "THE COMPROMISE", 2,
    {
      {CHAR_PARTNER, "PARTNER", "WE NEED STABILITY.", "RENT IS GOING UP.", "YOUR DREAMS DON'T PAY."},
      {CHAR_YOU,     "YOU",     "WHAT ARE WE", "SURRENDERING?", ""},
      {CHAR_PARTNER, "PARTNER", "JUST YOUR PRIDE.", "EVERYONE COMPROMISES.", "GROW UP."}
    },
    {
      {"TAKE THE SAFE JOB",   +5, -4, -3, +3, "Idealism exchanged for security."},
      {"DEFEND YOUR PASSION", -4, +5, +3, -3, "Domestic friction intensified."},
      {"PROMISE TO TRY BOTH", +2, -1, -2, +1, "Chronic burnout scheduled."},
      {"SURRENDER CONTROL",   +6, -5, -4, +4, "Autonomy voluntarily ceded."}
    },
    "PRAGMATIC SURRENDER LOGGED.", 100
  },

  // Scenario 4: The Metric (Corporate Dehumanization)
  {
    4, "THE METRIC", 2,
    {
      {CHAR_STRANGER, "REVIEWER", "YOUR EFFICIENCY IS", "DOWN BY 4.2 PERCENT.", "METRICS DO NOT LIE."},
      {CHAR_YOU,      "YOU",      "MY MOTHER WAS ILL.", "I DID MY BEST.", ""},
      {CHAR_STRANGER, "REVIEWER", "FEELINGS ARE OFF-GRID.", "ONLY NUMBERS SURVIVE.", "CORRECT THIS."}
    },
    {
      {"ACCEPT PENALTY",      +5, -3, -4, +2, "Subjugation to raw telemetry."},
      {"DEMAND EMPATHY",      -4, +5, +3, -4, "Anomaly: Emotional expectation."},
      {"PROMISE OVERTIME",    +6, -4, -3, +4, "Body offered to quota engine."},
      {"LOG OFF SILENTLY",    -5, +6, +4, -5, "Quiet rebellion registered."}
    },
    "HUMAN DEPRECIATION CONFIRMED.", 110
  },

  // Scenario 5: The Consumption (Hollow Dopamine)
  {
    5, "THE CONSUMPTION", 2,
    {
      {CHAR_FRIEND, "FRIEND", "I BOUGHT THE NEW CAR.", "I STILL FEEL EMPTY...", "BUT IT LOOKS GREAT."},
      {CHAR_YOU,    "YOU",    "DID IT HEAL", "ANYTHING?", ""},
      {CHAR_FRIEND, "FRIEND", "NO. SO I WILL BUY", "THE NEXT MODEL.", "WHAT ELSE IS THERE?"}
    },
    {
      {"ENCOURAGE SHOPPING",  +4, -2, -2, +3, "Hedonic treadmill lubricated."},
      {"SUGGEST THERAPY",     -2, +3, +2, -1, "System bypass attempted."},
      {"ADMIT SAME EMPTINESS", +1, +2, +1,  0, "Mutual despair acknowledged."},
      {"REJECT CONSUMERISM",  -4, +5, +3, -3, "Systemic cycle interrupted."}
    },
    "DOPAMINE DEFICIT ARCHIVED.", 120
  },

  // Scenario 6: The System (Systemic Gaslighting)
  {
    6, "THE SYSTEM", 2,
    {
      {CHAR_SYSTEM, "SYSTEM", "THIS STRUCTURE OFFERS", "INFINITE FREEDOM.", "FAILURE IS YOUR FAULT."},
      {CHAR_YOU,    "YOU",    "EVERY PATH LEADS", "TO EXHAUSTION.", ""},
      {CHAR_SYSTEM, "SYSTEM", "FREEDOM TO COMPLY.", "FREEDOM TO CONSUME.", "WHAT MORE DO YOU WANT?"}
    },
    {
      {"CONCUR WITH SYSTEM",  +6, -6, -5, +5, "Absolute ideological surrender."},
      {"EXPOSE CONTRADICTION",-6, +7, +5, -5, "Severe dissonance logged."},
      {"BEG FOR RELIEF",      +3, -2, -4,  0, "Helplessness metric maxed."},
      {"REFUSE TO PARTICIPATE",-8, +9, +6, -7, "Existential severance initiated."}
    },
    "GASLIGHT PROTOCOL ACTIVE.", 140
  },

  // Scenario 7: The Awakening (The Illusion of Choice)
  {
    7, "THE AWAKENING", 2,
    {
      {CHAR_STRANGER, "MIRROR", "LOOK CLOSER AT", "YOUR BUTTONS.", "DID YOU EVER CHOOSE?"},
      {CHAR_YOU,      "YOU",    "I CHOSE EVERY STEP!", "I BUILT THIS LIFE!", ""},
      {CHAR_STRANGER, "MIRROR", "YOU CHOSE THE PATH", "THEY DESIGNED.", "YOU BUILT YOUR OWN CAGE."}
    },
    {
      {"DENY THE MIRROR",     +5, -5, -4, +3, "Denial defense erect."},
      {"BREAK THE ILLUSION",  -7, +8, +6, -6, "Cognitive cage shattering."},
      {"ACCEPT COMPLICITY",   +2, +4, +2, -2, "Awakening through guilt."},
      {"QUESTION REALITY",    -8, +9, +7, -7, "Architecture exposed as hollow."}
    },
    "THE VEIL DISSOLVES.", 160
  },

  // Scenario 8: The Void (Architect of Nothing)
  {
    8, "THE VOID", 2,
    {
      {CHAR_SYSTEM, "SYSTEM", "CONGRATULATIONS.", "YOU SURVIVED EVERY TEST.", "WHAT DID YOU BUILD?"},
      {CHAR_YOU,    "YOU",    "...NOTHING.", "I BUILT NOTHING.", ""},
      {CHAR_SYSTEM, "SYSTEM", "YOU ARE THE ARCHITECT", "OF NOTHING.", "RESET TO COMMENCE AGAIN."}
    },
    {
      {"PRESS RESET",         +5, -5, -5, +5, "Loop restarted willingly."},
      {"STAND IN THE VOID",   -5, +8, +8, -5, "Truth embraced in silence."},
      {"FORGIVE YOURSELF",    -2, +6, +9, -2, "Peace made with illusion."},
      {"SEVER ALL CONNECTIONS",-9,+10,+10, -9, "Simulation terminated."}
    },
    "FINAL ARCHITECTURE ACHIEVED: ZERO.", 200
  }
};

// --------------------------------------------------------------------------------------------------
// 4. GLOBAL STATE & TIMERS
// --------------------------------------------------------------------------------------------------
GamePhase currentPhase = PHASE_TITLE;
int currentScenarioIdx = 0;
int currentDialoguePage = 0;
int lastSelectedChoice = 0;
bool toggleActive = false;

// --------------------------------------------------------------------------------------------------
// 5. AUDIO & HAPTIC SOUND ENGINE ("PP PP" SYNCOPATED RHYTHMIC ENGINE)
// --------------------------------------------------------------------------------------------------
unsigned long lastBeatTick = 0;
unsigned long sfxEndTime = 0;
int beatStep = 0;

void updateBackgroundAudio() {
  if (millis() < sfxEndTime) return;
  if (currentPhase == PHASE_TITLE) { noTone(BUZZER_PIN); return; }

  // ⚡ HYPER-INTENSE UNCOMFORTABLE TEMPO: Easy = 125ms/step, Hard = 85ms/step (Extreme Panic)
  int tempoMs = toggleActive ? 85 : 125;

  if (millis() - lastBeatTick >= (unsigned long)tempoMs) {
    lastBeatTick = millis();
    beatStep = (beatStep + 1) % 16;

    if (!toggleActive) {
      // 🎶 EASY MODE: Rapid Erratic Clockwork Anxiety Pulse (125ms)
      switch (beatStep) {
        case 0:  tone(BUZZER_PIN, 190, 20); digitalWrite(VIBRO_PIN, HIGH); delayMicroseconds(3000); digitalWrite(VIBRO_PIN, LOW); break;
        case 1:  tone(BUZZER_PIN, 1800, 10); break; // sharp pp
        case 2:  tone(BUZZER_PIN, 2400, 8); break;  // erratic click
        case 3:  tone(BUZZER_PIN, 1800, 10); break; // sharp pp
        case 4:  tone(BUZZER_PIN, 220, 20); break;
        case 5:  tone(BUZZER_PIN, 1900, 10); break; // pp
        case 6:  tone(BUZZER_PIN, 3300, 6); break;  // high tension tick
        case 8:  tone(BUZZER_PIN, 260, 20); digitalWrite(VIBRO_PIN, HIGH); delayMicroseconds(3000); digitalWrite(VIBRO_PIN, LOW); break;
        case 9:  tone(BUZZER_PIN, 1800, 10); break; // sharp pp
        case 10: tone(BUZZER_PIN, 2200, 8); break;
        case 12: tone(BUZZER_PIN, 310, 20); break;
        case 13: tone(BUZZER_PIN, 1800, 10); break; // sharp pp
        case 14: tone(BUZZER_PIN, 2900, 8); break;  // rapid stutter
        case 15: tone(BUZZER_PIN, 1800, 10); break; // sharp pp
        default: noTone(BUZZER_PIN); break;
      }
    } else {
      // ⚡ HARD MODE: 85ms Chaotic Sensory Overload & Psychological Panic Stabs
      switch (beatStep) {
        case 0:  tone(BUZZER_PIN, 110, 30); digitalWrite(VIBRO_PIN, HIGH); delayMicroseconds(8000); digitalWrite(VIBRO_PIN, LOW); break;
        case 1:  tone(BUZZER_PIN, 2800, 8); break;  // screech pp
        case 2:  tone(BUZZER_PIN, 3500, 6); break;  // ultrasonic tick
        case 3:  tone(BUZZER_PIN, 2800, 8); break;  // screech pp
        case 4:  tone(BUZZER_PIN, 130, 25); digitalWrite(VIBRO_PIN, HIGH); delayMicroseconds(6000); digitalWrite(VIBRO_PIN, LOW); break;
        case 5:  tone(BUZZER_PIN, 3100, 8); break;
        case 6:  tone(BUZZER_PIN, 440, 18); break;  // dissonant clash
        case 7:  tone(BUZZER_PIN, 2800, 8); break;
        case 8:  tone(BUZZER_PIN, 95, 30); digitalWrite(VIBRO_PIN, HIGH); delayMicroseconds(10000); digitalWrite(VIBRO_PIN, LOW); break;
        case 9:  tone(BUZZER_PIN, 3800, 6); break;  // high anxiety needle
        case 10: tone(BUZZER_PIN, 311, 20); break;  // tritone stab
        case 11: tone(BUZZER_PIN, 2800, 8); break;
        case 12: tone(BUZZER_PIN, 150, 25); break;
        case 13: tone(BUZZER_PIN, 3400, 6); break;
        case 14: tone(BUZZER_PIN, 2800, 8); break;
        case 15: tone(BUZZER_PIN, 4200, 5); break;  // panic peak
      }
    }
  }
}

void playSoundTypewriter() {
  sfxEndTime = millis() + 35;
  tone(BUZZER_PIN, 1400, 15);
}

void playSoundComply() {
  sfxEndTime = millis() + 160;
  tone(BUZZER_PIN, 523, 60);
  delay(65);
  tone(BUZZER_PIN, 659, 80);
}

void playSoundAdapt() {
  sfxEndTime = millis() + 140;
  tone(BUZZER_PIN, 440, 50);
  delay(55);
  tone(BUZZER_PIN, 523, 70);
}

void playSoundSacrifice() {
  sfxEndTime = millis() + 190;
  tone(BUZZER_PIN, 349, 70);
  delay(75);
  tone(BUZZER_PIN, 293, 100);
}

void playSoundResist() {
  sfxEndTime = millis() + 290;
  tone(BUZZER_PIN, 180, 120);
  delay(125);
  tone(BUZZER_PIN, 130, 150);
}

void playSoundToggle() {
  sfxEndTime = millis() + 110;
  tone(BUZZER_PIN, 880, 40);
  delay(45);
  tone(BUZZER_PIN, 1760, 50);
}

void playSoundBoot() {
  sfxEndTime = millis() + 300;
  tone(BUZZER_PIN, 523, 80);
  delay(90);
  tone(BUZZER_PIN, 659, 80);
  delay(90);
  tone(BUZZER_PIN, 784, 120);
}

void triggerVibration(int ms) {
  digitalWrite(VIBRO_PIN, HIGH);
  digitalWrite(ONBOARD_LED, HIGH);
  delay(ms);
  digitalWrite(VIBRO_PIN, LOW);
  digitalWrite(ONBOARD_LED, LOW);
}

// --------------------------------------------------------------------------------------------------
// 6. SMART WORD-WRAPPING RENDERER (NO CUT-OFF WORDS!)
// --------------------------------------------------------------------------------------------------
void printWrapped(const char* text, int startX, int startY, int maxCharsPerLine, int lineSpacing, uint16_t color, uint16_t bg) {
  tft.setTextColor(color, bg);
  int curX = startX;
  int curY = startY;
  int colCount = 0;

  char wordBuffer[32];
  int wordLen = 0;

  for (int i = 0; ; i++) {
    char c = text[i];
    if (c == ' ' || c == '\0' || c == '\n') {
      if (wordLen > 0) {
        wordBuffer[wordLen] = '\0';
        // If word doesn't fit on this line, wrap to next line
        if (colCount + wordLen > maxCharsPerLine && colCount > 0) {
          curX = startX;
          curY += lineSpacing;
          colCount = 0;
        }
        tft.setCursor(curX, curY);
        tft.print(wordBuffer);
        curX += wordLen * 6; // 6 pixels per char in size 1
        colCount += wordLen;
        wordLen = 0;
      }
      if (c == ' ') {
        if (colCount < maxCharsPerLine) {
          tft.setCursor(curX, curY);
          tft.print(' ');
          curX += 6;
          colCount++;
        }
      } else if (c == '\n') {
        curX = startX;
        curY += lineSpacing;
        colCount = 0;
      }
      if (c == '\0') break;
    } else {
      if (wordLen < 31) {
        wordBuffer[wordLen++] = c;
      }
    }
  }
}

// --------------------------------------------------------------------------------------------------
// 7. PIXEL ART AVATARS & UI (VERTICAL 128x160 LAYOUT)
// --------------------------------------------------------------------------------------------------
uint16_t getCharacterColor(CharacterID charID) {
  switch (charID) {
    case CHAR_YOU:       return COL_CYAN;
    case CHAR_PARENT:    return COL_GOLD;
    case CHAR_FRIEND:    return COL_GREEN;
    case CHAR_PARTNER:   return 0xF81F;
    case CHAR_STRANGER:  return COL_LIGHT_GRAY;
    case CHAR_SYSTEM:    return COL_RED;
    default:             return COL_WHITE;
  }
}

void drawPixelFace(int bx, int by, CharacterID charID, bool isSpeaking) {
  uint16_t HI, SK, EY, MT, BD, DT;
  switch (charID) {
    case CHAR_YOU:      HI=0x39A7; SK=0xFCF5; EY=0x001F; MT=0xD800; BD=0x07FF; DT=0xFFFF; break;
    case CHAR_PARENT:   HI=0xAD55; SK=0xFBE6; EY=0x2945; MT=0x8000; BD=0xFFE0; DT=0xC618; break;
    case CHAR_FRIEND:   HI=0xFD20; SK=0xFCF5; EY=0x07E0; MT=0xFC00; BD=0x07E0; DT=0xFFE0; break;
    case CHAR_PARTNER:  HI=0xC81F; SK=0xFCF5; EY=0x001F; MT=0xD800; BD=0xF81F; DT=0x07FF; break;
    case CHAR_STRANGER: HI=0x4228; SK=0xC638; EY=0x7BEF; MT=0x6B4D; BD=0x2104; DT=0x8410; break;
    case CHAR_SYSTEM:   HI=0xF800; SK=0x0841; EY=0xFD20; MT=0xFD20; BD=0xF800; DT=0xFC00; break;
    default:            HI=0x8410; SK=0xFCF5; EY=0x0000; MT=0xF800; BD=0x8410; DT=0xFFFF;
  }

  static const uint8_t base[16][14] = {
    {0,0,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {1,1,1,2,2,2,2,2,2,2,2,1,1,1},
    {1,1,2,2,2,2,2,2,2,2,2,2,1,1},
    {1,1,2,2,2,2,2,2,2,2,2,2,1,1},
    {1,1,2,2,0,0,2,2,0,0,2,2,1,1},
    {1,1,2,2,0,0,2,2,0,0,2,2,1,1},
    {1,1,2,2,2,2,2,2,2,2,2,2,1,1},
    {1,1,2,2,2,6,2,2,6,2,2,2,1,1},
    {0,1,2,2,2,2,2,2,2,2,2,2,1,0},
    {0,1,2,2,0,0,0,0,0,2,2,2,1,0},
    {0,1,2,2,2,2,2,2,2,2,2,2,1,0},
    {0,0,1,2,2,2,2,2,2,2,2,1,0,0},
    {0,0,0,1,1,2,2,2,2,1,1,0,0,0},
    {0,0,5,5,5,5,5,5,5,5,5,5,0,0},
    {0,0,5,5,5,5,5,5,5,5,5,5,0,0},
  };

  static const uint8_t eyes[6][2][14] = {
    {{1,1,2,2,3,3,2,2,3,3,2,2,1,1},{1,1,2,2,3,3,2,2,3,3,2,2,1,1}},
    {{1,1,3,3,2,2,2,2,2,2,3,3,1,1},{1,1,2,2,3,3,2,2,3,3,2,2,1,1}},
    {{1,1,2,2,2,2,2,2,2,2,2,2,1,1},{1,1,2,3,3,2,2,2,2,3,3,2,1,1}},
    {{1,1,2,2,2,3,2,2,2,3,2,2,1,1},{1,1,2,2,3,3,2,2,3,3,2,2,1,1}},
    {{1,1,2,2,2,2,2,2,2,2,2,2,1,1},{1,1,2,3,3,3,2,2,3,3,3,2,1,1}},
    {{1,1,3,3,3,3,2,2,3,3,3,3,1,1},{1,1,3,3,3,3,2,2,3,3,3,3,1,1}},
  };

  static const uint8_t mouths[6][14] = {
    {0,1,2,2,4,4,4,4,4,2,2,2,1,0},
    {0,1,2,2,4,2,2,2,4,2,2,2,1,0},
    {0,1,2,4,4,4,4,4,4,4,2,2,1,0},
    {0,1,2,2,2,4,4,4,2,2,2,2,1,0},
    {0,1,2,2,2,4,2,4,2,2,2,2,1,0},
    {0,1,4,2,4,2,4,2,4,2,4,2,1,0},
  };

  int ci = (int)charID;
  if (ci > 5) ci = 5;

  uint16_t palette[7] = {0, HI, SK, EY, MT, BD, DT};

  if (isSpeaking) {
    tft.drawRect(bx - 2, by - 2, 32, 36, COL_TXT_ACC);
  }

  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 14; col++) {
      uint8_t idx;
      if (row == 5 || row == 6) {
        idx = eyes[ci][row - 5][col];
      } else if (row == 10) {
        idx = mouths[ci][col];
      } else {
        idx = base[row][col];
      }
      if (idx > 0 && idx < 7) {
        tft.fillRect(bx + col * 2, by + row * 2, 2, 2, palette[idx]);
      }
    }
  }
}

// --------------------------------------------------------------------------------------------------
// 8. VERTICAL SCREEN RENDERERS (128x160 PORTRAIT MODE)
// --------------------------------------------------------------------------------------------------
void drawTitleScreen() {
  tft.fillScreen(COL_BLACK);

  // Gothic Outer Double Border
  tft.drawRect(2, 2, 124, 156, COL_TXT_ACC);
  tft.drawRect(4, 4, 120, 152, COL_DIALOGUE_BD);

  // Corner Cross Filigree
  tft.setTextColor(COL_TXT_ACC, COL_BLACK);
  tft.setCursor(8, 8);   tft.print("+--[+]--+");
  tft.setCursor(8, 146); tft.print("+--[+]--+");

  // Spires
  tft.drawFastHLine(10, 24, 108, COL_TXT_ACC);
  tft.drawFastHLine(10, 126, 108, COL_TXT_ACC);

  // Stylized Title
  tft.setTextSize(1);
  tft.setTextColor(COL_TXT_ACC, COL_BLACK);
  tft.setCursor(14, 42);
  tft.print("~ A R C H I T E C T ~");

  tft.setTextColor(COL_TXT_PRI, COL_BLACK);
  tft.setCursor(20, 60);
  tft.print("- OF NOTHING -");

  // Ambient subtitle
  tft.setTextColor(COL_TXT_ACC, COL_BLACK);
  tft.setCursor(16, 84);
  tft.print("CHOICE SIMULATION");

  // Press button prompt
  tft.setTextColor(COL_TXT_PRI, COL_BLACK);
  tft.setCursor(12, 110);
  tft.print("PRESS ANY BUTTON");
  tft.setCursor(34, 134);
  tft.print("TO BEGIN");
}

void drawDialogueScreen(const Scenario& sc, int pageIdx) {
  tft.fillScreen(COL_BLACK);

  const DialogueLine& line = sc.pages[pageIdx];
  bool isYouSpeaking = (line.speaker == CHAR_YOU);

  CharacterID opponentID = sc.pages[0].speaker;
  if (opponentID == CHAR_YOU && sc.totalPages > 1) {
    opponentID = sc.pages[1].speaker;
  }
  if (opponentID == CHAR_YOU) opponentID = CHAR_SYSTEM;

  // Header Bar (128px width)
  tft.fillRect(0, 0, 128, 12, 0x0821);
  tft.drawFastHLine(0, 12, 128, COL_DIALOGUE_BD);
  tft.setTextSize(1);
  tft.setTextColor(COL_TXT_ACC, 0x0821);
  tft.setCursor(3, 2);
  tft.printf("#%02d %s", sc.id, toggleActive ? "[HARD]" : "[EASY]");

  // Left Avatar (YOU) & Right Avatar (OPPONENT)
  drawPixelFace(4, 16, CHAR_YOU, isYouSpeaking);
  tft.fillRect(4, 50, 28, 8, isYouSpeaking ? 0x18C3 : 0x0821);
  tft.setTextColor(COL_TXT_ACC, isYouSpeaking ? 0x18C3 : 0x0821);
  tft.setCursor(8, 51);
  tft.print("YOU");

  drawPixelFace(96, 16, opponentID, !isYouSpeaking);
  tft.fillRect(94, 50, 32, 8, !isYouSpeaking ? 0x18C3 : 0x0821);
  tft.setTextColor(COL_TXT_ACC, !isYouSpeaking ? 0x18C3 : 0x0821);
  tft.setCursor(96, 51);
  tft.print(line.speakerName);

  // Center Speech Indicator
  tft.setTextColor(COL_TXT_ACC, COL_BLACK);
  tft.setCursor(42, 28);
  tft.print(isYouSpeaking ? "TALK>>" : "<<TALK");

  // Dialogue Box Frame (Vertical: Y=60 to 142)
  tft.fillRoundRect(2, 60, 124, 82, 3, COL_DIALOGUE_BG);
  tft.drawRoundRect(2, 60, 124, 82, 3, toggleActive ? COL_ALERT_RED : COL_DIALOGUE_BD);

  // Smart Word-Wrapped Dialogue Text!
  String fullDialogue = String(line.line1) + " " + String(line.line2) + " " + String(line.line3);
  printWrapped(fullDialogue.c_str(), 6, 66, 19, 10, COL_TXT_PRI, COL_DIALOGUE_BG);

  // Bottom Status Bar (Clean 128px fit, never cut off!)
  tft.fillRect(0, 146, 128, 14, 0x0821);
  tft.drawFastHLine(0, 145, 128, COL_DIALOGUE_BD);
  tft.setTextColor(COL_TXT_ACC, 0x0821);
  tft.setCursor(3, 149);
  tft.printf("C:%d%% R:%d%% W:%d%%", gVars.compliance, gVars.resistance, gVars.selfWorth);
}

void drawChoiceScreen(const Scenario& sc) {
  tft.fillScreen(COL_BLACK);

  // Header bar
  tft.fillRect(0, 0, 128, 12, 0x0821);
  tft.drawFastHLine(0, 12, 128, COL_DIALOGUE_BD);
  tft.setTextSize(1);
  tft.setTextColor(COL_TXT_ACC, 0x0821);
  tft.setCursor(3, 2);
  tft.printf("SCENARIO #%02d %s", sc.id, toggleActive ? "[HARD]" : "[EASY]");

  // 4 Clean Choice Cards (Vertical: Y = 16, 50, 84, 118)
  int rowY[4] = {16, 50, 84, 118};

  for (int i = 0; i < 4; i++) {
    uint16_t bg  = 0x0821;
    uint16_t bdr = COL_DIALOGUE_BD; // Clean consistent border for all!

    tft.fillRoundRect(2, rowY[i], 124, 30, 3, bg);
    tft.drawRoundRect(2, rowY[i], 124, 30, 3, bdr);

    // Number Badge
    tft.fillRect(4, rowY[i] + 3, 14, 24, 0x18C3);
    tft.drawRect(4, rowY[i] + 3, 14, 24, COL_TXT_ACC);
    tft.setTextColor(COL_TXT_ACC, 0x18C3);
    tft.setCursor(8, rowY[i] + 11);
    tft.printf("%d", i + 1);

    // Smart Word-Wrapped Choice Label!
    printWrapped(sc.options[i].label, 22, rowY[i] + 5, 16, 9, COL_TXT_PRI, bg);
  }

  // Footer bar
  tft.fillRect(0, 150, 128, 10, 0x0821);
  tft.setTextColor(COL_TXT_ACC, 0x0821);
  tft.setCursor(6, 151);
  tft.print("PRESS [1-4] TO ACT");
}

void drawSystemReactionScreen(const Scenario& sc, int choiceIdx) {
  tft.fillScreen(COL_BLACK);

  // Outer Border Frame
  tft.drawRoundRect(2, 2, 124, 156, 3, toggleActive ? COL_ALERT_RED : COL_DIALOGUE_BD);
  tft.fillRect(4, 4, 120, 152, 0x0821);

  // Title Header
  tft.setTextSize(1);
  tft.setTextColor(COL_TXT_ACC, 0x0821);
  tft.setCursor(6, 8);
  tft.print(toggleActive ? "SYSTEM ANALYSIS:" : "SYSTEM FEEDBACK:");

  tft.drawFastHLine(6, 18, 116, COL_DIALOGUE_BD);

  // Word-Wrapped Feedback Comment
  String feedbackText = "\"" + String(sc.options[choiceIdx].feedbackComment) + "\"";
  printWrapped(feedbackText.c_str(), 6, 24, 18, 10, COL_TXT_PRI, 0x0821);

  // System Evaluation Log
  tft.drawFastHLine(6, 68, 116, COL_GRID);
  tft.setCursor(6, 73);
  tft.setTextColor(COL_TXT_ACC, 0x0821);
  tft.print("SYSTEM LOG:");
  printWrapped(sc.systemEval, 6, 83, 18, 10, COL_TXT_PRI, 0x0821);

  // Metric Delta Bar
  tft.drawFastHLine(6, 118, 116, COL_GRID);
  tft.setCursor(6, 123);
  tft.setTextColor(COL_TXT_ACC, 0x0821);
  tft.printf("C:%d%% R:%d%% W:%d%%", gVars.compliance, gVars.resistance, gVars.selfWorth);

  // Prompt (Press to proceed)
  tft.fillRect(6, 138, 116, 14, 0x18C3);
  tft.drawRect(6, 138, 116, 14, COL_TXT_ACC);
  tft.setTextColor(COL_TXT_ACC, 0x18C3);
  tft.setCursor(10, 141);
  tft.print(">> PRESS ANY BTN <<");
}

void drawFinalCollapseScreen() {
  tft.fillScreen(COL_BLACK);

  tft.drawRect(2, 2, 124, 156, COL_TXT_ACC);
  tft.drawRect(4, 4, 120, 152, COL_DIALOGUE_BD);

  tft.setTextSize(1);
  tft.setTextColor(COL_TXT_ACC, COL_BLACK);
  tft.setCursor(8, 10);
  tft.print("+ REALIZATION +");

  // 4 Branching Psychological Endings:
  const char* endTitle;
  const char* endBody;

  if (gVars.compliance >= 65) {
    endTitle = "THE COMPLIANT COG";
    endBody  = "You surrendered every instinct to satisfy the score. The machine applauds your perfection, then replaces you.";
  } else if (gVars.resistance >= 30) {
    endTitle = "THE BROKEN REBEL";
    endBody  = "You fought every rule. Yet rebellion was predicted. You defined yourself solely by what you opposed.";
  } else if (gVars.selfWorth >= 60) {
    endTitle = "THE AWAKENED GHOST";
    endBody  = "You stopped measuring life by their numbers. You set down the controller. You built nothing—and are finally free.";
  } else {
    endTitle = "THE EMPTY SHELL";
    endBody  = "You survived every test, but never lived. The metrics are maxed. Welcome to the architecture of nothing.";
  }

  tft.setTextColor(COL_TXT_ACC, COL_BLACK);
  tft.setCursor(8, 26);
  tft.print(endTitle);

  printWrapped(endBody, 8, 42, 18, 10, COL_TXT_PRI, COL_BLACK);

  tft.drawFastHLine(8, 126, 112, COL_TXT_ACC);
  tft.setTextColor(COL_TXT_ACC, COL_BLACK);
  tft.setCursor(8, 132);
  tft.print("ARCHITECT OF NOTHING");
  tft.setTextColor(COL_TXT_PRI, COL_BLACK);
  tft.setCursor(14, 144);
  tft.print("[ PRESS TO REBOOT ]");
}

void renderCurrentScreen() {
  const Scenario& sc = SCENARIOS[currentScenarioIdx % TOTAL_SCENARIOS];

  switch (currentPhase) {
    case PHASE_TITLE:
      drawTitleScreen();
      break;

    case PHASE_DIALOGUE:
      drawDialogueScreen(sc, currentDialoguePage);
      break;

    case PHASE_CHOICE_SELECT:
      drawChoiceScreen(sc);
      break;

    case PHASE_SYSTEM_REACT:
      drawSystemReactionScreen(sc, lastSelectedChoice);
      break;

    case PHASE_FINAL_COLLAPSE:
      drawFinalCollapseScreen();
      break;
  }
}

// --------------------------------------------------------------------------------------------------
// 9. PROGRESSION & HARDWARE POLLING
// --------------------------------------------------------------------------------------------------
void advancePhase() {
  const Scenario& sc = SCENARIOS[currentScenarioIdx % TOTAL_SCENARIOS];

  if (currentPhase == PHASE_TITLE) {
    sfxEndTime = millis();
    currentPhase = PHASE_DIALOGUE;
    currentDialoguePage = 0;
  } else if (currentPhase == PHASE_DIALOGUE) {
    currentDialoguePage++;
    if (currentDialoguePage >= sc.totalPages) {
      currentPhase = PHASE_CHOICE_SELECT;
    }
  }

  playSoundTypewriter();
  renderCurrentScreen();
}

void advanceFromFeedback() {
  playSoundTypewriter();
  currentScenarioIdx++;

  if (currentScenarioIdx >= TOTAL_SCENARIOS) {
    currentPhase = PHASE_FINAL_COLLAPSE;
  } else {
    currentPhase = PHASE_DIALOGUE;
    currentDialoguePage = 0;
  }
  renderCurrentScreen();
}

void makeChoice(int choiceIndex) {
  const Scenario& sc = SCENARIOS[currentScenarioIdx % TOTAL_SCENARIOS];
  const ChoiceOption& opt = sc.options[choiceIndex];

  lastSelectedChoice = choiceIndex;

  gVars.compliance += opt.deltaCompliance;
  gVars.resistance += opt.deltaResistance;
  gVars.selfWorth  += opt.deltaSelfWorth;
  gVars.socialStatus += opt.deltaStatus;

  switch (choiceIndex) {
    case 0: playSoundComply(); break;
    case 1: playSoundAdapt(); break;
    case 2: playSoundSacrifice(); break;
    case 3: playSoundResist(); break;
  }

  triggerVibration(sc.vibroDuration);

  currentPhase = PHASE_SYSTEM_REACT;
  renderCurrentScreen();
}

bool lastStateB1 = HIGH, lastStateB2 = HIGH, lastStateB3 = HIGH, lastStateB4 = HIGH;
bool lastToggleState = HIGH;
unsigned long lastToggleFlipTime = 0;
int toggleFlipCount = 0;

void checkInputs() {
  bool currentToggle = digitalRead(TOGGLE_PIN);
  if (currentToggle != lastToggleState) {
    lastToggleState = currentToggle;
    toggleActive = (currentToggle == LOW);
    
    unsigned long now = millis();
    if (now - lastToggleFlipTime < 800) {
      toggleFlipCount++;
    } else {
      toggleFlipCount = 1;
    }
    lastToggleFlipTime = now;

    if (toggleFlipCount >= 3) {
      toggleFlipCount = 0;
      noTone(BUZZER_PIN);
      digitalWrite(VIBRO_PIN, LOW);
      digitalWrite(ONBOARD_LED, LOW);
      sfxEndTime = millis() + 99999999;

      Serial.println("\n>>> [3 TOGGLE FLIPS] Game & sound stopped! Return to Start Page.");

      currentPhase = PHASE_TITLE;
      currentScenarioIdx = 0;
      currentDialoguePage = 0;
      gVars = GameVariables();

      renderCurrentScreen();
      return;
    }

    playSoundToggle();
    triggerVibration(40);
    renderCurrentScreen();
  }

  bool rawB1 = digitalRead(BTN_1);
  bool rawB2 = digitalRead(BTN_2);
  bool rawB3 = digitalRead(BTN_3);
  bool rawB4 = digitalRead(BTN_4);

  bool pressB1 = (rawB1 == LOW && lastStateB1 == HIGH);
  bool pressB2 = (rawB2 == LOW && lastStateB2 == HIGH);
  bool pressB3 = (rawB3 == LOW && lastStateB3 == HIGH);
  bool pressB4 = (rawB4 == LOW && lastStateB4 == HIGH);

  lastStateB1 = rawB1;
  lastStateB2 = rawB2;
  lastStateB3 = rawB3;
  lastStateB4 = rawB4;

  bool anyPress = (pressB1 || pressB2 || pressB3 || pressB4);

  if (currentPhase == PHASE_TITLE || currentPhase == PHASE_DIALOGUE) {
    if (anyPress) { delay(40); advancePhase(); return; }
  } else if (currentPhase == PHASE_CHOICE_SELECT) {
    if (pressB1) { delay(40); makeChoice(0); return; }
    if (pressB2) { delay(40); makeChoice(1); return; }
    if (pressB3) { delay(40); makeChoice(2); return; }
    if (pressB4) { delay(40); makeChoice(3); return; }
  } else if (currentPhase == PHASE_SYSTEM_REACT) {
    if (anyPress) { delay(40); advanceFromFeedback(); return; }
  } else if (currentPhase == PHASE_FINAL_COLLAPSE) {
    if (anyPress) {
      delay(40);
      currentPhase = PHASE_TITLE;
      currentScenarioIdx = 0;
      currentDialoguePage = 0;
      gVars = GameVariables();
      renderCurrentScreen();
      return;
    }
  }

  if (Serial.available() > 0) {
    char c = Serial.read();
    if (currentPhase == PHASE_TITLE || currentPhase == PHASE_DIALOGUE) {
      advancePhase();
    } else if (currentPhase == PHASE_CHOICE_SELECT && c >= '1' && c <= '4') {
      makeChoice(c - '1');
    } else if (currentPhase == PHASE_SYSTEM_REACT) {
      advanceFromFeedback();
    } else if (currentPhase == PHASE_FINAL_COLLAPSE) {
      currentPhase = PHASE_TITLE;
      currentScenarioIdx = 0;
      renderCurrentScreen();
    }
  }
}

// --------------------------------------------------------------------------------------------------
// 10. SETUP & MAIN LOOP
// --------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2); // 💡 Flipped Vertical / Portrait Mode (128x160)
  tft.fillScreen(COL_BLACK);

  pinMode(BTN_1, INPUT_PULLUP); gpio_set_pull_mode((gpio_num_t)BTN_1, GPIO_PULLUP_ONLY);
  pinMode(BTN_2, INPUT_PULLUP); gpio_set_pull_mode((gpio_num_t)BTN_2, GPIO_PULLUP_ONLY);
  pinMode(BTN_3, INPUT_PULLUP); gpio_set_pull_mode((gpio_num_t)BTN_3, GPIO_PULLUP_ONLY);
  pinMode(BTN_4, INPUT_PULLUP); gpio_set_pull_mode((gpio_num_t)BTN_4, GPIO_PULLUP_ONLY);

  pinMode(TOGGLE_PIN, INPUT_PULLUP); gpio_set_pull_mode((gpio_num_t)TOGGLE_PIN, GPIO_PULLUP_ONLY);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIBRO_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);

  playSoundBoot();
  triggerVibration(100);

  currentPhase = PHASE_TITLE;
  renderCurrentScreen();
}

void loop() {
  checkInputs();
  updateBackgroundAudio();
  delay(15);
}
