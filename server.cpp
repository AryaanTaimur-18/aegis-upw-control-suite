// AEGIS-UPW HTTP Backend Server v2.1
// Compile: g++ -std=c++17 server.cpp Environment.cpp Layer.cpp DataLogger.cpp Actuator.cpp -o aegis_server -lpthread

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

#include "matrix.h"
#include "Layer.h"
#include "neuralnetwork.h"
#include "Environment.h"
#include "Actuator.h"
#include "DataLogger.h"

using namespace std;

// ─── Global mutex (not embedded in struct to avoid copy issues) ───────────────
mutex g_mtx;

// ─── Simulation state (plain struct, no mutex inside) ────────────────────────
struct SimState {
    double resistivity  = 18.2;
    double toc          = 0.05;
    double safetyScore  = 0.85;
    double truePurity   = 1.0;
    bool   fabValveOpen = true;
    bool   diverterOpen = false;
    bool   leakActive   = false;
    bool   clogActive   = false;
    int    tick         = 0;
    double gallons      = 0;
    double chips        = 0;
    double dollars      = 0;
    // Slider overrides from dashboard
    double sliderTemp   = 25.0;
    double sliderPres   = 45.0;
    double sliderCont   = 0.0;
    double sliderToc    = 0.0;
    // NN activations
    double nn_in[2]     = {1.0, 0.0};
    double nn_hidden[4] = {0.5, 0.5, 0.5, 0.5};
    double nn_out[1]    = {0.85};
    // Event log
    vector<string> log;
} S;

// Backend objects
Environment*     g_env        = nullptr;
NeuralNetwork*   g_brain      = nullptr;
MainProcessValve* g_fabValve  = nullptr;
DiverterValve*   g_diverter   = nullptr;
DataLogger*      g_logger     = nullptr;

// ─── Logging (called with g_mtx already held) ─────────────────────────────────
void addLog(const string& msg, const string& lvl = "info") {
    auto t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    char ts[10];
    strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&t));
    S.log.insert(S.log.begin(), string(ts) + "|" + lvl + "|" + msg);
    if (S.log.size() > 50) S.log.pop_back();
}

// ─── Simulation Loop ──────────────────────────────────────────────────────────
void simulationLoop() {
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(600));
        lock_guard<mutex> lk(g_mtx);

        g_env->tick();
        S.tick++;

        double r = g_env->getResistivitySignal();
        double t = g_env->getTOCSignal();

        // Apply slider overrides
        double tempPen = max(0.0, (S.sliderTemp - 60.0) / 100.0) * 4.0;
        double contPen = (S.sliderCont / 100.0) * 12.0;
        double presPen = max(0.0, (S.sliderPres - 120.0) / 200.0) * 2.0;
        r = max(0.1, r - tempPen - contPen - presPen);
        t = t + S.sliderToc;

        S.resistivity = r;
        S.toc         = t;
        S.truePurity  = max(0.0, min(1.0, r / 18.2));

        // Neural network prediction
        Matrix input(1, 2);
        input.set(0, 0, max(0.0, min(1.0, r / 18.2)));
        input.set(0, 1, max(0.0, min(1.0, t / 50.0)));

        Matrix pred  = g_brain->predict(input);
        double score = pred.get(0, 0);
        S.safetyScore = score;

        // Store NN telemetry
        auto tel = g_brain->getNetworkTelemetry();
        S.nn_in[0] = r / 18.2;
        S.nn_in[1] = t / 50.0;
        if (tel.size() >= 1)
            for (int i = 0; i < (int)min(tel[0].size(), (size_t)4); i++)
                S.nn_hidden[i] = tel[0][i];
        if (tel.size() >= 2)
            S.nn_out[0] = tel[1][0];

        // Trigger actuators
        bool prevFab = g_fabValve->getState();
        bool prevDiv = g_diverter->getState();
        g_fabValve->trigger(score);
        g_diverter->trigger(score);
        S.fabValveOpen = g_fabValve->getState();
        S.diverterOpen = g_diverter->getState();

        // Economic counters
        if (S.fabValveOpen) {
            S.gallons += 0.85;
            S.chips   += 0.05;
            S.dollars += 4.75;
        }

        g_logger->logDecision(r, score, S.fabValveOpen ? "FLOWING" : "BLOCKED");

        // Event log
        if (!prevFab && S.fabValveOpen)
            addLog("Main valve OPENED — flowing to Fab", "ok");
        if (prevFab && !S.fabValveOpen)
            addLog("Main valve CLOSED — safety stop", "alarm");
        if (!prevDiv && S.diverterOpen)
            addLog("Diverter ACTIVE — recycling water", "warn");
        if (S.tick % 10 == 0) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Tick %d | R=%.2f | TOC=%.2f | Score=%.3f",
                     S.tick, r, t, score);
            addLog(buf, "info");
        }
    }
}

// ─── JSON helpers ─────────────────────────────────────────────────────────────
static string esc(const string& s) {
    string o;
    for (char c : s) {
        if (c == '"')  o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else o += c;
    }
    return o;
}

string buildJson() {
    // Snapshot under lock
    SimState snap;
    {
        lock_guard<mutex> lk(g_mtx);
        snap = S;
    }

    ostringstream j;
    j.precision(4);
    j << fixed;
    j << "{"
      << "\"tick\":"         << snap.tick         << ","
      << "\"resistivity\":"  << snap.resistivity   << ","
      << "\"toc\":"          << snap.toc           << ","
      << "\"safetyScore\":"  << snap.safetyScore   << ","
      << "\"purity\":"       << snap.truePurity    << ","
      << "\"fabValve\":"     << (snap.fabValveOpen ? "true":"false") << ","
      << "\"diverter\":"     << (snap.diverterOpen ? "true":"false") << ","
      << "\"leakActive\":"   << (snap.leakActive   ? "true":"false") << ","
      << "\"clogActive\":"   << (snap.clogActive   ? "true":"false") << ","
      << "\"gallons\":"      << snap.gallons        << ","
      << "\"chips\":"        << snap.chips          << ","
      << "\"dollars\":"      << snap.dollars        << ","
      << "\"sliderTemp\":"   << snap.sliderTemp     << ","
      << "\"sliderPres\":"   << snap.sliderPres     << ","
      << "\"sliderCont\":"   << snap.sliderCont     << ","
      << "\"sliderToc\":"    << snap.sliderToc      << ","
      << "\"nn\":{"
        << "\"input\":["     << snap.nn_in[0] << "," << snap.nn_in[1] << "],"
        << "\"hidden\":["    << snap.nn_hidden[0] << "," << snap.nn_hidden[1] << ","
                             << snap.nn_hidden[2] << "," << snap.nn_hidden[3] << "],"
        << "\"output\":["    << snap.nn_out[0] << "]"
      << "},\"log\":[";

    size_t logN = min(snap.log.size(), (size_t)20);
    for (size_t i = 0; i < logN; i++) {
        if (i) j << ",";
        j << "\"" << esc(snap.log[i]) << "\"";
    }
    j << "]}";
    return j.str();
}

// ─── HTTP ─────────────────────────────────────────────────────────────────────
string httpOK(const string& body, const string& ct = "application/json") {
    ostringstream r;
    r << "HTTP/1.1 200 OK\r\n"
      << "Content-Type: " << ct << "\r\n"
      << "Access-Control-Allow-Origin: *\r\n"
      << "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
      << "Access-Control-Allow-Headers: Content-Type\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n\r\n"
      << body;
    return r.str();
}

// Parse a single double param from query string
double getParam(const string& qs, const string& key, double def = -1.0) {
    auto pos = qs.find(key + "=");
    if (pos == string::npos) return def;
    pos += key.size() + 1;
    auto end = qs.find('&', pos);
    string val = (end == string::npos) ? qs.substr(pos) : qs.substr(pos, end - pos);
    try { return stod(val); } catch (...) { return def; }
}

void handleClient(int fd) {
    char buf[4096] = {};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) { close(fd); return; }

    string req(buf, n);
    string method, path;
    {
        istringstream ss(req);
        ss >> method >> path;
    }

    if (method == "OPTIONS") {
        auto r = httpOK("");
        send(fd, r.c_str(), r.size(), 0);
        close(fd); return;
    }

    string resp;
    string basePath = path;
    string queryStr = "";
    auto qpos = path.find('?');
    if (qpos != string::npos) {
        basePath  = path.substr(0, qpos);
        queryStr  = path.substr(qpos + 1);
    }

    if (basePath == "/state") {
        resp = httpOK(buildJson());

    } else if (basePath == "/inject_leak") {
        lock_guard<mutex> lk(g_mtx);
        g_env->injectLeak();
        S.leakActive = true;
        addLog("!!! LEAK INJECTED — Contaminant in pipes !!!", "alarm");
        resp = httpOK("{\"ok\":true}");

    } else if (basePath == "/inject_clog") {
        lock_guard<mutex> lk(g_mtx);
        S.clogActive = true;
        for (int i = 0; i < 8; i++) g_env->tick();
        addLog("FILTER CLOGGED — pressure spike detected", "warn");
        resp = httpOK("{\"ok\":true}");

    } else if (basePath == "/fix_system") {
        lock_guard<mutex> lk(g_mtx);
        g_env->fixSystem();
        S.leakActive = false;
        S.clogActive = false;
        S.sliderCont = 0; S.sliderToc = 0;
        S.sliderTemp = 25; S.sliderPres = 45;
        addLog("System flushed. Filters replaced. All nominal.", "ok");
        resp = httpOK("{\"ok\":true}");

    } else if (basePath == "/set_params") {
        lock_guard<mutex> lk(g_mtx);
        double v;
        v = getParam(queryStr, "temp"); if (v >= 0) S.sliderTemp = v;
        v = getParam(queryStr, "pres"); if (v >= 0) S.sliderPres = v;
        v = getParam(queryStr, "cont"); if (v >= 0) S.sliderCont = v;
        v = getParam(queryStr, "toc");  if (v >= 0) S.sliderToc  = v;
        char logbuf[128];
        snprintf(logbuf, sizeof(logbuf),
            "Params set: Temp=%.1f°C Pres=%.0fPSI Cont=%.1f%% TOC=%.1fppb",
            S.sliderTemp, S.sliderPres, S.sliderCont, S.sliderToc);
        addLog(logbuf, "info");
        resp = httpOK("{\"ok\":true}");

    } else if (basePath == "/stress_test") {
        thread([] {
            auto pause = [](int ms){ this_thread::sleep_for(chrono::milliseconds(ms)); };
            pause(0);   { lock_guard<mutex> lk(g_mtx); addLog("STRESS: high-temp phase","warn"); S.sliderTemp=85; }
            pause(2000);{ lock_guard<mutex> lk(g_mtx); g_env->injectLeak(); S.leakActive=true; addLog("STRESS: leak injected","alarm"); }
            pause(2000);{ lock_guard<mutex> lk(g_mtx); S.sliderCont=70; S.sliderToc=30; addLog("STRESS: max contamination","alarm"); }
            pause(2000);{ lock_guard<mutex> lk(g_mtx); S.sliderPres=170; addLog("STRESS: pressure surge","warn"); }
            pause(2000);{ lock_guard<mutex> lk(g_mtx); g_env->fixSystem(); S.leakActive=false; S.clogActive=false;
                          S.sliderTemp=25; S.sliderPres=45; S.sliderCont=0; S.sliderToc=0;
                          addLog("STRESS: system recovered","ok"); }
        }).detach();
        resp = httpOK("{\"ok\":true}");

    } else {
        resp = httpOK("{\"error\":\"unknown\"}");
    }

    send(fd, resp.c_str(), resp.size(), 0);
    close(fd);
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main() {
    signal(SIGPIPE, SIG_IGN);

    g_env     = new Environment();
    // Initialize NN and ensure clean water (R~18.2, TOC~0) scores above 0.7
    // Re-seed until we get a network that scores reasonably on clean input
    g_brain = nullptr;
    for (int attempt = 0; attempt < 200; attempt++) {
        delete g_brain;
        g_brain = new NeuralNetwork();
        g_brain->addLayer(2, 4);
        g_brain->addLayer(4, 1);
        // Test on clean input
        Matrix probe(1, 2);
        probe.set(0, 0, 1.0);   // perfect resistivity
        probe.set(0, 1, 0.001); // near-zero TOC
        Matrix out = g_brain->predict(probe);
        double score = out.get(0, 0);
        // Also test on dirty input
        probe.set(0, 0, 0.3);   // low resistivity
        probe.set(0, 1, 0.7);   // high TOC
        Matrix out2 = g_brain->predict(probe);
        double dirty = out2.get(0, 0);
        if (score > 0.70 && dirty < 0.70) break; // good separation found
    }
    g_fabValve  = new MainProcessValve();
    g_diverter  = new DiverterValve();
    g_logger    = new DataLogger("fab_telemetry.csv");

    {
        lock_guard<mutex> lk(g_mtx);
        addLog("AEGIS-UPW v2.1 initialized — 2->4->1 neural topology", "ok");
        addLog("HTTP API online at localhost:8081", "info");
    }

    thread(simulationLoop).detach();

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR,  &opt, sizeof(opt));
    setsockopt(srv, SOL_SOCKET, SO_REUSEPORT,  &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(8081);

    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Bind failed on port 8081 — is another instance running?\n";
        return 1;
    }
    listen(srv, 20);
    cout << "[AEGIS-SERVER] Listening on http://localhost:8081\n";

    while (true) {
        int client = accept(srv, nullptr, nullptr);
        if (client >= 0)
            thread(handleClient, client).detach();
    }
    return 0;
}
