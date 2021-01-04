====================
EEWL Arduino Library
====================

EEWL, EEPROM wear leveling.
EEPROMs have a limited number of write cycles, approximately in the
order of 100000 cycles, the EEWL library allows to extend the EEPROM life
by distributing data writes along a circular buffer.

The EEWL library is well suited for storing both simple C++ data types
(char, int, etc.) and structured data like C++ struct.
EEPROMs are mainly used to store data that change unfrequently and need
to be preserved accross power up/down cycles. The system configuration
parameters are a typical example of such kind of data. This data can be
fitted into a single data block defined by a C++ struct that can be easily
managed by the EEWL library. When something is changed in the block and
needs to be saved, the EEWL library store the whole block into EEPROM
with a single call to the library. Similarly, the block is read.

The original EEPROM life is extended by a multiplication factor equal to
the circular buffer len. The user can control this factor defining the
len of the circular buffer.

When in the same system, there are data that have quite different change
rate, they can be assigned to different data blocks, providing to keep data
with similar change rate in the same block. In this way, it is possible
to give a different length to the circular buffer of each block to obtain
the same life time extension for all data. The EEWL library can manage
several types of data blocks at the same time to achieve this goal.


Features
========

* API interface like Arduino EEPROM library.
* Any C++ data type is allowed.
* Multiple data amd multiple types can be managed as a single C++ struct.
* Same application, multiple buffers are allowed.
* Very low EEPROM data overhead: 1 byte for each circular buffer block. 
* Depends only on Arduino EEPROM library.


Quick start
===========

Here is a typical structure of an Arduino application using the EEWL library.
It shows the scheme of a very simple management of system parameters that
need to be saved to EEPROM.

.. code:: cpp

  ...

  // EEPROM circular buffer configuration
  #define BUFFER_START 0x4      // buffer start address
  #define BUFFER_LEN 10         // number of data blocks

  #include "eewl.h"

  ...

  // define a structure of data to be saved into EEPROM (an example).
  // User can put here the initialization values that will be in effect
  // when the application will be run the first time ever.
  struct
  {
    int parameter1 = 1;
    char parameter2[4] = {'a','b','c','d'};
    double parameter3 = 0.123456789;
  }
    systemParameters;

  // create EEWL object managing data
  EEWL sysParms(systemParameters, BUFFER_LEN, BUFFER_START_ADD);
  ...


  void setup ()
  {
    ...

    // get saved system parameters or their initial values.
    // From now, a copy of the system parameters is loaded into struct
    // systemParameters available to the application.
    sysParms.get(systemParameters);

    ...
  }


  void loop()
  {
    ...
    ...

    if (isProgramTerminating && isSystemParametersChanged)
      // application is terminating: save system parameters.
      sysParms.put(systemParameters);

    ...
  }


If the application needs to differentiate the processing of the system
parameters between the first time run and subsequent runs, the following
code shows a solution.

.. code:: cpp

  void setup()
  {
    ...
    ...

    if (sysParms.get(systemParameters))
    // system parameter where saved into EEPROM by the previous application 
    // run. They are reloaded into systemParameters.
    {
      // put here the code to process the system parameters left by
      // the previous application run.  
    }
    else
    // this is the first time ever the application is run. System parameters
    // are set to the initial values specified in systemParametrs definition.
    {
      // put here the code to process the system parameters when the
      // application runs the first time ever.
    }

    ...
    ...
  }


EEPROM life extension
=====================

Here there are some simple criteria for sizing the circular buffer to obtain
the required life extension of EEPROM memory. To be clearer let's refer to a
popular type of processor used on Arduino boards: the AVR 328p. In
this case, the datasheet tells us that there is an EEPROM memory with a size
of 1K bytes and an expected life of 100000 write/erase cycles. 

Let see a case where the application need to keep the cumulated up
time (the powered up elapsed time) and the system hardware running the 
application is able to give an alert to the application just before the
power off. In such a case, the application simply needs to read the up
time from the last power up (output of the millis function), to add it
to the cumulated up time (an unsigned long, 4 bytes) and to save it to
the EEPROM. Each saving of the cumulated up time requires 5 bytes, 4 bytes
for data and 1 byte overhead for the management of the circular buffer.
If the specifications require a system life of 1,000,000 power cycles,
an EEPROM life extension by a factor of 10 is required. This can be
obtained by EEWL library specifying a circular buffer len of 10. Since
each write to the circular buffer requires 5 bytes, altogether 50 bytes
of EEPROM are needed.


Module reference
================

The EEWL library is implemented as a single C++ class. An EEWL object needs
to be instantiated with the proper parameters to manage the write/read
operations in the circular buffer.


Objects and methods
-------------------

**EEWL**

  This class embeds all EEWL object status info.


EEWL **EEWL** (**data**, int **blk_num**, int **start_add**);

  The class constructor.

  **data**: data to be written into EEPROM. It may be any data
  type of C++. Once defined, the data type can't be changed.

  **blk_num**: circular buffer length as number of data blocks.

  **start_add**: EEPROM start address where to allocate the circular buffer.

  Returns an **EEWL** object.


void **fastFormat** (void);

  Format only essential metadata of circular buffer. Required to be run one
  time before the first EEPROM put/get. Automatically called by class
  contructor. Can be called to clear the whole circular buffer.

 
void **put** (**data**);

  Save **data** into the EEPROM circular buffer.

  **data**: data to be written into EEPROM. It must be the same data
  type specified in the class constructor.

 
bool **get** (**data**);

  Read from EEPROM circular buffer into **data**.

  **data**: data where to write data read from EEPROM. It must be the same
  data type specified in the class constructor.

  Returns **true** if there is saved data. Returns **false** if there is
  no saved data.


Examples
========

See the "examples" directory.


Installing
==========

By arduino IDE library manager or by unzipping EEWL.zip into
arduino libraries.


Contributing
============

Send wishes, comments, patches, etc. to mxgbot_a_t_gmail.com .


Copyright
=========

EEWL library is authored by Fabrizio Pollastri <mxgbot_a_t_gmail.com>,
year 2017-2021, under the GNU Lesser General Public License version 3.

.. ==== END
