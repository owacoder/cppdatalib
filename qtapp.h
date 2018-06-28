#ifndef QTAPP_H
#define QTAPP_H

#include <QCoreApplication>
#include <QTimer>

class QtApp : public QObject {
    Q_OBJECT

    void (*function)();

public:
    QtApp(void(*function)(), QObject *parent = 0) : QObject(parent), function(function) {}

public slots:
    void go() {function(); emit done();}

signals:
    void done();
};

inline int qtapp_main(int argc, char **argv, void (*function)())
{
    QCoreApplication a(argc, argv);

    QtApp *f = new QtApp(function, &a);

    QTimer::singleShot(0, f, SLOT(go()));

    QObject::connect(f, SIGNAL(done()), qApp, SLOT(quit()));

    return a.exec();
}

#endif // QTAPP_H
