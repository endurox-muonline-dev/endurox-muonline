#ifndef _protocol_h_
#define _protocol_h_
#include <cstdint>
#include <ubf.h>
#include "driver.h"

extern pg_driver *M_db_handle;

void protocol_core(UBFH *&buffer_, uint8_t header_, uint8_t *message_, int len_);

#endif