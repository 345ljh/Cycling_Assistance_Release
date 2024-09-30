import socketserver
import http.server
from urllib.parse import parse_qs, urlparse
import json
import requests
import numpy as np
import geotran

PORT = 8000
key = "your_amap_key"  # 高德地图key

class SimpleHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        params = parse_qs(urlparse(self.path).query)
        origin_location = params['origin_location'][0]
        destination = params['destination'][0]

        no_city = False
        try:
            city = params['city'][0]
        except:
            no_city = True
        # origin_location = "113.966936,22.58839"
        # destination = "清华大学深圳研究生院"
        # city = "广东"

        # 转换起始坐标到G系
        ori_g = origin_location.split(",")
        ori_g = geotran.wgs84_to_gcj02(float(ori_g[0]), float(ori_g[1]))
        origin_location = str(np.around(ori_g[0], decimals=6)) + "," + str(np.around(ori_g[1], decimals=6))

        url_ = "https://restapi.amap.com/v3/geocode/geo?"
        if(no_city):
            url = url_ + "address=" + destination + "&output=JSON" + "&key=" + key
        else:
            url = url_ + "address=" + destination + "&output=JSON" + "&key=" + key + "&city=" + city

        response = requests.get(url)
        get_dest_loc_data = response.json()
        if(get_dest_loc_data['status'] == "1"):
            destination_location = get_dest_loc_data['geocodes'][0]['location']
        else:
            message = {
            "status" : 1  # 未搜索到地点
            }
            self.wfile.write(json.dumps(message).encode())
            return


        # 骑行路径规划
        url_ = "https://restapi.amap.com/v4/direction/bicycling?"
        url = url_ + "origin=" + origin_location + "&destination=" + destination_location + "&output=JSON" + "&key=" + key

        response = requests.get(url)
        get_route_data = response.json()
        if(get_route_data['errcode'] == 0):
            # 获取路径轨迹
            route = np.array([]).reshape(-1,2)
            for i in range(get_route_data['data']['paths'][0]['steps'].__len__()):
                s = get_route_data['data']['paths'][0]['steps'][i]['polyline']
                pairs = s.split(';')
                float_pairs = np.array([list(map(float, pair.split(','))) for pair in pairs][:-1])
                route = np.concatenate((route, float_pairs))

            # 将轨迹转换为W系
            for i in range(route.shape[0]):
                tmp = geotran.gcj02_to_wgs84(route[i,0], route[i,1])
                route[i,0] = tmp[0]
                route[i,1] = tmp[1]
            
            # 按距离合并点
            merge_dist = 20  # 单位m
            i = 0
            while(i < route.shape[0] - 1):
                dist = np.sqrt(((route[i,0] - route[i+1,0])*102.656) ** 2 + ((route[i,1] - route[i+1,1])*111.194) ** 2)*1000
                if(dist < merge_dist):
                    route = np.delete(route, i+1, axis=0)
                else:
                    i += 1

            # 按斜率合并点
            merge_angle = 15  # 单位°
            i = 0
            while(i < route.shape[0] - 2):
                v1 = route[i] - route[i+1]
                v2 = route[i+2] - route[i+1]
                
                if(np.linalg.norm(v1) == 0 or np.linalg.norm(v2) == 0):
                    angle = 0
                else:
                    angle = np.arccos(np.abs(np.dot(v1, v2) / np.linalg.norm(v1) / np.linalg.norm(v2))) * 180 / np.pi
                if(angle < merge_angle):
                    route = np.delete(route, i+1, axis=0)
                else:
                    i += 1

        else:
            message = {
            "status" : 2  # 无法规划路径
            }
            self.wfile.write(json.dumps(message).encode())
            return

        
        message = {
            "status": 0,
            "distance" : get_route_data['data']['paths'][0]['distance'],
            "duration" : get_route_data['data']['paths'][0]['duration'],
            "count": route.shape[0],
            "route": (route * 1e6).astype(int).tolist(),
            # 路径中心
            "center": [
                int((np.max(route[:,0]) + np.min(route[:,0])) * 5e5), 
                int((np.max(route[:,1]) + np.min(route[:,1])) * 5e5)
            ],
            # 以center作最小正方形, 边长的一半
            "halfside": int(np.max(np.array([np.ptp(route[:,0]), np.ptp(route[:,1])])) * 5e5)
        }
        self.wfile.write(json.dumps(message).encode())

with socketserver.TCPServer(('127.0.0.1', PORT), SimpleHTTPRequestHandler) as httpd:
    print('Python web server at port 8000 is running..')
    httpd.serve_forever()