import socketserver
import http.server
from urllib.parse import parse_qs, urlparse
import json
import requests
import ch_encode

PORT = 8000
key = "your_amap_key"  # 高德地图key

class SimpleHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        params = parse_qs(urlparse(self.path).query)
        searchname = params["searchname"]
        no_city = False
        try:
            city = params['city'][0]
        except:
            no_city = True

        url_ = "https://restapi.amap.com/v3/place/text?"
        if(no_city):
            url = url_ + "&key=" + key + "&output=JSON&offset=5&page=1&extensions=base" + "&keywords=" + searchname
        else:
            url = url_ + "&key=" + key + "&output=JSON&offset=5&page=1&extensions=base" + "&keywords=" + searchname + "&city=" + city + "&citylimit=true"

        response_json = requests.get(url).json()
        cnt = int(response_json['count'])
        if(cnt > 0):
            if cnt > 6:
                cnt = 6
            length = []
            encode_name = []
            name = []
            for poi in response_json['pois']:
                # length.append(poi['name'].__len__())
                # encode_name.append(ch_encode.str_encode(poi['name']))
                name.append({
                    "length": poi['name'].__len__(),
                    "encoded": ch_encode.str_encode(poi['name'])
                })

            message = {
                "status": 0,
                "name": name
            }
            self.wfile.write(json.dumps(message).encode())
            return

        else:
            message = {
                "status": 1,
            }
        self.wfile.write(json.dumps(message).encode())
        return

with socketserver.TCPServer(('127.0.0.1', PORT), SimpleHTTPRequestHandler) as httpd:
    print('Python web server at port 8000 is running..')
    httpd.serve_forever()