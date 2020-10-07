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
    }

    static const auto test_  = App::registerTest("reusable", &test);
}
#endif
