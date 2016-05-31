import gdal
import sys
import numpy

#ds = gdal.Open('/home/vmo/data_lyon/MNT2009_Altitude_10m_CC46.tif')
ds = gdal.Open('dataset.py')
print ds.GetMetadata()

print ds.GetGeoTransform()
print ds.GetProjectionRef()
band = ds.GetRasterBand(1)
print band


print "[ NO DATA VALUE ] = ", band.GetNoDataValue()
print "[ MIN ] = ", band.GetMinimum()
print "[ MAX ] = ", band.GetMaximum()
print "[ SCALE ] = ", band.GetScale()
print "[ UNIT TYPE ] = ", band.GetUnitType()
ctable = band.GetColorTable()

if ctable is None:
    print 'No ColorTable found'

print band.ReadAsArray(0, 0, 2, 3)
res = band.ReadAsArray(5, 6, 2, 3).astype(numpy.float)
print res
