#ifndef UNITTESTINFO_H
#define UNITTESTINFO_H

#include "unittestmanager.h"



class UnitTestInfo : public UnitTestBase
{
    Q_OBJECT
public:
    explicit UnitTestInfo(QObject *parent = 0);

public slots:
    virtual void start();

};

#endif // UNITTESTINFO_H
