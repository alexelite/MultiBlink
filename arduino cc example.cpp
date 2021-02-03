// Multi_Blink2
//
// Blink lots of LEDs at different frequencies simultaneously, include fades, delays and loops in patterns
//
// Marco Colli - September 2012
//
// Demonstrates the way to carry out multiple time based tasks without using the delay() function
// Demonstrates the use of structures (and structures within structures)
// Demonstrates a data driven approach to programming to create compact, reusable code
//

#include "Multi_Blink2.h"  // type definitions

// Blink Table T - Modify this table to suit whatever the output requirements are
// Add or delete lines as required to achieve the desired effects.
// Have multiple tables and switch between them to create different effects based on external inputs
// To add additional states the structure in the header file needs to be modified
//
ledTable  T1[] =
//Pin  St WUp  State 0              State 1              etc
{
  { 3, 0, 0, {{MB_ON, 50, 0, 0},   {MB_OFF, 100, 0 ,0}, {MB_LOOP, 0, LOOP_PARAM(0, 4), 0},  {MB_OFF, 800, 0, 0}, {MB_NULL, 0, 0, 0}}  },
  { 4, 0, 0, {{MB_OFF, 100, 0, 0}, {MB_ON, 50, 0, 0},   {MB_LOOP, 0, LOOP_PARAM(0, 4), 0},  {MB_OFF, 800, 0, 0}, {MB_NULL, 0, 0, 0}}  },
  { 5, 0, 0, {{MB_OFF, 800, 0, 0}, {MB_ON, 50, 0, 0},   {MB_OFF, 100, 0, 0}, {MB_LOOP, 0, LOOP_PARAM(1, 4), 0},  {MB_NULL, 0, 0, 0}}  },
  { 6, 0, 0, {{MB_OFF, 800, 0, 0}, {MB_OFF, 100, 0, 0}, {MB_ON, 50, 0, 0},   {MB_LOOP, 0, LOOP_PARAM(1, 4), 0},  {MB_NULL, 0, 0, 0}}  },
};
ledTable T2[] =
{
  { 3, 0, 0, {{MB_ON, 20, 0, 0},   {MB_OFF, 50, 0, 0},  {MB_LOOP, 0, LOOP_PARAM(0, 3), 0},  {MB_ON, 250, 0, 0},  {MB_OFF, 450, 0, 0}} },
  { 4, 0, 0, {{MB_OFF, 450, 0, 0}, {MB_ON, 20, 0, 0},   {MB_OFF, 50, 0, 0},  {MB_LOOP, 0, LOOP_PARAM(1, 3), 0},  {MB_ON, 250, 0, 0}}  },
 
  { 5, 0, 0, {{MB_FADE_ON, 45, FADE_PARAM(1, 30), 0}, {MB_ON, 70, 0, 0}, {MB_FADE_OFF, 45, FADE_PARAM(30, 1), 0}, {MB_NULL,0,0,0}, {MB_NULL,0,0,0}} },
  { 6, 0, 0, {{MB_FADE_ON, 10, FADE_PARAM(0, 255), 0}, {MB_ON, 70, 0, 0}, {MB_FADE_OFF, 10, FADE_PARAM(255, 0), 0}, {MB_NULL,0,0,0}, {MB_NULL,0,0,0}} },
};

// Self adjusting constants for loop indexes
#define  MAX_STATE  (sizeof(T1[0].state)/sizeof(stateDef))

void BlinkInit(ledTable *pT, uint8_t tableSize)
{
  for (uint8_t i=0; i < tableSize; i++, pT++)
  {
    pinMode(pT->ledPin, OUTPUT);
   
    pT->nextWakeup = 0;
    digitalWrite(pT->ledPin, LOW);
    for (uint8_t j=0; j<MAX_STATE; j++)
    {
      pT->state[j].counter = CTR_UNDEF;
    }
  }
  return;
}

void MultiBlink(ledTable *pT, uint8_t tableSize)
{
  for (int i=0; i < tableSize; i++, pT++)
  {
      uint8_t  cs = pT->currentState;  // current state shortcut

    // check if the state active time has expired (ie, it is less than current time)
    if (millis() >= pT->nextWakeup)
    {
      pT->nextWakeup = millis() + pT->state[cs].activeTime;

      switch (pT->state[cs].stateID)
      {
        case MB_OFF:
        case MB_ON:    // Write digital value
        {
          digitalWrite(pT->ledPin, pT->state[cs].stateID == MB_ON ? HIGH : LOW);
          pT->currentState = (pT->currentState + 1) % MAX_STATE;
        }
        break;
         
        case MB_FADE_ON:
        {
          // first time in this state? Check the counter
          if (pT->state[cs].counter == CTR_UNDEF)
          {
            pT->state[cs].counter = FADE_START_GET(pT->state[cs].data);
          }

          analogWrite(pT->ledPin, pT->state[cs].counter++);
         
          if (pT->state[cs].counter >= FADE_END_GET(pT->state[cs].data))
          {
            pT->state[cs].counter = CTR_UNDEF; // set up loop counter
            pT->currentState = (pT->currentState + 1) % MAX_STATE;
          }
        }
        break;

        case MB_FADE_OFF:
        {
          // first time in this state? Check the counter
          if (pT->state[cs].counter == CTR_UNDEF)
          {
            pT->state[cs].counter = FADE_START_GET(pT->state[cs].data);
          }

          analogWrite(pT->ledPin, pT->state[cs].counter--);
         
          if (pT->state[cs].counter <= FADE_END_GET(pT->state[cs].data))
          {
            pT->state[cs].counter = CTR_UNDEF; // set up loop counter
            pT->currentState = (pT->currentState + 1) % MAX_STATE;
          }
        }
        break;

        case MB_LOOP:  // loop back to specified state if there is still count left
        {
          // first time in this state? Check the counter
          if (pT->state[cs].counter == CTR_UNDEF)
          {
            pT->state[cs].counter = LOOP_SP_GET(pT->state[cs].data);
          }

          // loop around or keep going?         
          if (pT->state[cs].counter-- > 0)
          {
            pT->currentState = LOOP_STATE_GET(pT->state[cs].data);
            pT->nextWakeup = 0;  // do it immediately
          }
          else
          {
            pT->state[cs].counter = CTR_UNDEF; // set up loop counter
            pT->currentState = (pT->currentState + 1) % MAX_STATE;
          }
        }
        break; 
         
        default:  // just move on - handles NULL and any stupid states we may get into
        {
          pT->currentState = (pT->currentState + 1) % MAX_STATE;
        }
        break;
      }
    }
  }
 
  return;
}

void setup()
{
  BlinkInit(T1, sizeof(T1)/sizeof(ledTable));
  BlinkInit(T2, sizeof(T2)/sizeof(ledTable));
}

#define  BUTTON_PIN  7  // switch between patterns

void loop()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    MultiBlink(T1, sizeof(T1)/sizeof(ledTable));
  }
  else
  {
    MultiBlink(T2, sizeof(T2)/sizeof(ledTable));
  }   
}

// Multi_Blink2
//
// Blink lots of LEDs at different frequencies simultaneously, include fades, delays and loops in patterns
//
// Marco Colli - September 2012
//
// Demonstrates the way to carry out multiple time based tasks without using the delay() function
// Demonstrates the use of structures (and structures within structures)
// Demonstrates a data driven approach to programming to create compact, reusable code
//

#include "Multi_Blink2.h"  // type definitions

// Blink Table T - Modify this table to suit whatever the output requirements are
// Add or delete lines as required to achieve the desired effects.
// Have multiple tables and switch between them to create different effects based on external inputs
// To add additional states the structure in the header file needs to be modified
//
ledTable  T1[] =
//Pin  St WUp  State 0              State 1              etc
{
  { 3, 0, 0, {{MB_ON, 50, 0, 0},   {MB_OFF, 100, 0 ,0}, {MB_LOOP, 0, LOOP_PARAM(0, 4), 0},  {MB_OFF, 800, 0, 0}, {MB_NULL, 0, 0, 0}}  },
  { 4, 0, 0, {{MB_OFF, 100, 0, 0}, {MB_ON, 50, 0, 0},   {MB_LOOP, 0, LOOP_PARAM(0, 4), 0},  {MB_OFF, 800, 0, 0}, {MB_NULL, 0, 0, 0}}  },
  { 5, 0, 0, {{MB_OFF, 800, 0, 0}, {MB_ON, 50, 0, 0},   {MB_OFF, 100, 0, 0}, {MB_LOOP, 0, LOOP_PARAM(1, 4), 0},  {MB_NULL, 0, 0, 0}}  },
  { 6, 0, 0, {{MB_OFF, 800, 0, 0}, {MB_OFF, 100, 0, 0}, {MB_ON, 50, 0, 0},   {MB_LOOP, 0, LOOP_PARAM(1, 4), 0},  {MB_NULL, 0, 0, 0}}  },
};
ledTable T2[] =
{
  { 3, 0, 0, {{MB_ON, 20, 0, 0},   {MB_OFF, 50, 0, 0},  {MB_LOOP, 0, LOOP_PARAM(0, 3), 0},  {MB_ON, 250, 0, 0},  {MB_OFF, 450, 0, 0}} },
  { 4, 0, 0, {{MB_OFF, 450, 0, 0}, {MB_ON, 20, 0, 0},   {MB_OFF, 50, 0, 0},  {MB_LOOP, 0, LOOP_PARAM(1, 3), 0},  {MB_ON, 250, 0, 0}}  },
 
  { 5, 0, 0, {{MB_FADE_ON, 45, FADE_PARAM(1, 30), 0}, {MB_ON, 70, 0, 0}, {MB_FADE_OFF, 45, FADE_PARAM(30, 1), 0}, {MB_NULL,0,0,0}, {MB_NULL,0,0,0}} },
  { 6, 0, 0, {{MB_FADE_ON, 10, FADE_PARAM(0, 255), 0}, {MB_ON, 70, 0, 0}, {MB_FADE_OFF, 10, FADE_PARAM(255, 0), 0}, {MB_NULL,0,0,0}, {MB_NULL,0,0,0}} },
};

// Self adjusting constants for loop indexes
#define  MAX_STATE  (sizeof(T1[0].state)/sizeof(stateDef))

void BlinkInit(ledTable *pT, uint8_t tableSize)
{
  for (uint8_t i=0; i < tableSize; i++, pT++)
  {
    pinMode(pT->ledPin, OUTPUT);
   
    pT->nextWakeup = 0;
    digitalWrite(pT->ledPin, LOW);
    for (uint8_t j=0; j<MAX_STATE; j++)
    {
      pT->state[j].counter = CTR_UNDEF;
    }
  }
  return;
}

void MultiBlink(ledTable *pT, uint8_t tableSize)
{
  for (int i=0; i < tableSize; i++, pT++)
  {
      uint8_t  cs = pT->currentState;  // current state shortcut

    // check if the state active time has expired (ie, it is less than current time)
    if (millis() >= pT->nextWakeup)
    {
      pT->nextWakeup = millis() + pT->state[cs].activeTime;

      switch (pT->state[cs].stateID)
      {
        case MB_OFF:
        case MB_ON:    // Write digital value
        {
          digitalWrite(pT->ledPin, pT->state[cs].stateID == MB_ON ? HIGH : LOW);
          pT->currentState = (pT->currentState + 1) % MAX_STATE;
        }
        break;
         
        case MB_FADE_ON:
        {
          // first time in this state? Check the counter
          if (pT->state[cs].counter == CTR_UNDEF)
          {
            pT->state[cs].counter = FADE_START_GET(pT->state[cs].data);
          }

          analogWrite(pT->ledPin, pT->state[cs].counter++);
         
          if (pT->state[cs].counter >= FADE_END_GET(pT->state[cs].data))
          {
            pT->state[cs].counter = CTR_UNDEF; // set up loop counter
            pT->currentState = (pT->currentState + 1) % MAX_STATE;
          }
        }
        break;

        case MB_FADE_OFF:
        {
          // first time in this state? Check the counter
          if (pT->state[cs].counter == CTR_UNDEF)
          {
            pT->state[cs].counter = FADE_START_GET(pT->state[cs].data);
          }

          analogWrite(pT->ledPin, pT->state[cs].counter--);
         
          if (pT->state[cs].counter <= FADE_END_GET(pT->state[cs].data))
          {
            pT->state[cs].counter = CTR_UNDEF; // set up loop counter
            pT->currentState = (pT->currentState + 1) % MAX_STATE;
          }
        }
        break;

        case MB_LOOP:  // loop back to specified state if there is still count left
        {
          // first time in this state? Check the counter
          if (pT->state[cs].counter == CTR_UNDEF)
          {
            pT->state[cs].counter = LOOP_SP_GET(pT->state[cs].data);
          }

          // loop around or keep going?         
          if (pT->state[cs].counter-- > 0)
          {
            pT->currentState = LOOP_STATE_GET(pT->state[cs].data);
            pT->nextWakeup = 0;  // do it immediately
          }
          else
          {
            pT->state[cs].counter = CTR_UNDEF; // set up loop counter
            pT->currentState = (pT->currentState + 1) % MAX_STATE;
          }
        }
        break; 
         
        default:  // just move on - handles NULL and any stupid states we may get into
        {
          pT->currentState = (pT->currentState + 1) % MAX_STATE;
        }
        break;
      }
    }
  }
 
  return;
}

void setup()
{
  BlinkInit(T1, sizeof(T1)/sizeof(ledTable));
  BlinkInit(T2, sizeof(T2)/sizeof(ledTable));
}

#define  BUTTON_PIN  7  // switch between patterns

void loop()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    MultiBlink(T1, sizeof(T1)/sizeof(ledTable));
  }
  else
  {
    MultiBlink(T2, sizeof(T2)/sizeof(ledTable));
  }   
}
