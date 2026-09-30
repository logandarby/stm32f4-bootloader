#ifndef F24B386B_58DC_434C_B528_7A28BD3CF42B
#define F24B386B_58DC_434C_B528_7A28BD3CF42B

#include "common.h"
#include "mldsa_native.h"

// NOTE: This is not cryptographically secure. This should be stored in OTP
// if put into production, but I don't want to do that.
extern const uint8_t MLDSA44_PUBLIC_KEY[MLDSA44_PUBLICKEYBYTES];

#endif /* F24B386B_58DC_434C_B528_7A28BD3CF42B */
