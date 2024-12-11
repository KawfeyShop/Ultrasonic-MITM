import json
from http.server import BaseHTTPRequestHandler, HTTPServer
import requests

class JSONHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        # Get the content length to read the POST data
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        
        # Parse JSON data
        try:
            data = json.loads(post_data)
            duration = data.get("duration", 0)
            result = duration * 2

            # Send the result to a remote server (IP on port 8000)
            remote_ip = "127.0.0.1"
            remote_url = f"http://{remote_ip}:8000/"
            payload = {"duration": result}
            response = requests.post(remote_url, json=payload)
            print(payload)
            # Send response back to the client
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({"status": "success", "result": result}).encode())

        except (json.JSONDecodeError, KeyError) as e:
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"Invalid JSON input.")

def run(server_class=HTTPServer, handler_class=JSONHandler):
    server_address = ('', 8080)
    httpd = server_class(server_address, handler_class)
    print("Starting server on port 8080...")
    httpd.serve_forever()

if __name__ == "__main__":
    run()
