import http.server
import socketserver
import json

PORT = 9001


class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        # Collect all headers received
        headers_received = {}
        for header_name, header_value in self.headers.items():
            headers_received[header_name] = header_value
        
        # Print headers to console for debugging
        print(f"\n=== Headers received on port {PORT} ===")
        for key, value in headers_received.items():
            print(f"  {key}: {value}")
        print("=" * 40)
        
        # Create response with headers info
        response_data = {
            "backend": "9001",
            "message": "Hello from backend",
            "headers_received": headers_received
        }
        
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(response_data, indent=2).encode())


if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"Backend running on port {PORT}")
        httpd.serve_forever()


