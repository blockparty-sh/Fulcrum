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

#include "robin_hood/robin_hood.h" // TODO do we need this here or in TXO.h ?
#include "tsl/htrie-map.h"

#include <QString>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring> // for std::memcpy
#include <functional> // for std::hash
#include <optional>

// Our prefixes are lower 4 bits inside char
using PrefixNibble = char;

/// TODO describe these
/// This stores prefixes -> txids
/// The prefixes are the last bytes of sha256 inputs
struct ReusableBlock {
    std::vector<TxHash> txHashes; // TODO do we need this, or can we just use the Storage system to grab quickly from block given pos?
    tsl::htrie_map<PrefixNibble, TxNum> pmap; // Prefix map for efficient searching

    // bool isValid() const { return txHashes.size() == pmap.size();  }
    // QString toString() const;

    // bool operator==(const ReusableBlock &o) const noexcept { return txHashes == o.txHashes && pmap == o.pmap; } // TODO do we need this?
    // bool operator<(const TXO &o) const noexcept { return txHash < o.txHash && outN < o.outN; }


    // serialization/deserialization
    QByteArray toBytes() const noexcept {
        QByteArray ret;
        if (!isValid()) return ret;
        const size_t hlen = txHashes.length();
        ret.resize(int(serSize()));
        std::memcpy(ret.data(), txHash.data(), size_t(hlen));
        std::memcpy(ret.data() + hlen, reinterpret_cast<const char *>(&outN), sizeof(outN));
        return ret;
    }

    static TXO fromBytes(const QByteArray &ba) noexcept {
        TXO ret;
        if (ba.length() != int(serSize())) return ret;
        ret.txHash = QByteArray(ba.data(), HashLen);
        // we memcpy rather than reinterpret_cast in order to guard against unaligned access
        std::memcpy(reinterpret_cast<char *>(&ret.outN), ba.data()+HashLen, sizeof(ret.outN));
        return ret;
    }

    static constexpr size_t serSize() noexcept { return HashLen + sizeof(IONum); }
};


namespace std {
/// specialization of std::hash to be able to add struct TXO to any unordered_set or unordered_map as a key
template<> struct hash<TXO> {
    size_t operator()(const TXO &txo) const noexcept {
        const auto val1 = BTC::QByteArrayHashHasher{}(txo.txHash);
        const auto val2 = txo.outN;
        // We must copy the hash bytes and the ionum to a temporary buffer and hash that.
        // Previously, we put these two items in a struct but it didn't have a unique
        // objected repr and that led to bugs.  See Fulcrum issue #47 on GitHub.
        std::array<std::byte, sizeof(val1) + sizeof(val2)> buf;
        std::memcpy(buf.data()               , reinterpret_cast<const char *>(&val1), sizeof(val1));
        std::memcpy(buf.data() + sizeof(val1), reinterpret_cast<const char *>(&val2), sizeof(val2));
        // on 32-bit: below hashes the above 6-byte buffer using MurMur3
        // on 64-bit: below hashes the above 10-byte buffer using CityHash64
        return Util::hashForStd(buf);
    }
};
} // namespace std
