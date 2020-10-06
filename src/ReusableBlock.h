//
// Fulcrum - A fast & nimble SPV Server for Bitcoin Cash
// Copyright (C) 2019-2020  Calin A. Culianu <calin.culianu@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program (see LICENSE.txt).  If not, see
// <https://www.gnu.org/licenses/>.
//
#pragma once

#include "BlockProcTypes.h"
#include "BTC.h"
#include "TXO_Compact.h"
#include "Util.h"

#include "bitcoin/streams.h"
#include "bitcoin/transaction.h"
#include "tsl/htrie_map.h"

#include <QString>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring> // for std::memcpy
#include <functional> // for std::hash

// Our prefixes are lower 4 bits inside char
using PrefixNibble = char;

// TODO provide description of data here
using PrefixMap = tsl::htrie_map<PrefixNibble, std::vector<TxNum>, tsl::ah::str_hash<PrefixNibble>, std::uint8_t>;

// we can save some space on disk by using uint32_t - max size is dependent on block, so if blocks overflow uint32::max txs this could fail
using HATSerializationVectorSizeType = uint32_t;

struct ReusableHATSerializer {
    QByteArray store;

    ReusableHATSerializer() {}

    template <typename T> // required support for uint64_t and float (WHY float? see https://github.com/Tessil/hat-trie note about serialization)
    void operator()(const T& value) {
        // Log() << "Serialize()(T) " << sizeof(T);
        store.append(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    void operator()(const PrefixMap::mapped_type& value) { // specialize for our list of TxNums
        // Log() << "Serialize()(T) " << sizeof(T);
        HATSerializationVectorSizeType size = value.size();
        store.append(reinterpret_cast<const char*>(&size), sizeof(HATSerializationVectorSizeType));
        store.append(reinterpret_cast<const char*>(value.data()), size * sizeof(TxNum));
    }

    void operator()(const char* value, std::size_t value_size) {
        // Log() << "Serialize()(CharT) " << (value_size * sizeof(CharT));
        store.append(reinterpret_cast<const char*>(value), value_size);
    }
};

struct ReusableHATDeserializer {
    QByteArray store;
    QByteArray::iterator iter;

    ReusableHATDeserializer(QByteArray store): store(store), iter(store.begin()) {}

    template <typename T> // required support for uint64_t and float (see note above)
    T operator()() {
        T value;
        Log() << "Deserializer()(T) " << sizeof(T);
        std::memcpy(reinterpret_cast<char*>(&value), &*iter, sizeof(T));
        iter += sizeof(T);
        return value;
    }

    PrefixMap::mapped_type operator()() { // specialization on our value type for deserialization
        Log() << "Deserializer()(T) ";
        HATSerializationVectorSizeType size = 0;
        std::memcpy(reinterpret_cast<char*>(&size), &*iter, sizeof(HATSerializationVectorSizeType));
        iter += sizeof(HATSerializationVectorSizeType);

        PrefixMap::mapped_type value(size, 0); // resize our vector so we can copy into it without causing explosion 
        std::memcpy(value.data(), &*iter, size * sizeof(TxNum));
        iter += size * sizeof(TxNum);

        return value;
    }

    void operator()(char* value_out, std::size_t value_size) {
        Log() << "Deserializer()(CharT) " << value_size;
        std::memcpy(value_out, &*iter, value_size);
        iter += value_size;
    }
};


/// TODO describe these
/// This stores prefixes -> txids
/// The prefixes are the last bytes of sha256 inputs
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

