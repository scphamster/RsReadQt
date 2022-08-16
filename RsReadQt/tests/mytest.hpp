#include <QTest.h>
#include <QObject>

class MyTest : public QObject {
    Q_OBJECT

  private slots:
    void DoTest() { QCOMPARE(1, 1); }
    void DoBadTest() { QVERIFY(1 == 0); }
};