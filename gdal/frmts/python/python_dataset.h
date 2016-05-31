#include "gdal_priv.h"
#include <Python.h>
#include <string>

/* RAII class with unique ownership to decremet refcount on destruction */
struct RAII_PyObject
{
    RAII_PyObject( PyObject * obj )
        : _obj( obj )
    {}

    /* note: cpy and = transfer ownership */

    RAII_PyObject( RAII_PyObject &other )
        : _obj( other.release() )
    {}

    RAII_PyObject operator=( RAII_PyObject other )
    {
        std::swap( _obj, other._obj);
        return *this;
    }

    ~RAII_PyObject() { if ( _obj ) Py_DECREF( _obj ); }

    operator PyObject*() { return _obj; }

    operator bool() const { return _obj != NULL; }

    PyObject *release()
    {
        PyObject * ret = _obj;
        _obj = NULL;
        return ret;
    }

private:
    PyObject * _obj;
};

struct PythonDataset : public GDALDataset
{
    PythonDataset( PyObject * class_ );
    ~PythonDataset();
    
    static GDALDataset *Open( GDALOpenInfo * );
    static int Identify( GDALOpenInfo * );
    CPLErr GetGeoTransform( double * padfTransform );
    const char *GetProjectionRef();

private :
    RAII_PyObject _class;
    std::string _proj_ref;
};


struct PythonRasterBand : public GDALRasterBand
{
    PythonRasterBand( PyObject * class_, PythonDataset * ds, int one_based_index, GDALDataType data_type, size_t size_x, size_t size_y);
    CPLErr IReadBlock( int, int, void * );
private :
    RAII_PyObject _class;
};
