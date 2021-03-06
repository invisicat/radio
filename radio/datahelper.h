#ifndef DATAHELPER_H
#define DATAHELPER_H

#include "stdafx.h"
#include "qwt_raster_data.h"
#include <complex>
#include "options.h"
#include "jobmanager.hpp"

class DataHelper : public QwtRasterData
{
private:
    QList<FilterResult*> m_data;
    QMutex *m_accessLock;
    T_REAL m_zTop;

public:    
    DataHelper();

    ~DataHelper();

    int columns() const;
    int rows() const;

    QMutex * mutex();

    void setData(FilterResult *data);
    void clear();
    virtual double value(double x, double y) const;
};


#endif // DATAHELPER_H
