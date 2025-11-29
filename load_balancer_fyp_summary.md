# FYP Progress Summary: HTTP Server + Load Balancer (C Language)

## **Project Title**
High-Performance HTTP Server With Custom Load Balancer Using C Language

---

## **1. Objective (End Goal)**
To build a production-style load balancer in the C programming language that:
- Accepts client HTTP requests.
- Parses raw HTTP messages manually.
- Forwards requests to backend servers using Round Robin.
- Runs behind a proxy and works in real-world backend applications.
- Supports concurrency (multi-threading).
- Maintains backend health-checking.
- Provides a minimal, modular, extensible design.

---

## **2. What We Have Completed So Far**
### **2.1 Basic HTTP Server (Step 1)**
✔ Created a low-level TCP socket server in C.  
✔ Implemented recv() buffering and line-based parsing.  
✔ Designed a `Request` struct:
- Method (string)
- Path (string)
- Version (string)
- Header map (2D array)
- Body (string)

✔ Implemented full HTTP parsing logic:
- Parse request line.
- Parse headers.
- Handle body.

✔ Designed modular folder structure:
```
/include
    http.h
    server.h
    backend.h
/src
    http.c
    server.c
    backend.c
    main.c
```

✔ Server now prints parsed method, path, and version.  
✔ Server responds with a JSON response.

---

### **2.2 Backend Pool + Round Robin (Step 2.1)**
✔ Created backend pool system:
- `Backend` struct
- `BackendPool` struct

✔ Implemented functions:
- `init_backends()` → initializes 3 backend servers.
- `get_next_backend()` → returns next backend using Round Robin.

✔ Integrated Round Robin selection in `main.c`.

Server output example:
```
Initialized 3 backend servers for load balancing.
Selected backend → 127.0.0.1:9001
```

---

## **3. Remaining Steps (Roadmap)**
### **Step 2.2 – Forwarding Requests to Backend Servers**
- Make the load balancer act as a *reverse proxy*.
- Connect to selected backend using `connect()`.
- Forward the raw HTTP request to backend using `send()`.
- recv() backend response.
- Send backend response back to client.

### **Step 3 – Multi-threading (Concurrency)**
- Convert single-threaded server into multi-threaded.
- Each client request handled in a separate thread.
- Use pthreads:
```
pthread_create()
pthread_detach()
```

### **Step 4 – Health Checks**
- Periodically ping backend servers.
- Mark unhealthy backends as down.
- Skip them in load balancing.

### **Step 5 – Running Behind Proxy (Production Mode)**
- Support X-Forwarded-For header.
- Support real-world proxy forwarding.

### **Step 6 – Logging + Error Handling**
- Standardized logs.
- Request logs.
- Error logs.

### **Final Deliverable**
A C-based modular load balancer that:
- Accepts HTTP traffic.
- Parses & forwards requests.
- Balances load across multiple backend servers.
- Runs concurrently with threads.
- Supports proxy environments.
- Has clean modular code.

---

## **4. Status Summary**
| Component | Status |
|----------|--------|
| Basic TCP Server | **Completed** |
| HTTP Parsing | **Completed** |
| Backend Pool | **Completed** |
| Round Robin | **Completed** |
| Reverse Proxy Logic | **Pending** |
| Multi-threading | **Pending** |
| Health Checking | **Pending** |
| Proxy-friendly Features | **Pending** |
| Logging | **Pending** |

---

## **5. What We Are Working on Next**
✔ Now starting **Step 2.2: Build reverse proxy logic → Forward request to backend**.

This will convert your server into a real load balancer.

---
**Prepared For:**  
FYP Supervisor  
**Student:** Umair

