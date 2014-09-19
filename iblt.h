#ifndef IBLT_H
#define IBLT_H

#include <inttypes.h>
#include <set>
#include <vector>

//
// Invertible Bloom Lookup Table implementation
// References:
//
// "What's the Difference? Efficient Set Reconciliation
// without Prior Context" by Eppstein, Goodrich, Uyeda and
// Varghese
//
// "Invertible Bloom Lookup Tables" by Goodrich and
// Mitzenmacher
//

class IBLT
{
public:
    IBLT(size_t _expectedNumEntries, size_t _ValueSize);
    virtual ~IBLT();

    void insert(uint64_t k, const std::vector<uint8_t> v);
    void erase(uint64_t k, const std::vector<uint8_t> v);

    // Returns true if a result is definitely found or not
    // found. If not found, result will be empty.
    // Returns false if overloaded and we don't know whether or
    // not k is in the table.
    bool get(uint64_t k, std::vector<uint8_t>& result) const;

    // Returns true and all key/value pairs in result if all can be
    // decoded; otherwise, returns false and as many key/value pairs as
    // could be decoded
    bool listEntries(std::set<std::pair<uint64_t,std::vector<uint8_t> > >& result) const;

    // ??? this?  Or write an IBLTIterator class ?
    // template<typename output_iterator>
    // bool listEntries(output_iterator saveTo) const;

    std::string DumpTable() const;

private:
    void _insert(int plusOrMinus, uint64_t k, const std::vector<uint8_t> v);

    size_t valueSize;

    class HashTableEntry
    {
    public:
        size_t count;
        uint64_t keySum;
        uint32_t keyCheck;
        std::vector<uint8_t> valueSum;

        bool isPure() const;
    };

    std::vector<HashTableEntry> hashTable;
};

// TODO: operator+ and operator-

#endif /* IBLT_H */
