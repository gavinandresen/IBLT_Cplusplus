#include <cassert>
#include <iostream>
#include <list>
#include <sstream>
#include <utility>
#include "iblt.h"
#include "murmurhash3.h"

static const size_t N_HASH = 4;
static const size_t N_HASHCHECK = 11;

template<typename T>
std::vector<uint8_t> ToVec(T number)
{
    std::vector<uint8_t> v(sizeof(T));
    for (size_t i = 0; i < sizeof(T); i++) {
        v.at(i) = (number >> i*8) & 0xff;
    }
    return v;
}


bool IBLT::HashTableEntry::isPure() const
{
    if (count == 1 || count == -1) {
        uint32_t check = MurmurHash3(N_HASHCHECK, ToVec(keySum));
        return (keyCheck == check);
    }
    return false;
}

bool IBLT::HashTableEntry::empty() const
{
    return (count == 0 && keySum == 0 && keyCheck == 0);
}

void IBLT::HashTableEntry::addValue(const std::vector<uint8_t> v)
{
    if (v.empty()) {
        return;
    }
    if (valueSum.size() < v.size()) {
        valueSum.resize(v.size());
    }
    for (size_t i = 0; i < v.size(); i++) {
        valueSum[i] ^= v[i];
    }
}

IBLT::IBLT(size_t _expectedNumEntries, size_t _valueSize) :
    valueSize(_valueSize)
{
    // 1.5x expectedNumEntries gives very low probability of
    // decoding failure
    size_t nEntries = _expectedNumEntries + _expectedNumEntries/2;
    // ... make nEntries exactly divisible by N_HASH
    while (N_HASH * (nEntries/N_HASH) != nEntries) ++nEntries;
    hashTable.resize(nEntries);
}

IBLT::IBLT(const IBLT& other)
{
    valueSize = other.valueSize;
    hashTable = other.hashTable;
}

IBLT::~IBLT()
{
}

void IBLT::_insert(int plusOrMinus, uint64_t k, const std::vector<uint8_t> v)
{
    assert(v.size() == valueSize);

    std::vector<uint8_t> kvec = ToVec(k);

    size_t bucketsPerHash = hashTable.size()/N_HASH;
    for (size_t i = 0; i < N_HASH; i++) {
        size_t startEntry = i*bucketsPerHash;

        uint32_t h = MurmurHash3(i, kvec);
        IBLT::HashTableEntry& entry = hashTable.at(startEntry + (h%bucketsPerHash));
        entry.count += plusOrMinus;
        entry.keySum ^= k;
        entry.keyCheck ^= MurmurHash3(N_HASHCHECK, kvec);
        if (entry.empty()) {
            entry.valueSum.clear();
        }
        else {
            entry.addValue(v);
        }
    }
}

void IBLT::insert(uint64_t k, const std::vector<uint8_t> v)
{
    _insert(1, k, v);
}

void IBLT::erase(uint64_t k, const std::vector<uint8_t> v)
{
    _insert(-1, k, v);
}

bool IBLT::get(uint64_t k, std::vector<uint8_t>& result) const
{
    result.clear();

    std::vector<uint8_t> kvec = ToVec(k);

    size_t bucketsPerHash = hashTable.size()/N_HASH;
    for (size_t i = 0; i < N_HASH; i++) {
        size_t startEntry = i*bucketsPerHash;

        uint32_t h = MurmurHash3(i, kvec);
        const IBLT::HashTableEntry& entry = hashTable.at(startEntry + (h%bucketsPerHash));

        if (entry.empty()) {
            // Definitely not in table. Leave
            // result empty, return true.
            return true;
        }
        else if (entry.isPure()) {
            if (entry.keySum == k) {
                // Found!
                result.assign(entry.valueSum.begin(), entry.valueSum.end());
                return true;
            }
            else {
                // Definitely not in table.
                return true;
            }
        }
    }

    // Don't know if k is in table or not; "peel" the IBLT to try to find
    // it:
    IBLT peeled = *this;
    size_t nErased = 0;
    for (size_t i = 0; i < peeled.hashTable.size(); i++) {
        IBLT::HashTableEntry& entry = peeled.hashTable.at(i);
        if (entry.isPure()) {
            if (entry.keySum == k) {
                // Found!
                result.assign(entry.valueSum.begin(), entry.valueSum.end());
                return true;
            }
            ++nErased;
            peeled._insert(-entry.count, entry.keySum, entry.valueSum);
        }
    }
    if (nErased > 0) {
        // Recurse with smaller IBLT
        return peeled.get(k, result);
    }
    return false;
}

bool IBLT::listEntries(std::set<std::pair<uint64_t,std::vector<uint8_t> > >& positive,
                       std::set<std::pair<uint64_t,std::vector<uint8_t> > >& negative) const
{
    IBLT peeled = *this;

    size_t nErased = 0;
    do {
        nErased = 0;
        for (size_t i = 0; i < peeled.hashTable.size(); i++) {
            IBLT::HashTableEntry& entry = peeled.hashTable.at(i);
            if (entry.isPure()) {
                if (entry.count == 1) {
                    positive.insert(std::make_pair(entry.keySum, entry.valueSum));
                }
                else {
                    negative.insert(std::make_pair(entry.keySum, entry.valueSum));
                }
                peeled._insert(-entry.count, entry.keySum, entry.valueSum);
                ++nErased;
            }
        }
    } while (nErased > 0);

    // If any buckets for one of the hash functions is not empty,
    // then we didn't peel them all:
    for (size_t i = 0; i < peeled.hashTable.size()/N_HASH; i++) {
        if (peeled.hashTable.at(i).empty() != true) return false;
    }
    return true;
}

IBLT IBLT::operator-(const IBLT& other)
{
    // IBLT's must be same params/size:
    assert(valueSize == other.valueSize);
    assert(hashTable.size() == other.hashTable.size());

    IBLT result(*this);
    for (size_t i = 0; i < hashTable.size(); i++) {
        IBLT::HashTableEntry& e1 = result.hashTable.at(i);
        const IBLT::HashTableEntry& e2 = other.hashTable.at(i);
        e1.count -= e2.count;
        e1.keySum ^= e2.keySum;
        e1.keyCheck ^= e2.keyCheck;
        if (e1.empty()) {
            e1.valueSum.clear();
        }
        else {
            e1.addValue(e2.valueSum);
        }
    }

    return result;
}

// For debugging during development:
std::string IBLT::DumpTable() const
{
    std::ostringstream result;

    result << "count keySum keyCheckMatch\n";
    for (size_t i = 0; i < hashTable.size(); i++) {
        const IBLT::HashTableEntry& entry = hashTable.at(i);
        result << entry.count << " " << entry.keySum << " ";
        result << (MurmurHash3(N_HASHCHECK, ToVec(entry.keySum)) == entry.keyCheck ? "true" : "false");
        result << "\n";
    }

    return result.str();
}
