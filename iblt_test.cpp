#include <cassert>
#include <iostream>

#include "iblt.h"
#include "murmurhash3.h"
#include "utilstrencodings.h"

#define T(expected, seed, data) \
    { \
        uint32_t result = MurmurHash3(seed, ParseHex(data)); \
        assert(result == expected); \
    }
    

std::vector<uint8_t> PseudoRandomValue(uint32_t n)
{
    std::vector<uint8_t> result;
    for (int i = 0; i < 4; i++) {
        result.push_back(static_cast<uint8_t>(MurmurHash3(n+i, result) & 0xff));
    }
    return result;
}

void TestMurmurHash()
{
    T(0x00000000, 0x00000000, "");
    T(0x6a396f08, 0xFBA4C795, "");
    T(0x81f16f39, 0xffffffff, "");

    T(0x514e28b7, 0x00000000, "00");
    T(0xea3f0b17, 0xFBA4C795, "00");
    T(0xfd6cf10d, 0x00000000, "ff");

    T(0x16c6b7ab, 0x00000000, "0011");
    T(0x8eb51c3d, 0x00000000, "001122");
    T(0xb4471bf8, 0x00000000, "00112233");
    T(0xe2301fa8, 0x00000000, "0011223344");
    T(0xfc2e4a15, 0x00000000, "001122334455");
    T(0xb074502c, 0x00000000, "00112233445566");
    T(0x8034d2a0, 0x00000000, "0011223344556677");
    T(0xb4698def, 0x00000000, "001122334455667788");    
}

void TestInsertErase()
{
    IBLT t(20, 4);
    t.insert(0,  ParseHex("00000000"));
    t.insert(1,  ParseHex("00000001"));
    t.insert(11, ParseHex("00000011"));

    bool gotResult;
    std::vector<uint8_t> result;
    gotResult = t.get(0, result);
    assert(gotResult && result == ParseHex("00000000"));
    gotResult = t.get(11, result);
    assert(gotResult && result == ParseHex("00000011"));

    t.erase(0,  ParseHex("00000000"));
    t.erase(1,  ParseHex("00000001"));
    gotResult = t.get(1, result);
    assert(gotResult && result.empty());
    t.erase(11, ParseHex("00000011"));
    gotResult = t.get(11, result);
    assert(gotResult && result.empty());

    t.insert(0,  ParseHex("00000000"));
    t.insert(1,  ParseHex("00000001"));
    t.insert(11, ParseHex("00000011"));

    for (int i = 100; i < 115; i++) {
        t.insert(i, ParseHex("aabbccdd"));
    }

    gotResult = t.get(101, result);
    assert(gotResult && result == ParseHex("aabbccdd"));
    gotResult = t.get(200, result);
    assert(gotResult && result.empty());
}

void TestOverload()
{
    IBLT t(20, 4);

    // 1,000 values in an IBLT that has room for 20,
    // all lookups should fail.
    for (int i = 0; i < 1000; i++) {
        t.insert(i, PseudoRandomValue(i));
    }
    bool gotResult;
    std::vector<uint8_t> result;
    for (int i = 0; i < 1000; i+= 97) {
        gotResult = t.get(i, result);
        assert(!gotResult && result.empty());
    }

    // erase all but 20:
    for (int i = 20; i < 1000; i++) {
        t.erase(i, PseudoRandomValue(i));
    }
    for (int i = 0; i < 20; i++) {
        gotResult = t.get(i, result);
        assert(gotResult && result == PseudoRandomValue(i));
    }
}

void TestList()
{
    std::set<std::pair<uint64_t,std::vector<uint8_t> > > expected;
    IBLT t(20, 4);
    for (int i = 0; i < 20; i++) {
        t.insert(i, PseudoRandomValue(i*2));
        expected.insert(std::make_pair(i, PseudoRandomValue(i*2)));
    }
    std::set<std::pair<uint64_t,std::vector<uint8_t> > > entries;
    bool fAllFound = t.listEntries(entries);
    assert(fAllFound && entries == expected);
}

int main()
{
    TestMurmurHash();

    TestInsertErase();
    TestOverload();
    TestList();

    std::cout << "Tests successful.\n";

    return(0);
}
