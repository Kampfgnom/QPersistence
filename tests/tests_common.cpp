#include "tests_common.h"

QVariant NULLKEY() {
#ifdef QP_FOR_MYSQL
    return QVariant(QVariant().toInt());
#elif defined QP_FOR_SQLITE
    return QVariant(QVariant().toString());
#endif
}


bool waitForSignal(QObject *sender, const char *signal)
{
    QEventLoop loop;
    QObject::connect(sender, signal, &loop, SLOT(quit()));

    // Execute the event loop here, now we will wait here until signal signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();
    return true;
}
