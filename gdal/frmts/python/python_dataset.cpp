#include "python_dataset.h"

#include <memory>
#include <sstream>
#include <iostream>

#pragma GCC diagnostic ignored "-Wwrite-strings"

PythonDataset::PythonDataset( PyObject * class_ )
    : _class( class_ )
{
}

PythonDataset::~PythonDataset()
{
}

GDALDataset *PythonDataset::Open( GDALOpenInfo * poOpenInfo )
{
    /* confirm the file licit python code */
    if (!Identify(poOpenInfo))
    {
        return NULL;
    }

    /* confirm the requested access is supported */
    if( poOpenInfo->eAccess == GA_Update )
    {
        CPLError( CE_Failure, CPLE_NotSupported,
                  "The Python driver does not support update access to "
                  "existing datasets.\n" );
        return NULL;
    }
    
    /* Check that the file name from GDALOpenInfo* is available */
    if( poOpenInfo->pszFilename == NULL )
    {
        return NULL;
    }

    /* init python interpreter, juste in case it's not been done before */
    /* @note we do not need to call Py_Finalize */
    Py_Initialize();

    /* load module and create an instance of GDALPluginDataset class */
    const std::string filename( poOpenInfo->pszFilename );
    RAII_PyObject pName( PyString_FromString( 
                filename.substr( 0, filename.length() - 3 ).c_str() ) );
    RAII_PyObject pModule( PyImport_Import( pName.get() ) );
    if ( PyErr_Occurred() || !pModule )
    {
        PyErr_Print();
        return NULL;
    }
    RAII_PyObject pDict( PyModule_GetDict( pModule.get() ) );
    RAII_PyObject pClass( PyDict_GetItemString(pDict.get(), "GDALPluginDataset") );
    if ( PyErr_Occurred() || !pClass )
    {
        PyErr_Print();
        return NULL;
    }
    RAII_PyObject pInstance( PyObject_CallObject(pClass.get(), NULL) );
    if ( PyErr_Occurred() || !pInstance )
    {
        PyErr_Print();
        return NULL;
    }
    std::auto_ptr< PythonDataset > ds( new PythonDataset( pInstance.release() ) );
    
    const size_t nband = PyInt_AsLong(
            RAII_PyObject( PyObject_CallMethod( ds->_class.get(), "GetRasterCount", NULL ) ).get() );
    for (size_t b=1; b<= nband; b++)
    {
        std::ostringstream bstr;
        bstr << b;
        RAII_PyObject band( PyObject_CallMethod( ds->_class.get(), "GetRasterBand", "(i)", 
                bstr.str().c_str() ) );
        if ( PyErr_Occurred() || !band )
        {
            PyErr_Print();
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Python datasett returned null raster band\n" );
            return NULL;
        }

        RAII_PyObject data_type( PyObject_CallMethod( band.get(), "GetRasterDataType", NULL ) );
        if ( PyErr_Occurred() || !PyInt_Check( data_type.get() ) )
        {
            PyErr_Print();
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Python raster band GetRasterDataType returned non int\n" );
            return NULL;
        }
        RAII_PyObject size_x( PyObject_CallMethod( band.get(), "GetXSize", NULL ) );
        if ( PyErr_Occurred() || !PyInt_Check( size_x.get() ) )
        {
            PyErr_Print();
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Python raster band GetXSize returned non int\n" );
            return NULL;
        }
        RAII_PyObject size_y( PyObject_CallMethod( band.get(), "GetYSize", NULL ) );
        if ( PyErr_Occurred() || !PyInt_Check( size_y.get() ) )
        {
            PyErr_Print();
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Python raster band GetYSize returned non int\n" );
            return NULL;
        }

        ds->SetBand( b, new PythonRasterBand( band.release(), ds.get(), b, 
                    GDALDataType( PyInt_AsLong( data_type.get() ) ), 
                    PyInt_AsLong( size_x.get() ), 
                    PyInt_AsLong( size_y.get() ) ) );
    }

    ds->_proj_ref = PyString_AsString( RAII_PyObject( 
        PyObject_CallMethod( ds->_class.get(), "GetProjectionRef", NULL ) ).get() );


    return ds.release();
}

int PythonDataset::Identify( GDALOpenInfo * /*poOpenInfo*/ )
{
    ///* init python interpreter, juste in case it's not been done before */
    ///* @note we do not need to call Py_Finalize */
    //Py_Initialize();

    ///** check the script is licit python code with the appropriate functions defined */
    //const std::string filename( poOpenInfo->pszFilename );
    //RAII_PyObject pName( PyString_FromString( 
    //            filename.substr( 0, filename.length() - 3 ).c_str() ) );
    //RAII_PyObject pModule( PyImport_Import( pName.get() ) );
    //if ( PyErr_Occurred() || !pModule )
    //{
    //    PyErr_Print();
    //    return FALSE;
    //}
    //RAII_PyObject pDict( PyModule_GetDict( pModule.get() ) );
    //RAII_PyObject pClass( PyDict_GetItemString( pDict.get(), "GDALPluginDataset") );
    //if ( PyErr_Occurred() || !pClass )
    //{
    //    PyErr_Print();
    //    return FALSE;
    //}
    return TRUE;
}

CPLErr PythonDataset::GetGeoTransform( double * padfTransform )
{
    RAII_PyObject transfo( PyObject_CallMethod( _class.get(), "GetGeoTransform", NULL ) );
    size_t tupleSize = PyTuple_Size( transfo.get() );
    if ( PyErr_Occurred() || tupleSize != 6 )
    {
        PyErr_Print();
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Python dataset GetGeoTransform returned a tuples with"
                  "ivalid size (i.e != 6)" );
        return CE_Failure;
    }
    for( size_t i=0; i < tupleSize; i++ ) {
        RAII_PyObject tupleItem( PyTuple_GetItem( transfo.get(), i ) );
        if ( PyErr_Occurred() || ( !PyFloat_Check( tupleItem.get() ) 
                                && !PyInt_Check( tupleItem.get() ) ) )
        {
            PyErr_Print();
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Python dataset GetGeoTransform returned a tuples with "
                      "non float values" );
            return CE_Failure;
        }
        padfTransform[i] = PyFloat_AsDouble( tupleItem.get() );
    }

    return CE_None;
}

const char *PythonDataset::GetProjectionRef()
{
    return _proj_ref.c_str();
}

PythonRasterBand::PythonRasterBand( PyObject * class_, PythonDataset * ds, int one_based_index, GDALDataType data_type, size_t size_x, size_t size_y )
    : _class( class_ )
{
    poDS = ds;
    nBand = one_based_index;
    eDataType = data_type;
    nRasterXSize = size_x;
    nRasterYSize = size_y;
    nBlockXSize = 1;
    nBlockYSize = 1;
    nBlocksPerRow = size_x;
    nBlocksPerColumn = size_y;
    
}

CPLErr PythonRasterBand::IRasterIO( 
        GDALRWFlag flag, int xOffset, int yOffset, int xSize, int ySize,
                              void *buffer, int bufXSize, int bufYSize, GDALDataType dataType,
                              GSpacing pixelSpacing, GSpacing lineSpacing, GDALRasterIOExtraArg* )
{
    PyObject_CallMethod( _class.get(), "RasterIO", "(iiiiiliiiii)",
            flag, xOffset, yOffset, xSize, ySize, 
            (long)buffer, bufXSize, bufYSize, dataType, 
            pixelSpacing, lineSpacing );
    if ( PyErr_Occurred() )
    {
        PyErr_Print();
        return CE_Failure;
    }
    return CE_None;
}

CPLErr PythonRasterBand::IReadBlock( int xBlock, int yBlock, void *block)
{
    std::cout << "PythonRasterBand::IReadBlock\n";
    const int xOff = xBlock * nBlockXSize;
    const int yOff = yBlock * nBlockYSize;
    return RasterIO(GF_Read, xOff, yOff, nBlockXSize, nBlockYSize, block, nBlockXSize, nBlockYSize, eDataType, 0, 0, NULL );
}


void GDALRegister_Python()
{
    GDALDriver  *poDriver;
    if (! GDAL_CHECK_VERSION("Python"))
        return;
    if( GDALGetDriverByName( "Python" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "python" );
        poDriver->SetMetadataItem( GDAL_DCAP_RASTER, "YES" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Python plugin (.py)" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#Python" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "py" );
        poDriver->SetMetadataItem( GDAL_DCAP_VIRTUALIO, "YES" );
        poDriver->pfnOpen = PythonDataset::Open;
        poDriver->pfnIdentify = PythonDataset::Identify;
        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}


#pragma GCC diagnostic pop
