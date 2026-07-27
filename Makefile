CXX      = g++
CXXFLAGS = -std=c++17 -Wall -O2
SERVER_SRCS = server.cpp Environment.cpp Layer.cpp DataLogger.cpp Actuator.cpp
ORIG_SRCS   = main.cpp   Environment.cpp Layer.cpp DataLogger.cpp Actuator.cpp

all: aegis_server

aegis_server: $(SERVER_SRCS)
	$(CXX) $(CXXFLAGS) $(SERVER_SRCS) -o aegis_server -lpthread
	@echo "[OK] ./aegis_server is ready — open aegis_dashboard.html in your browser"

aegis_original: $(ORIG_SRCS)
	$(CXX) $(CXXFLAGS) $(ORIG_SRCS) -o aegis_original

clean:
	rm -f aegis_server aegis_original fab_telemetry.csv

.PHONY: all aegis_server aegis_original clean
