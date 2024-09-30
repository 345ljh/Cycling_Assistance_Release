import math

x_pi = math.pi  * 3000.0 / 180.0

a = 6378245.0  # 长半轴
ee = 0.00669342162296594323  # 扁率
pi = math.pi

def wgs84_to_gcj02(lng, lat):
   """
  WGS84转GCJ02(火星坐标系)
  :param lng:WGS84坐标系的经度
  :param lat:WGS84坐标系的纬度
  :return:
  """
   if out_of_china(lng, lat):  # 判断是否在国内
       return lng, lat
   dlat = _transformlat(lng - 105.0, lat - 35.0)
   dlng = _transformlng(lng - 105.0, lat - 35.0)
   radlat = lat / 180.0 * pi
   magic = math.sin(radlat)
   magic = 1 - ee * magic * magic
   sqrtmagic = math.sqrt(magic)
   dlat = (dlat * 180.0) / ((a * (1 - ee)) / (magic * sqrtmagic) * pi)
   dlng = (dlng * 180.0) / (a / sqrtmagic * math.cos(radlat) * pi)
   mglat = lat + dlat
   mglng = lng + dlng
   return [mglng, mglat]


def gcj02_to_wgs84(lng, lat):
   """
  GCJ02(火星坐标系)转GPS84
  :param lng:火星坐标系的经度
  :param lat:火星坐标系纬度
  :return:
  """
   if out_of_china(lng, lat):
       return lng, lat
   dlat = _transformlat(lng - 105.0, lat - 35.0)
   dlng = _transformlng(lng - 105.0, lat - 35.0)
   radlat = lat / 180.0 * math.pi
   magic = math.sin(radlat)
   magic = 1 - ee * magic * magic
   sqrtmagic = math.sqrt(magic)
   dlat = (dlat * 180.0) / ((a * (1 - ee)) / (magic * sqrtmagic) * math.pi)
   dlng = (dlng * 180.0) / (a / sqrtmagic * math.cos(radlat) * math.pi)
   mglat = lat + dlat
   mglng = lng + dlng
   return [lng * 2 - mglng, lat * 2 - mglat]

def _transformlat(lng, lat):
   ret = -100.0 + 2.0 * lng + 3.0 * lat + 0.2 * lat * lat + \
         0.1 * lng * lat + 0.2 * math.sqrt(math.fabs(lng))
   sin_lng_pi = math.sin(6.0 * lng * math.pi)
   sin_2_lng_pi = math.sin(2.0 * lng * math.pi)
   sin_lat_pi = math.sin(lat * math.pi)
   sin_lat_3_pi = math.sin(lat / 3.0 * math.pi)
   sin_lat_12_pi = math.sin(lat / 12.0 * math.pi)
   sin_lat_30_pi = math.sin(lat * math.pi / 30.0)
   ret += (20.0 * sin_lng_pi + 20.0 * sin_2_lng_pi) * 2.0 / 3.0
   ret += (20.0 * sin_lat_pi + 40.0 * sin_lat_3_pi) * 2.0 / 3.0
   ret += (160.0 * sin_lat_12_pi + 320 * sin_lat_30_pi) * 2.0 / 3.0
   return ret

def _transformlng(lng, lat):
   ret = 300.0 + lng + 2.0 * lat + 0.1 * lng * lng + \
         0.1 * lng * lat + 0.1 * math.sqrt(math.fabs(lng))
   ret += (20.0 * math.sin(6.0 * lng * math.pi) + 20.0 *
           math.sin(2.0 * lng * math.pi)) * 2.0 / 3.0
   ret += (20.0 * math.sin(lng * math.pi) + 40.0 *
           math.sin(lng / 3.0 * math.pi)) * 2.0 / 3.0
   ret += (150.0 * math.sin(lng / 12.0 * math.pi) + 300.0 *
           math.sin(lng / 30.0 * math.pi)) * 2.0 / 3.0
   return ret


def out_of_china(lng, lat):
   """
  判断是否在国内，不在国内不做偏移
  :param lng:
  :param lat:
  :return:
  """
   return not (lng > 73.66 and lng < 135.05 and lat > 3.86 and lat < 53.55)
