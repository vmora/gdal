import gdal
import osr
from ctypes import cast, POINTER, c_float, pythonapi, addressof
import numpy

class GDALPluginRasterBand():
    def __init__(self):
        pass

    def GetRasterDataType(self):
        return gdal.GDT_Float32

    def GetXSize(self):
        return 512

    def GetYSize(self):
        return 1024

    def RasterIO(self, flag, x_offset, y_offset, x_size, y_size, data, data_x_size, data_y_size, data_type, pixel_space, line_space, extra_arg=None):
        a = numpy.ctypeslib.as_array(cast(data, POINTER(c_float)), (data_y_size, data_x_size))
        a[0, :] = 666
        a[1:, :] = 999

class GDALPluginDataset(object):
    def __init__(self):
        print "GDALPluginDataset.__init__"
        self.__rasterBand = GDALPluginRasterBand()

    def GetRasterCount(self):
 	"Fetch the number of raster bands on this dataset."
        return 1
 
    def GetRasterBand(self, one_based_index):
 	"Fetch a band object for a dataset."
        return self.__rasterBand

    def GetProjectionRef(self):
        "returns string containing a coordinate system definition"
        ref = osr.SpatialReference()
        ref.ImportFromEPSG(2154)
        return ref.ExportToWkt()

    def GetGeoTransform(self):
        # Xp = transfo[0] + P*transfo[1] + L*transfo[2];
        # Yp = transfo[3] + P*transfo[4] + L*transfo[5];
        return (0, 2, 0, 0, 0, 2)

if __name__ == '__main__':
    ds = GDALPluginDataset()
    print ds.GetProjectionRef()
