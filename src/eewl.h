/* .+

.context    : EEWL EEPROM wear level library
.title      : EEWL class definition
.kind       : c++ source
.author     : Fabrizio Pollastri <mxgbot@gmail.com>
.site       : Torino - Italy
.creation   : 16-Nov-2017
.copyright  : (c) 2017 Fabrizio Pollastri
.license    : GNU Lesser General Public License

.description
  EEPROMs have a limited number of write cycles, approximately in the
  order of 100000 cycles, the EEWL library allows to extend the EEPROM life
  by distributing data writes along a circular buffer.

.warning
  Start address = 0 not allowed.

.- */

#ifndef EEWL_H
#define EEWL_H

#include "EEPROM.h"


struct EEWL {

  // control vars
  int blk_num;
  int blk_size;
  int blk_addr;
  int blk_mark = 0xfe;
  int start_addr;
  int end_addr;

  // member functions

  // class constructor
  template <typename T> EEWL(T &data, int blk_num_, int start_addr_) {

    // allocate and init control vars
    blk_size = sizeof(T) + 1;
    blk_num = blk_num_;
    start_addr = start_addr_;
    end_addr = start_addr + blk_num * blk_size;

    #ifdef ESP8266
    EEPROM.begin((blk_size * blk_num / 256 + 1) * 256);
    #endif

    // search for a valid current data
    blk_addr = 0;
    for (int addr = start_addr; addr < end_addr; addr += blk_size) {

      // save the first occurrence of a valid data marker
      if (EEPROM[addr] != 0xff) {
        blk_addr = addr;

        // check for other valid data markers
        for (addr += blk_size; addr < end_addr; addr += blk_size) {

          // if found, formatting is needed (only one valid data block allowed)
          if (EEPROM[addr] != 0xff) {
            fastFormat();
            break;
          }
        }
        break;
      }
    }
  }


  // format essential metadata of circular buffer, buffer is logically cleared.
  void fastFormat(void) {

    // set all data status bytes as free
    for (int addr = start_addr; addr < end_addr; addr += blk_size) {

      #ifdef ESP8266
      EEPROM.write(addr,0xff);
      #else
      EEPROM[addr].update(0xff);
      #endif

    }

    // mark no valid data available
    blk_addr = 0;

    #ifdef ESP8266
      EEPROM.commit();
    #endif

  }


  // read data from EEPROM 
  template <typename T> int get(T &data) {

    // if no valid data into eeprom, return a ram data null pointer
    if (!blk_addr) return 0;

    // else copy data from eeprom to ram
    uint8_t *ptr = (uint8_t *) &data;
    for(int data_addr = blk_addr + 1; data_addr < blk_addr + blk_size; data_addr++)
      *ptr++ = EEPROM[data_addr];

    // return success to mark presence of valid data
    return 1;
    
  }
 

  // write data to EEPROM
  template <typename T> void put(T &data) {

    // if data already stored in buffer ...
    if (blk_addr) {
      // save current block mark and set new mark as free
      blk_mark = EEPROM[blk_addr];
      #ifdef ESP8266
      EEPROM.write(blk_addr,0xff);
      #else
      EEPROM.update(blk_addr,0xff);
      #endif

      // point to next data block
      blk_addr += blk_size;
      if (blk_addr >= end_addr)
        blk_addr = start_addr;
    }
    else
      blk_addr = start_addr;

    // write data block data mark and data
    blk_mark <<= 1;
    blk_mark |= 1;
    blk_mark &= 0xff;
    if (blk_mark == 0xff)
      blk_mark = 0xfe;
    #ifdef ESP8266
    EEPROM.write(blk_addr,blk_mark);
    #else
    EEPROM.update(blk_addr,blk_mark);
    #endif
    EEPROM.put(blk_addr + 1,data);

    #ifdef ESP8266
    EEPROM.commit();
    #endif

  }


#ifdef EEWL_DEBUG

  void dump_control(void) {

    Serial.print("blk_size:   ");
    Serial.println(blk_size);
    Serial.print("blk_num:    ");
    Serial.println(blk_num);
    Serial.print("blk_addr:   ");
    Serial.println(blk_addr,HEX);
    Serial.print("blk_mark:   ");
    Serial.println(blk_mark,HEX);
    Serial.print("start_addr: ");
    Serial.println(start_addr,HEX);
    Serial.print("end_addr:   ");
    Serial.println(end_addr,HEX);

  }


  void dump_buffer(void) {

    for (int addr = start_addr; addr < end_addr;) {
      Serial.print(addr, HEX);
      Serial.print(": ");
      Serial.print(EEPROM[addr], HEX);
      Serial.print("-");
      int endaddr = addr + blk_size;
      for (++addr; addr < endaddr; addr++) {
        Serial.print(EEPROM[addr], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }

#endif

};

#endif
