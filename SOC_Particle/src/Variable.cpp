#include "Variable.h"
#include "parameters.h"
#include "hardware/SerialRAM.h"
#include "mySensors.h"


// Print bitmap
void bitMapPrint(char *buf, const int16_t fw, const uint8_t num)
{
  for ( int i=0; i<num; i++ )
  {
    if ( bitRead(fw, i) ) buf[num-i-1] = '1';
    else  buf[num-i-1] = '0';
  }
  buf[num] = '\0';
}


Vars::Vars(SavedPars *sp)
{
    sp_ = sp;
}

// Manage changes to modeling configuration
void Vars::put_Modeling(const uint8_t input, Sensors *Sen)
{
    sp_->put_Modeling(input);  // uint8_t pointers change after instantiation!  TODO:  why?
    Sen->ShuntAmp->dscn_cmd(mod_ib_amp_dscn());
    Sen->ShuntNoAmp->dscn_cmd(mod_ib_noa_dscn());
}
