/* .+

.context    : EEWL EEPROM wear level library
.title      : count power up/down cycles
.kind       : c++ source
.author     : Fabrizio Pollastri <mxgbot@gmail.com>
.site       : Revello - Italy
.creation   : 22-Jun-2022
.copyright  : (c) 2022 Fabrizio Pollastri
.license    : GNU Lesser General Public License

.description
  This application counts the number of power up/down cycles. At each
  power up this counter is read from EEPROM, incremented by one and
  saved again into EEPROM to be preserved accross power cycles.
  A circular buffer len of 10 is defined. This extends the EEPROM life
  10 times, about 1 million of power cycles.
  This application outputs the results of mehods dump_control and dump_buffer.
  
.- */

#define BUFFER_LEN 10     // number of data blocks (1 blk = 1 ulong)
#define BUFFER_START 0x10 // EEPROM address where buffer starts

#define EEWL_DEBUG        // include debug methods
#include "eewl.h"

unsigned long powerCycles = 0;
EEWL pC(powerCycles, BUFFER_LEN, BUFFER_START);


void setup()
{
  // initialize serial
  Serial.begin(9600);
  delay(1000);

  // init EEPROM buffer
  pC.begin();
    
  // read into powerCycles the previous counter value or default value (0)
  if (!pC.get(powerCycles))
    Serial.println("\nsetup: first time ever");

  // increment power cycles counter and save it into EEPROM
  powerCycles++;
  pC.put(powerCycles);

  // display power cycles count
  Serial.print("\npower cycles = ");
  Serial.println(powerCycles);
}


void loop()
{
  pC.dump_control();
  pC.dump_buffer();
  Serial.println("stop here");
  while(true) delay(1000);
}

/**** END ****/
