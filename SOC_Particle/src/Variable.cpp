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


Vars::Vars(SavedPars *sp, SerialRAM *ram)
{
    sp_ = sp;
    Freq = Variable<float>(sp->freq_ptr(), ram, sp->freq_eeram(), "Inj freq", "Hz");
    Ib_select = Variable<int8_t>(sp->ib_select_ptr(), ram, sp->ib_select_eeram(), "curr sel mode (-1=noa, 0=auto, 1=amp)", "code");
    Modeling = Variable<uint8_t>(sp->modeling_ptr(), ram, sp->modeling_eeram(), " modeling bitmap [0b00000000]", "bitpacked word", true);
}


// Manage changes to modeling configuration
void Vars::put_Modeling(const uint8_t input, Sensors *Sen)
{
    sp_->put_modeling(input);  // uint8_t pointers change after instantiation!  TODO:  why?
    Modeling.set(input);
    Sen->ShuntAmp->dscn_cmd(mod_ib_amp_dscn());
    Sen->ShuntNoAmp->dscn_cmd(mod_ib_noa_dscn());
}

void Vars::put_Ib_select(const uint8_t input, Sensors *Sen)
{
    sp_->put_ib_select(input);  // int pointers change after instantiation!  TODO:  why?
    Ib_select.set(input);
}
