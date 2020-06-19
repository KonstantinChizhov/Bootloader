#pragma once

#include <stddef.h>
#include <stdint.h>


namespace Bootloader
{
    bool DecryptPage(const uint8_t *input, size_t inputSize, uint8_t *output, size_t *outputSize);

   } // namespace Bootloader