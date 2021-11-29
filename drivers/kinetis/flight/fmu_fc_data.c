#include "Ano_FcData.h"
#include "Ano_Parameter.h"


_flag flag;




void data_save(void)
{
    para_sta.save_en = !flag.unlock_sta;
    para_sta.save_trig = 1;
}

