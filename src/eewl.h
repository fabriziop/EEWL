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
  by distributing data writes along a circular buffer. EEWL can work also
  with a RAM buffer for testing pourposes.

.warning
  Start address = 0 not allowed.

.compile_options
  1. debugging printout methods, to include them define symbol EEWL_DEBUG.
  2. use RAM instead of EEPROM, to activate define symbol EEWL_RAM.

.- */

#ifndef EEWL_H
#define EEWL_H


/**** macros ****/

#ifdef EEWL_RAM
  #define EE_WRITE( addr , val ) (buffer[(int)( addr )] = val)
  #define EE_READ( addr ) (buffer[(int)( addr )])
#else
  #ifdef __AVR__
    #define EE_WRITE( addr , val ) (EEPROM[ (int)( addr )  ].update( val ))
  #elif defined(ESP8266) || defined(ESP32) || defined(TARGET_RP2040)
    #define EE_WRITE( addr , val ) (EEPROM.write((int)( addr ),( val )))
  #else
    #error ERROR: unsupported architecture
  #endif
  #ifdef ESP32
    #define EE_READ( addr ) (EEPROM.read((int)( addr )))
  #else
    #define EE_READ( addr ) (EEPROM[ (int)( addr ) ])
  #endif
  #include <EEPROM.h>
#endif


/**** class ****/

struct EEWL {

  // control vars
  int blk_num;
  int blk_size;
  int blk_addr;
  int blk_mark = 0xfe;
  int start_addr;
  int end_addr;

  #if (defined(ESP8266) || defined(ESP32)) || defined(TARGET_RP2040) && !defined(EEWL_RAM)
  static inline int highest_end_addr = 0;
  static inline bool eepromBeginDone = false;
  #endif

  #ifdef EEWL_RAM
  uint8_t *buffer;
  #endif


  // member functions

  // class constructor
  template <typename T> EEWL(T &data, int blk_num_, int start_addr_) {

    (void)data;

    // allocate and init control vars
    blk_size = sizeof(T) + 1;
    blk_num = blk_num_;
    start_addr = start_addr_;
    end_addr = start_addr + blk_num * blk_size;

    #if defined(EEWL_RAM)
    buffer = (uint8_t *)malloc(end_addr);
    #elif defined(ESP8266) || defined(ESP32) || defined(TARGET_RP2040)
    if (highest_end_addr < end_addr)
      highest_end_addr = end_addr;
    #endif

  }


  // class initializer
  void begin() {

    #if !defined(EEWL_RAM) && !defined(__AVR__)
    if (!eepromBeginDone) {
      #if defined(ESP8266) || defined(TARGET_RP2040)
      EEPROM.begin((highest_end_addr / 256 + 1) * 256);
      #elif defined(ESP32)
      if (!EEPROM.begin((highest_end_addr / 256 + 1) * 256)) {
        Serial.println("ERROR: EEPROM init failure");
        while(true) delay(1000);
      }
      delay(500);
      #endif
      eepromBeginDone = true;
    }
    #endif

    // search for a valid current data
    blk_addr = 0;
    int blocks_addr[2];
    int blocks_count = 0;
    for (int addr = start_addr; addr < end_addr; addr += blk_size) {

      // find up to two occurrences of a valid data marker
      if (EE_READ(addr) != 0xff) {

	// if more than two valid block, formatting is needed
	if (++blocks_count > 2) {
          fastFormat();
          return;
	}

        blocks_addr[blocks_count-1] = addr;

      }
    }

    // manage the number of valid blocks found (0,1,2)
    switch (blocks_count) {

      // buffer is empty
      case 0:
	break;

      // regular case: one valid data block.
      case 1:
	blk_addr = blocks_addr[0];
	break;

      // incomplete put execution: interrupted by power off.
      // in this case the two block must be consecutive, check it.
      case 2:

	// regular consecutive blocks
	if (blocks_addr[0] + blk_size == blocks_addr[1]) {
	  blk_addr = blocks_addr[1];
	  EE_WRITE(blocks_addr[0],0xff);
	  break;
	}

	// consecutive blocks wrapped around the end of buffer
	if (blocks_addr[0] == start_addr) {
	  blk_addr = start_addr;
	  EE_WRITE(blocks_addr[1],0xff);
	  break;
	}

	// non consecutive blocks, formatting is needed.
	fastFormat();

    }
  }


  // format essential metadata of circular buffer, buffer is logically cleared.
  void fastFormat(void) {

    // set all data status bytes as free
    for (int addr = start_addr; addr < end_addr; addr += blk_size)
      EE_WRITE(addr,0xff);

    // mark no valid data available
    blk_addr = 0;

    #if (defined(ESP8266) || defined(ESP32)) || defined(TARGET_RP2040) && !defined(EEWL_RAM)
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
      *ptr++ = EE_READ(data_addr);

    // return success to mark presence of valid data
    return 1;

  }


  // write data to EEPROM
  template <typename T> void put(T &data) {

    int old_blk_addr;
    int new_blk_addr;

    // if data already stored in buffer ...
    if (blk_addr) {

      // save current block address and mark and set new mark as free
      old_blk_addr = blk_addr;
      blk_mark = EE_READ(blk_addr);

      // point to next data block
      blk_addr += blk_size;
      if (blk_addr >= end_addr)
        blk_addr = start_addr;
    }
    // else: no data already stored in buffer ...
    else {
      old_blk_addr = 0;
      blk_addr = start_addr;
    }

    // save new block address
    new_blk_addr = blk_addr;

    // write data
    blk_mark <<= 1;
    blk_mark |= 1;
    blk_mark &= 0xff;
    if (blk_mark == 0xff)
      blk_mark = 0xfe;
    uint8_t *ptr = (uint8_t *) &data;
    for(int data_addr = blk_addr + 1; data_addr < blk_addr + blk_size; data_addr++)
      EE_WRITE(data_addr,*ptr++);

    // write data block data mark
    EE_WRITE(new_blk_addr,blk_mark);

    // if it exists, mark old block as free
    if (old_blk_addr)
      EE_WRITE(old_blk_addr,0xff);

    #if (defined(ESP8266) || defined(ESP32)) || defined(TARGET_RP2040) && !defined(EEWL_RAM)
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
      Serial.print(EE_READ(addr), HEX);
      Serial.print("-");
      int endaddr = addr + blk_size;
      for (++addr; addr < endaddr; addr++) {
        Serial.print(EE_READ(addr),HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }

#endif

};

#endif

/**** end ****/
