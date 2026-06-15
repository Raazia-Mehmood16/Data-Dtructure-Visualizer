#include <emscripten/emscripten.h>
#include <vector>
#include <queue>
#include <stack>
#include <unordered_map>
#include <climits>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <string>

using namespace std;

// ============================================================
//  BINARY HEAP
// ============================================================
static vector<int> heap;
static bool isMinHeap = true;

static string heapToJSON() {
    string s = "[";
    for (int i = 0; i < (int)heap.size(); i++) {
        if (i) s += ",";
        s += to_string(heap[i]);
    }
    s += "]";
    return s;
}

static void heapifyUp(int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        bool shouldSwap = isMinHeap ? (heap[idx] < heap[parent]) : (heap[idx] > heap[parent]);
        if (shouldSwap) {
            swap(heap[idx], heap[parent]);
            idx = parent;
        } else break;
    }
}

static void heapifyDown(int idx, int n) {
    while (true) {
        int target = idx;
        int l = 2*idx+1, r = 2*idx+2;
        if (l < n) {
            bool better = isMinHeap ? (heap[l] < heap[target]) : (heap[l] > heap[target]);
            if (better) target = l;
        }
        if (r < n) {
            bool better = isMinHeap ? (heap[r] < heap[target]) : (heap[r] > heap[target]);
            if (better) target = r;
        }
        if (target != idx) { swap(heap[idx], heap[target]); idx = target; }
        else break;
    }
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
const char* heap_setType(int minHeap) {
    isMinHeap = (minHeap == 1);
    heap.clear();
    static string r;
    r = string("{\"type\":\"") + (isMinHeap ? "min" : "max") + "\",\"heap\":[]}";
    return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* heap_insert(int val) {
    heap.push_back(val);
    heapifyUp((int)heap.size()-1);
    static string r;
    r = "{\"type\":\"" + string(isMinHeap?"min":"max") + "\",\"heap\":" + heapToJSON() + "}";
    return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* heap_extractRoot() {
    static string r;
    if (heap.empty()) { r = "{\"error\":\"Heap is empty\"}"; return r.c_str(); }
    int root = heap[0];
    heap[0] = heap.back(); heap.pop_back();
    if (!heap.empty()) heapifyDown(0, (int)heap.size());
    r = "{\"type\":\"" + string(isMinHeap?"min":"max") + "\",\"extracted\":" + to_string(root) + ",\"heap\":" + heapToJSON() + "}";
    return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* heap_getState() {
    static string r;
    r = "{\"type\":\"" + string(isMinHeap?"min":"max") + "\",\"heap\":" + heapToJSON() + "}";
    return r.c_str();
}

// ============================================================
//  AVL TREE
// ============================================================
struct AVLNode {
    int key, height, bf;
    AVLNode *left, *right;
    AVLNode(int k): key(k), height(1), bf(0), left(nullptr), right(nullptr){}
};

static AVLNode* avlRoot = nullptr;
static string avlLog;

static int avlH(AVLNode* n) { return n ? n->height : 0; }
static void avlUpdate(AVLNode* n) {
    if (!n) return;
    n->height = 1 + max(avlH(n->left), avlH(n->right));
    n->bf = avlH(n->left) - avlH(n->right);
}
static AVLNode* rotateRight(AVLNode* y) {
    avlLog += "LL Rotation at " + to_string(y->key) + "; ";
    AVLNode* x = y->left; AVLNode* T = x->right;
    x->right = y; y->left = T;
    avlUpdate(y); avlUpdate(x); return x;
}
static AVLNode* rotateLeft(AVLNode* x) {
    avlLog += "RR Rotation at " + to_string(x->key) + "; ";
    AVLNode* y = x->right; AVLNode* T = y->left;
    y->left = x; x->right = T;
    avlUpdate(x); avlUpdate(y); return y;
}
static AVLNode* avlBalance(AVLNode* n) {
    avlUpdate(n);
    if (n->bf > 1) {
        if (n->left->bf < 0) { avlLog += "LR Case; "; n->left = rotateLeft(n->left); }
        return rotateRight(n);
    }
    if (n->bf < -1) {
        if (n->right->bf > 0) { avlLog += "RL Case; "; n->right = rotateRight(n->right); }
        return rotateLeft(n);
    }
    return n;
}
static AVLNode* avlInsert(AVLNode* n, int key) {
    if (!n) return new AVLNode(key);
    if (key < n->key) n->left = avlInsert(n->left, key);
    else if (key > n->key) n->right = avlInsert(n->right, key);
    else return n;
    return avlBalance(n);
}
static AVLNode* avlMinNode(AVLNode* n) { while (n->left) n = n->left; return n; }
static AVLNode* avlDelete(AVLNode* n, int key) {
    if (!n) return n;
    if (key < n->key) n->left = avlDelete(n->left, key);
    else if (key > n->key) n->right = avlDelete(n->right, key);
    else {
        if (!n->left || !n->right) {
            AVLNode* tmp = n->left ? n->left : n->right;
            delete n; return tmp;
        }
        AVLNode* mn = avlMinNode(n->right);
        n->key = mn->key;
        n->right = avlDelete(n->right, mn->key);
    }
    return avlBalance(n);
}
static string avlNodeJSON(AVLNode* n) {
    if (!n) return "null";
    return "{\"key\":" + to_string(n->key) +
           ",\"height\":" + to_string(n->height) +
           ",\"bf\":" + to_string(n->bf) +
           ",\"left\":" + avlNodeJSON(n->left) +
           ",\"right\":" + avlNodeJSON(n->right) + "}";
}

EMSCRIPTEN_KEEPALIVE
const char* avl_insert(int key) {
    avlLog = "Inserting " + to_string(key) + "; ";
    avlRoot = avlInsert(avlRoot, key);
    static string r;
    r = "{\"log\":\"" + avlLog + "\",\"tree\":" + avlNodeJSON(avlRoot) + "}";
    return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* avl_delete(int key) {
    avlLog = "Deleting " + to_string(key) + "; ";
    avlRoot = avlDelete(avlRoot, key);
    static string r;
    r = "{\"log\":\"" + avlLog + "\",\"tree\":" + avlNodeJSON(avlRoot) + "}";
    return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* avl_getState() {
    static string r;
    r = "{\"log\":\"\",\"tree\":" + avlNodeJSON(avlRoot) + "}";
    return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
void avl_clear() { avlRoot = nullptr; }

// ============================================================
//  GRAPH (Adjacency List, Weighted)
// ============================================================
#define MAXN 20
static int gAdj[MAXN][MAXN]; // weight, 0=no edge
static int gN = 0;
static int nodeLabels[MAXN];

static string graphJSON() {
    string s = "{\"nodes\":[";
    for (int i = 0; i < gN; i++) {
        if (i) s += ",";
        s += to_string(nodeLabels[i]);
    }
    s += "],\"edges\":[";
    bool first = true;
    for (int i = 0; i < gN; i++)
        for (int j = i; j < gN; j++)
            if (gAdj[i][j]) {
                if (!first) s += ",";
                s += "{\"u\":" + to_string(i) + ",\"v\":" + to_string(j) + ",\"w\":" + to_string(gAdj[i][j]) + "}";
                first = false;
            }
    s += "]}";
    return s;
}

EMSCRIPTEN_KEEPALIVE
const char* graph_init(int n) {
    gN = (n > MAXN ? MAXN : n);
    memset(gAdj, 0, sizeof(gAdj));
    for (int i = 0; i < gN; i++) nodeLabels[i] = i;
    static string r; r = graphJSON(); return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* graph_addEdge(int u, int v, int w) {
    static string r;
    if (u < 0 || u >= gN || v < 0 || v >= gN) { r = "{\"error\":\"Invalid node\"}"; return r.c_str(); }
    gAdj[u][v] = w; gAdj[v][u] = w;
    r = graphJSON(); return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* graph_removeEdge(int u, int v) {
    gAdj[u][v] = 0; gAdj[v][u] = 0;
    static string r; r = graphJSON(); return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* graph_bfs(int start) {
    if (start < 0 || start >= gN) { static string e = "{\"error\":\"Invalid\"}"; return e.c_str(); }
    vector<bool> visited(gN, false);
    vector<int> order, queueStates;
    queue<int> q;
    q.push(start); visited[start] = true;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        order.push_back(u);
        for (int v = 0; v < gN; v++) {
            if (gAdj[u][v] && !visited[v]) {
                visited[v] = true;
                q.push(v);
                queueStates.push_back(v);
            }
        }
    }
    string s = "{\"algorithm\":\"BFS\",\"order\":[";
    for (int i = 0; i < (int)order.size(); i++) { if(i)s+=","; s+=to_string(order[i]); }
    s += "]}";
    static string r; r = s; return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* graph_dfs(int start) {
    if (start < 0 || start >= gN) { static string e = "{\"error\":\"Invalid\"}"; return e.c_str(); }
    vector<bool> visited(gN, false);
    vector<int> order;
    stack<int> st;
    st.push(start);
    while (!st.empty()) {
        int u = st.top(); st.pop();
        if (visited[u]) continue;
        visited[u] = true;
        order.push_back(u);
        for (int v = gN-1; v >= 0; v--)
            if (gAdj[u][v] && !visited[v]) st.push(v);
    }
    string s = "{\"algorithm\":\"DFS\",\"order\":[";
    for (int i = 0; i < (int)order.size(); i++) { if(i)s+=","; s+=to_string(order[i]); }
    s += "]}";
    static string r; r = s; return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* graph_dijkstra(int start) {
    vector<int> dist(gN, INT_MAX);
    vector<int> prev(gN, -1);
    vector<bool> visited(gN, false);
    dist[start] = 0;
    priority_queue<pair<int,int>, vector<pair<int,int>>, greater<>> pq;
    pq.push({0, start});
    string steps = "";
    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (visited[u]) continue;
        visited[u] = true;
        steps += "Visit " + to_string(u) + "(d=" + to_string(d) + "); ";
        for (int v = 0; v < gN; v++) {
            if (gAdj[u][v] && dist[u] + gAdj[u][v] < dist[v]) {
                dist[v] = dist[u] + gAdj[u][v];
                prev[v] = u;
                pq.push({dist[v], v});
                steps += "Relax " + to_string(u) + "->" + to_string(v) + "=" + to_string(dist[v]) + "; ";
            }
        }
    }
    string s = "{\"algorithm\":\"Dijkstra\",\"start\":" + to_string(start) + ",\"dist\":[";
    for (int i = 0; i < gN; i++) { if(i)s+=","; s+=(dist[i]==INT_MAX?"-1":to_string(dist[i])); }
    s += "],\"steps\":\"" + steps + "\"}";
    static string r; r = s; return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* graph_prim() {
    vector<int> key(gN, INT_MAX), parent(gN, -1);
    vector<bool> inMST(gN, false);
    key[0] = 0;
    string steps = "", mstEdges = "";
    bool firstEdge = true;
    for (int cnt = 0; cnt < gN; cnt++) {
        int u = -1;
        for (int i = 0; i < gN; i++) if (!inMST[i] && (u==-1 || key[i]<key[u])) u=i;
        inMST[u] = true;
        steps += "Add node " + to_string(u) + "; ";
        if (parent[u] != -1) {
            if (!firstEdge) mstEdges += ",";
            mstEdges += "{\"u\":" + to_string(parent[u]) + ",\"v\":" + to_string(u) + ",\"w\":" + to_string(gAdj[parent[u]][u]) + "}";
            firstEdge = false;
        }
        for (int v = 0; v < gN; v++) {
            if (gAdj[u][v] && !inMST[v] && gAdj[u][v] < key[v]) {
                key[v] = gAdj[u][v]; parent[v] = u;
                steps += "Update " + to_string(v) + " key=" + to_string(key[v]) + "; ";
            }
        }
    }
    static string r;
    r = "{\"algorithm\":\"Prim\",\"mst\":[" + mstEdges + "],\"steps\":\"" + steps + "\"}";
    return r.c_str();
}

// ============================================================
//  HASH TABLE (Chaining)
// ============================================================
#define HT_SIZE 11
static vector<vector<pair<int,string>>> hashTable(HT_SIZE);

static int hashFn(int key) { return ((key % HT_SIZE) + HT_SIZE) % HT_SIZE; }

static string htJSON() {
    string s = "{\"size\":" + to_string(HT_SIZE) + ",\"buckets\":[";
    for (int i = 0; i < HT_SIZE; i++) {
        if (i) s += ",";
        s += "[";
        for (int j = 0; j < (int)hashTable[i].size(); j++) {
            if (j) s += ",";
            s += "{\"key\":" + to_string(hashTable[i][j].first) + ",\"val\":\"" + hashTable[i][j].second + "\"}";
        }
        s += "]";
    }
    s += "]}";
    return s;
}

EMSCRIPTEN_KEEPALIVE
const char* ht_insert(int key, const char* val) {
    int idx = hashFn(key);
    for (auto& p : hashTable[idx]) if (p.first == key) { p.second = val; break; }
    hashTable[idx].push_back({key, string(val)});
    static string r;
    r = "{\"inserted\":" + to_string(key) + ",\"bucket\":" + to_string(idx) + ",\"table\":" + htJSON() + "}";
    return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* ht_search(int key) {
    int idx = hashFn(key);
    static string r;
    for (auto& p : hashTable[idx]) {
        if (p.first == key) {
            r = "{\"found\":true,\"key\":" + to_string(key) + ",\"val\":\"" + p.second + "\",\"bucket\":" + to_string(idx) + ",\"table\":" + htJSON() + "}";
            return r.c_str();
        }
    }
    r = "{\"found\":false,\"key\":" + to_string(key) + ",\"bucket\":" + to_string(idx) + ",\"table\":" + htJSON() + "}";
    return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* ht_delete(int key) {
    int idx = hashFn(key);
    auto& chain = hashTable[idx];
    for (auto it = chain.begin(); it != chain.end(); ++it) {
        if (it->first == key) { chain.erase(it); break; }
    }
    static string r;
    r = "{\"deleted\":" + to_string(key) + ",\"bucket\":" + to_string(idx) + ",\"table\":" + htJSON() + "}";
    return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* ht_getState() {
    static string r; r = htJSON(); return r.c_str();
}

EMSCRIPTEN_KEEPALIVE
void ht_clear() { for (auto& b : hashTable) b.clear(); }

} // extern "C"
