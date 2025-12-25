import http.server
import socketserver

PORT = 9001


class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"backend": "9001", "message": "Hello from backend"}')


if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"Backend running on port {PORT}")
        httpd.serve_forever()


