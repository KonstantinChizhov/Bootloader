

#include <aes.hpp>
#include <string.h>

namespace Bootloader
{

    static const constexpr uint8_t Key[] = { AES_KEY };

    bool DecryptPage(const uint8_t *input, size_t inputSize, uint8_t *output, size_t *outputSize)
    {
        if (inputSize % AES_BLOCKLEN != 0)
        {
            return false;
        }

        for (size_t i = 0; i <= inputSize; i += AES_BLOCKLEN)
        {
            uint8_t buffer[AES_BLOCKLEN];
            memcpy(buffer, &input[i], AES_BLOCKLEN);
            AES_ctx ctx;
            AES_init_ctx(&ctx, Key);
            AES_ECB_decrypt(&ctx, buffer);
            memcpy(&output[i], buffer, AES_BLOCKLEN);
        }
        *outputSize = inputSize;
        return true;
    }

} // namespace Bootloader