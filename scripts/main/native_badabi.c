/* Test fixture for native_test: a library that reports an ABI version Dockly must refuse. */
#include <stdint.h>

__declspec(dllexport) uint32_t dn_abi_version(void)
{
    return 0xFFFFFFFFu;
}
