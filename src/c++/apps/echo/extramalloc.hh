// Copyright (c) Microsoft Corporation
// Licensed under the MIT license.


#ifndef ECHO_MALLOCBASELINE_H_
#define ECHO_MALLOCBASELINE_H_

#include <dmtr/sga.h>
#include "message.hh"


class malloc_baseline : public echo_message
{
    public: malloc_baseline(uint32_t field_size, string message_type);

    public: virtual void serialize_message(dmtr_sgarray_t &sga);
    public: virtual void deserialize_message(dmtr_sgarray_t &sga);
};

#endif
