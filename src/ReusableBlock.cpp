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
#ifdef ENABLE_TESTS
#include "App.h"
#include "ReusableBlock.h"
#include <QRandomGenerator>
#include <QVector>


namespace {
    void test()
    {
        QRandomGenerator rgen = QRandomGenerator();
        auto genRandomRuHash = [&rgen] {
            QVector<quint32> vec(32);
            rgen.fillRange(vec.data(), vec.size());
            RuHash h; // lazy but who really would be so pedantic to care
            for (size_t i=0; i<h.size(); ++i) // shh
                h.push_back(vec[i]);
            return h;
        };

        Log() << "Testing ReusableBlock add...";
        ReusableBlock ru;
        for (size_t i=1; i<100; ++i) {
            size_t times = rgen.bounded(4);
            for (size_t j=0; j<times; ++j)
                ru.add(genRandomRuHash(), i);
        }

        Log() << "Testing ReusableBlock serialize...";
        QByteArray serialized = ru.toBytes();

        Log() << "Testing ReusableBlock deserialize...";
        ReusableBlock ru2 = ReusableBlock::fromBytes(serialized);

        Log() << "Testing equality...";
        if (ru != ru2)
            throw Exception("ReusableBlock not equal");

        std::vector<QString> txStrs = {
            { "0100000001751ac11802cc3e4efc8aaaee87ca818482be9140dd6623f69db2c3af5c0b0ede01000000644161e02824b2ad3e24b1967ecd2e1bbcb53ca2b7c990802865b7f0f55e861849f7821daff5e78964346b1f7d16e5ce522d3354ca3cc1f6f4cba4ca0e57725af59e412102c986f0b3d6f4f8c765469fe0118cf973d676862f358e62a14104fae7d43f3032feffffff02e8030000000000001976a914ed707a5dbba9f4c117086c547fdc4e1e7a5ba40088accc550100000000001976a914e32151fdef9bc46cbb11514a84f54d8f51a905e588ac747a0a00" },
        };
        for (auto & txStr : txStrs) {
            bitcoin::CMutableTransaction tx;
            BTC::Deserialize(tx, Util::ParseHexFast(txStr.toUtf8()), 0, false);

            for (size_t n = 0; n < tx.vin.size(); ++n) {
                bitcoin::CTxIn input = tx.vin[n];
                RuHash ruHash = ReusableBlock::serializeInput(input);
                std::string ruHashPrefix = ReusableBlock::ruHashToPrefix(ruHash);
                QByteArray prefixHex = Util::ToHexFast(QByteArray(ruHashPrefix.c_str(), ruHashPrefix.length()));
                QByteArray prefixHexClean = prefixHex;
                // remove leading 0 from hex to match the format we use otherwise
                for (size_t j = 0; j < 4; ++j) {
                    prefixHexClean.remove(j, 1);
                }
                Log()
                    << "Txid: " << tx.GetId().ToString() << ":" << n
                    << " RuHash: " << Util::ToHexFast(ruHash)
                    << " Prefix: " << prefixHexClean;
            }
        }

    }

    static const auto test_  = App::registerTest("reusable", &test);
}
#endif
