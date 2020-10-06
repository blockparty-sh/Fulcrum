#include "BlockProc.h"
#include "ReusableBlock.h"

struct ReusableBlock {
    constexpr static size_t NIBBLE_WIDTH = 4; // half octet
    constexpr static size_t MAX_BITS = 16; // must be a multiple of NIBBLE_WIDTH

    PrefixMap pmap; // Prefix map for efficient searching

    // perform serialization of a bitcoin input, the prefix of this will be indexed
    static RuHash serializeInput(const bitcoin::CTxIn& input) {
        bitcoin::CDataStream s(0, 0); // 0,0 is for the version types, which are not relevant for us here
        input.Serialize(s);
        RuHash ruHash = BTC::Hash(QByteArray(s.data(), s.size()), false); // double sha2
        return ruHash;
    }

    // split hash into little nibbles so we can perform prefix queries on 4 bit sections
    // in future this can be expanded if we want even more fine grained queries, this (16bit) currently allows for 1/65536
    static std::string ruHashToPrefix(QByteArray a) {
        assert(a.size() >= 2);
        return {
            (a[0] & 0xF0) >> NIBBLE_WIDTH, a[0] & 0x0F,
            (a[1] & 0xF0) >> NIBBLE_WIDTH, a[1] & 0x0F
        };
    }

    auto prefixSearch(const std::string& prefix) {
        return pmap.equal_prefix_range(prefix);
    }

    bool isValid() { return true; } // TODO implement

    void add(const RuHash& ruHash, const TxNum n) {
        // Log() << Util::ToHexFast(ruHash) << ":" << n;
        // we could calculate masks from nibble width but it makes code harder to read
        // split the input hash by every 4 bits
        // we can use prefix search on htrie to handle wider scans
        std::string prefix = ReusableBlock::ruHashToPrefix(ruHash);

        auto it = pmap.find(prefix);
        if (it == pmap.end()) {
            pmap.insert(prefix, { n });
        } else {
            (*it).push_back(n);
        }
    }

    // serialization
    QByteArray toBytes() const noexcept {
        ReusableHATSerializer serializer;
        // Log() << "serialize ReusableBlock";
        pmap.serialize(serializer);
        // Log() << "serialized " << serializer.store.size();
        return serializer.store;
    }

    // deserialization
    static ReusableBlock fromBytes(const QByteArray &ba) noexcept {
        ReusableHATDeserializer deserializer(ba);
        ReusableBlock ret;
        Log() << "Deserialize ReusableBlock " << ba.size();
        Log() << Util::ToHexFast(ba);
        ret.pmap = PrefixMap::deserialize(deserializer);
        Log() << "Deserialized " << ret.pmap.size();
        return ret;
    }
};


