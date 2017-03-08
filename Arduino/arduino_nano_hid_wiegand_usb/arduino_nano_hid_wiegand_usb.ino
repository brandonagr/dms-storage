/*
   HID RFID Reader Wiegand Interface for Arduino Uno
   Written by Daniel Smith, 2012.01.30
   www.pagemac.com

   Modified by Stan Simmons, 2016.12.30 to add PIN codes
   Modified by Bill Gee, 2016.12.30 to add LED and Buzzer control
   Modified by Stan Simmons, 2017.02.07 to add Parity Checking

   This program decodes the Wiegand data from a variety of HID RFID readers.

   The Wiegand interface has two data lines, DATA0 and DATA1. These lines are
   normally held high at 5V. When a 0 is sent, DATA0 drops to 0V for a few us.
   When a 1 is sent, DATA1 drops to 0V for a few us. There is usually a few ms
   between the pulses. Both lines dropping to 0V is an error condition.

   The reader should have at least 4 connections (some readers have more, LED,
   Buzzer, 35 bit enable). Connect the red wire to 12V. Connect the black wire
   to ground. Connect the green wire (DATA0) to Digital Pin 2 (INT0). Connect
   the white wire (DATA1) to Digital Pin 3 (INT1). The blue wire is the reader
   LED, the yellow wire is the reader Buzzer, and usually the grey wire is 35
   bit enable. The LED and Buzzer lines are held HIGH for off, LOW for on.

   Each of the data lines are connected to hardware interrupt lines. When one
   drops low, an interrupt routine is called and some bits are flipped. After
   some time of of not receiving any bits, the Arduino will decode the data.
   The program currently only decodes the 26 bit, 35 bit and PIN Code (4 bit)
   formats, but you can easily add more.

   35 bit HID Corporate 1000 format:
   The 35 bits are broken down into a 12 bit facility code (bits 3-14) and a
   20 bit card code (bits 15-34).
   Bit 1 is an odd parity bit that covers all 35 bits. Bit 2 is an even parity
   covering bits 3&4, 6&7, 9&10, 12&13, 15&16, 18&19, 21&22, 24&25, 27&28,
   30&31, 33&34. Bit 35 is odd parity, covering bits 2&3, 5&6, 8&9, 11&12,
   14&15, 17&18, 20&21, 23&24, 26&27, 29&30, 32&33. When calculating the parity
   bits, calculate bit 2, bit 35, and then bit 1. (Parity not yet implemented)

   26 bit standard format:
   The 26 bits are broken down into an 8 bit facility code (bits 2-9), a 16 bit
   card code (bits 10-25), and two parity bits. The first parity bit is an even
   parity bit and covers the first 13 bits. Bits 2 thru 9 are the facility code
   and bits 10 thru 25 are the card code, stored big endian (MSB first). The
   last parity bit is an odd parity bit and covers the last 13 bits. There are
   only 255 possible facility codes and 65,535 possible card codes, so there
   are duplicate cards.

   8 bit PIN format: (Not yet implemented)
   No parity bits. 8 bit Wiegand keyboard data, high nibble is the "NOT" of low
   nibble. eg if key 1 is pressed, data = E1, in binary 11100001 , high nibble
   = 1110 , low nibble = 0001.

   4 bit PIN format:
   No parity bits, internal code = bits 0 to 4, ESC (1010) clears pin, ENT
   (1011) sends the pin. We are arbitrarily limiting the pin to a minimum of 6
   and maximum of 10 characters. We drop leading zeros to match the cards.
*/

#include <Wire.h>

#define MAX_BITS 100                 /* max number of bits */
#define WIEGAND_RECEIVE_WAIT_US  2500 /* time to wait for another wiegand pulse, if exceeded assume transmission is done and its time to process buffers */

#define KEYPAD_ESC    0xA            /* Keypad ESC code */
#define KEYPAD_ENT    0xB            /* Keypad ENT code */
#define PINMIN        6              /* Minimum pin length */
#define PINMAX        10             /* Maximum pin length */

#define SECONDS_UNTIL_CLEAR 5        /* Seconds before pending PIN is reset */

unsigned char databits[MAX_BITS];    // stores all of the data bits
unsigned char bitCount;              // number of bits currently captured
unsigned long lastBitReceiveUs = 0;   // microseconds since last bit was received

unsigned long lastPinTimeMs = 0;          // millis the last time a digit pressed

unsigned long facilityCode = 0;      // decoded facility code
unsigned long cardCode = 0;          // decoded card code
unsigned long internalCode = 0;      // decoded internal card code
unsigned long pinCode = 0;           // decoded pin code.
int           pinKeys = 0;           // number of digits entered
int           gotNonZero = 0;        // got a non-zero digit
int           evenparity = 0;        // calculate even parity
int           oddparity = 0;         // calculate odd parity
int           fullparity = 0;        // calculate full length parity
int           everythird = 0;        // calculate every third for parity
int           fulloddparity = 0;     // calculate full length odd parity
int           startingparity = 0;    // calculate starting parity bit


// interrupt that happens when INT0 goes low (0 bit)
void ISR_INT0()
{
  bitCount++;
  lastBitReceiveUs = micros();
}

// interrupt that happens when INT1 goes low (1 bit)
void ISR_INT1()
{
  databits[bitCount] = 1;
  bitCount++;
  lastBitReceiveUs = micros();
}

void setup()
{
  pinMode(2, INPUT);     // D2 INT0 Wiegand DATA0 Green
  pinMode(3, INPUT);     // D3 INT1 Wiegand DATA1 White
  pinMode(4, OUTPUT);    // D4 Wiegand Buzzer Yellow
  pinMode(5, OUTPUT);    // D5 Wiegand LED Blue

  Serial.begin(57600);

  // binds the ISR functions to the falling edge of INTO and INT1
  attachInterrupt(digitalPinToInterrupt(2), ISR_INT0, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), ISR_INT1, FALLING);
}

// Give the user negative feedback
void slapUser()
{
  int i = 0;
  for (i = 0; i < 5; i++)
  {
    digitalWrite(4, LOW);   // Buzzer on
    digitalWrite(5, LOW);   // LED on
    delay(100);
    digitalWrite(4, HIGH);  // Buzzer off
    digitalWrite(5, HIGH);  // LED off
    delay(100);
  }
}

void loop()
{
  unsigned long currentTime;

  digitalWrite(4, HIGH);  // sets the Wiegand Buzzer to off
  digitalWrite(5, HIGH);  // sets the Wiegand LED to off

  if (bitCount == 0) {
    return; // no data
  }

  currentTime = micros();
  if (lastBitReceiveUs > currentTime || currentTime - lastBitReceiveUs < WIEGAND_RECEIVE_WAIT_US) { // the two micros call will race each other, and unsigned math means negative number is large
    return; // still receiving data
  }

  // if we have bits and we the wiegand counter went out
  unsigned char i;

  // we will decode the bits differently depending on how many bits we have
  if (bitCount == 35)
  {
    // 35 bit HID Corporate 1000 format
    // 2 parity bits in front, 1 parity bit at end

//    // facility code = bits 2 to 14
//    for (i = 2; i < 14; i++)
//    {
//      facilityCode <<= 1;
//      facilityCode |= databits[i];
//    }
//
//    // card code = bits 15 to 34
//    for (i = 14; i < 34; i++)
//    {
//      cardCode <<= 1;
//      cardCode |= databits[i];
//    }

    // internal code = bits 2 to 34
    for (i = 2; i < 34; i++)
    {
      internalCode <<= 1;
      internalCode |= databits[i];
    }

    // even parity
    int evenparity = 0;
    for (i = 2; i < 34; i++) {
      everythird += 1;
      if (everythird == 3) {
        everythird = 0;
      } else {
        if (databits[i] == 1) {
          evenparity = (evenparity == 0 ? 1 : 0);
        }
      }
    }

    // odd parity
    oddparity = 1;
    for (i = 1; i < 34; i++) {
      everythird += 1;
      if (everythird == 3) {
        everythird = 0;
      } else {
        if (databits[i] == 1) {
          oddparity = (oddparity == 0 ? 1 : 0);
        }
      }
    }

    // fulloddparity
    fulloddparity = 1;
    for (i = 0; i < 34; i++) {
      if (databits[i] == 1) {
        fulloddparity = (fulloddparity == 0 ? 1 : 0);
      }
    }

    if (evenparity != databits[1] or oddparity != databits[0] or fulloddparity != databits[34])
    {
      slapUser();
    }
    else
    {
      // Clear any pending PIN digits
      pinCode = 0;
      pinKeys = 0;
      gotNonZero = 0;

      printInternalCode('C');
    }
  }
  else if (bitCount == 26)
  {
    // standard 26 bit format
    // 1 parity bit in front, 1 parity bit at end

    // Even Parity calculation
    evenparity = 0;
    for (i = 1; i < 13; i++)
    {
      // Serial.print (databits[i]);
      if (databits[i] == 1)
        evenparity = (evenparity == 1 ? 0 : 1);
    }
    // Odd Parity calculation
    oddparity = 1;
    for (i = 13; i < 25; i++)
    {
      // Serial.print (databits[i]);
      if (databits[i] == 1)
        oddparity = (oddparity == 1 ? 0 : 1);
    }

//    // facility code = bits 2 to 9
//    for (i = 1; i < 9; i++)
//    {
//      facilityCode <<= 1;
//      facilityCode |= databits[i];
//    }
//
//    // card code = bits 10 to 25
//    for (i = 9; i < 25; i++)
//    {
//      cardCode <<= 1;
//      cardCode |= databits[i];
//    }

    // internal code = bits 2 to 25
    for (i = 1; i < 25; i++)
    {
      internalCode <<= 1;
      internalCode |= databits[i];
    }

    if (evenparity != databits[0] or oddparity != databits[25])
    {
      slapUser();
    }
    else
    {
      // Clear any pending PIN digits
      pinCode = 0;
      pinKeys = 0;
      gotNonZero = 0;

      printInternalCode('C');
    }
  }
  else if (bitCount == 4)
  {
    // PIN 4 bit format
    // no parity bits
    // internal code = bits 0 to 4
    // ESC (1010) clears pin, ENT (1011) sends pin.
    for (i = 0; i < 4; i++)
    {
      internalCode <<= 1;
      internalCode |= databits[i];
    }

    if (internalCode <= 9 || internalCode == KEYPAD_ENT)
    {
      // Clear pending pin if enough time has passed since the previous digit
      currentTime = millis();
      if (currentTime >= lastPinTimeMs + 1000 * SECONDS_UNTIL_CLEAR)
      {
        // Clear pending PIN
        pinCode = 0;
        pinKeys = 0;
        gotNonZero = 0;
      }
      lastPinTimeMs = currentTime;
    }

    if (internalCode <= 9)
    {
      if (pinKeys >= PINMAX)
      {
        // Too many digits entered
        slapUser();

        // Clear the PIN
        pinCode = 0;
        pinKeys = 0;
        gotNonZero = 0;
      }
      else
      {
        // Accept a digit
        pinCode = pinCode * 10 + internalCode;
        if (internalCode != 0)
          // We got something other than a leading zero
          gotNonZero = 1;
        if (internalCode != 0 || gotNonZero)
          // Count significant digit
          pinKeys++;
      }
    }
    else if (internalCode == KEYPAD_ESC)
    {
      // Clear the PIN
      pinCode = 0;
      pinKeys = 0;
      gotNonZero = 0;
    }
    else if (internalCode == KEYPAD_ENT)
    {
      if (pinKeys < PINMIN)
      {
        // Too few digits entered
        slapUser();

        // Clear the PIN
        pinCode = 0;
        pinKeys = 0;
        gotNonZero = 0;
      }
      else
      {
        // Accept the PIN
        internalCode = pinCode;
        printInternalCode('P');

        // For the next person
        pinCode = 0;
        pinKeys = 0;
        gotNonZero = 0;
      }
    }
  }
  else {
    // you can add other formats if you want!
    slapUser();
  }

  // cleanup and get ready for the next card
  bitCount = 0;
  facilityCode = 0;
  cardCode = 0;
  internalCode = 0;
  for (i = 0; i < MAX_BITS; i++)
  {
    databits[i] = 0;
  }
}

void printInternalCode(char flagType)
{
  Serial.write(flagType);
  Serial.print("\t");
  Serial.println(internalCode);
}