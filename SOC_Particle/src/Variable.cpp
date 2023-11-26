#include "Variable.h"
#include "parameters.h"

#include "hardware/SerialRAM.h"

// extern SavedPars sp;              // Various parameters to be static at system level and saved through power cycle
// extern SerialRAM ram;

Vars::Vars(SavedPars sp, SerialRAM *ram)
{
    Freq = Variable<float>(sp.freq_ptr(), ram, sp.freq_eeram(), "Inj freq", "Hz");
    Ib_select = Variable<int8_t>(sp.ib_select_ptr(), ram, sp.ib_select_eeram(), "vcurr select mode (-1=noa, 0=auto, 1=amp)", "int");
}
