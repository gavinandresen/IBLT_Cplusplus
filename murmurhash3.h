#ifndef MURMURHASH3_H
#define MURMURHASH3_H

#include <inttypes.h>
#include <vector>

extern uint32_t MurmurHash3(uint32_t nHashSeed, const std::vector<unsigned char>& vDataToHash);

#endif /* MURMURHASH3_H */
