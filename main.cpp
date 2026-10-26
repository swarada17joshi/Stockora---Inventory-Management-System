// Stockora - all C++ DSA algorithms, inventory logic and the web server in ONE file.
// Build:  g++ -std=c++17 -O2 -pthread main.cpp -o stockora      (Windows/MinGW: add -lws2_32)
// Run:    ./stockora          then open http://localhost:8080   (optional port: ./stockora 9000)
#ifdef _WIN32
  #include <winsock2.h>
  typedef SOCKET sock_t;
  #define CLOSESOCK closesocket
#else
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  typedef int sock_t;
  #define CLOSESOCK close
#endif
#include <bits/stdc++.h>          // all the standard C++ headers in one line
using namespace std;

/* ===================== PART 1 - DATA STRUCTURES & ALGORITHMS ===================== */

// Min-heap priority queue: the most urgent item sits at the root.  push/pop O(log n)
template <class T, class Less> class Heap {
    vector<T> a; Less less;
public:
    bool empty() const { return a.empty(); }
    void push(T x) {
        a.push_back(x);
        for (size_t i = a.size() - 1, p; i > 0 && less(a[i], a[p = (i - 1) / 2]); i = p) swap(a[i], a[p]);
    }
    T pop() {
        T top = a[0]; a[0] = a.back(); a.pop_back();
        for (size_t i = 0, n = a.size();;) {
            size_t l = 2 * i + 1, r = l + 1, m = i;
            if (l < n && less(a[l], a[m])) m = l;
            if (r < n && less(a[r], a[m])) m = r;
            if (m == i) break;
            swap(a[i], a[m]); i = m;
        }
        return top;
    }
};

// AVL tree: self-balancing search tree mapping product ID -> product number.  find/insert O(log n)
class AVL {
    struct Node { string k; int v, h = 1; Node *l = nullptr, *r = nullptr; Node(string k, int v) : k(k), v(v) {} };
    Node* root = nullptr;
    static int H(Node* n) { return n ? n->h : 0; }
    static void up(Node* n) { n->h = 1 + max(H(n->l), H(n->r)); }
    static Node* rotR(Node* y) { Node* x = y->l; y->l = x->r; x->r = y; up(y); up(x); return x; }
    static Node* rotL(Node* x) { Node* y = x->r; x->r = y->l; y->l = x; up(x); up(y); return y; }
    static Node* ins(Node* n, const string& k, int v) {
        if (!n) return new Node(k, v);
        if (k < n->k) n->l = ins(n->l, k, v); else if (k > n->k) n->r = ins(n->r, k, v); else { n->v = v; return n; }
        up(n);
        int b = H(n->l) - H(n->r);
        if (b > 1)  { if (k > n->l->k) n->l = rotL(n->l); return rotR(n); }
        if (b < -1) { if (k < n->r->k) n->r = rotR(n->r); return rotL(n); }
        return n;
    }
    static void del(Node* n) { if (n) { del(n->l); del(n->r); delete n; } }
public:
    AVL() = default; AVL(const AVL&) = delete; AVL& operator=(const AVL&) = delete;
    ~AVL() { del(root); }
    void insert(const string& k, int v) { root = ins(root, k, v); }
    int find(const string& k) const { Node* n = root; while (n && k != n->k) n = k < n->k ? n->l : n->r; return n ? n->v : -1; }
};

// Merge sort (stable).  O(n log n)
template <class T, class Less> void mergeSort(vector<T>& a, Less less) {
    vector<T> tmp(a.size());
    function<void(size_t, size_t)> rec = [&](size_t lo, size_t hi) {
        if (hi - lo < 2) return;
        size_t mid = (lo + hi) / 2, i = lo, j = mid, k = lo;
        rec(lo, mid); rec(mid, hi);
        while (i < mid && j < hi) tmp[k++] = less(a[j], a[i]) ? a[j++] : a[i++];
        while (i < mid) tmp[k++] = a[i++];
        while (j < hi) tmp[k++] = a[j++];
        copy(tmp.begin() + lo, tmp.begin() + hi, a.begin() + lo);
    };
    rec(0, a.size());
}

/* ===================== PART 2 - tiny JSON reader/writer ===================== */
struct J {
    enum T { NUL, BOOL, NUM, STR, ARR, OBJ } t = NUL;
    bool b = false; double n = 0; string s; vector<J> a; vector<string> keys; vector<J> vals;
    J() = default;
    J(bool v) : t(BOOL), b(v) {}
    J(const char* v) : t(STR), s(v) {}
    J(const string& v) : t(STR), s(v) {}
    template <class N, class = enable_if_t<is_arithmetic<N>::value && !is_same<N, bool>::value>> J(N v) : t(NUM), n((double)v) {}
    J(initializer_list<pair<string, J>> o) : t(OBJ) { for (auto& kv : o) set(kv.first, kv.second); }   // J{{"key", value}, ...}
    static J Arr() { J j; j.t = ARR; return j; }
    static J Obj() { J j; j.t = OBJ; return j; }
    J& set(const string& k, const J& v) { keys.push_back(k); vals.push_back(v); return *this; }
    J& add(const J& v) { a.push_back(v); return *this; }
    const J* get(const string& k) const { for (size_t i = 0; i < keys.size(); i++) if (keys[i] == k) return &vals[i]; return nullptr; }
};
static string jesc(const string& s) {
    string o = "\"";
    for (unsigned char c : s) {
        if (c == '"') o += "\\\""; else if (c == '\\') o += "\\\\"; else if (c == '\n') o += "\\n"; else if (c == '\r') o += "\\r"; else if (c == '\t') o += "\\t";
        else if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; } else o += (char)c;
    }
    return o + "\"";
}
static void dump(const J& j, string& o) {
    switch (j.t) {
        case J::NUL: o += "null"; break;
        case J::BOOL: o += j.b ? "true" : "false"; break;
        case J::NUM: { char b[40]; snprintf(b, sizeof b, j.n == floor(j.n) && fabs(j.n) < 1e15 ? "%.0f" : "%.10g", j.n); o += b; break; }
        case J::STR: o += jesc(j.s); break;
        case J::ARR: o += "["; for (size_t i = 0; i < j.a.size(); i++) { if (i) o += ","; dump(j.a[i], o); } o += "]"; break;
        case J::OBJ: o += "{"; for (size_t i = 0; i < j.keys.size(); i++) { if (i) o += ","; o += jesc(j.keys[i]) + ":"; dump(j.vals[i], o); } o += "}"; break;
    }
}
static string dumps(const J& j) { string o; dump(j, o); return o; }
struct Parser {
    const string& s; size_t i = 0; bool ok = true;
    explicit Parser(const string& str) : s(str) {}
    void ws() { while (i < s.size() && isspace((unsigned char)s[i])) i++; }
    bool at(char c) { return i < s.size() && s[i] == c; }                  // current character is c?
    J str() {
        if (!at('"')) { ok = false; return J(); }
        i++; string r;
        while (i < s.size() && s[i] != '"') {
            char c = s[i++];
            if (c != '\\' || i >= s.size()) { r += c; continue; }
            char d = s[i++];
            if (d == 'n') r += '\n'; else if (d == 't') r += '\t'; else if (d == 'r') r += '\r';
            else if (d == 'u' && i + 4 <= s.size()) {
                unsigned cp = (unsigned)strtoul(s.substr(i, 4).c_str(), nullptr, 16); i += 4;
                if (cp < 0x80) r += (char)cp;
                else if (cp < 0x800) { r += (char)(0xC0 | (cp >> 6)); r += (char)(0x80 | (cp & 0x3F)); }
                else { r += (char)(0xE0 | (cp >> 12)); r += (char)(0x80 | ((cp >> 6) & 0x3F)); r += (char)(0x80 | (cp & 0x3F)); }
            } else r += d;
        }
        if (i >= s.size()) { ok = false; return J(); }
        i++; return J(r);
    }
    J val() {
        ws(); if (i >= s.size()) { ok = false; return J(); }
        char c = s[i];
        if (c == '{') {
            i++; J o = J::Obj(); ws(); if (at('}')) { i++; return o; }
            while (ok) {
                ws(); J k = str(); if (!ok) break; ws();
                if (!at(':')) { ok = false; break; }
                i++; J v = val(); o.set(k.s, v); ws();
                if (at(',')) { i++; continue; }
                if (at('}')) { i++; break; }
                ok = false;
            }
            return o;
        }
        if (c == '[') {
            i++; J a = J::Arr(); ws(); if (at(']')) { i++; return a; }
            while (ok) {
                a.add(val()); ws();
                if (at(',')) { i++; continue; }
                if (at(']')) { i++; break; }
                ok = false;
            }
            return a;
        }
        if (c == '"') return str();
        if (s.compare(i, 4, "true") == 0) { i += 4; return J(true); }
        if (s.compare(i, 5, "false") == 0) { i += 5; return J(false); }
        if (s.compare(i, 4, "null") == 0) { i += 4; return J(); }
        char* e; double d = strtod(s.c_str() + i, &e);
        if (e == s.c_str() + i) { ok = false; return J(); }
        i = (size_t)(e - s.c_str()); return J(d);
    }
};
static bool parseJson(const string& s, J& out) { Parser p(s); out = p.val(); return p.ok && out.t == J::OBJ; }

/* ===================== PART 3 - INVENTORY LOGIC ===================== */
struct Product { int id = 0; string sku, name, category; int stock = 0; double demand = 0; int lead = 7, safety = 0; };
struct Order { int id = 0, pid = 0, qty = 0; string status, expected; int received = 0; };

static mutex mtx;
static vector<Product> products;
static vector<Order> orders;
static int nextId = 1, bufferDays = 2, reviewDays = 7;
static const char* DB = "data.json";

static string lower(string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
static string trim(const string& s) { size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n"); return a == string::npos ? "" : s.substr(a, b - a + 1); }
static string f1(double v) { char b[32]; snprintf(b, sizeof b, "%.1f", v); string s = b; if (s.size() > 2 && s.substr(s.size() - 2) == ".0") s.resize(s.size() - 2); return s; }
static string dateIn(int days) { time_t t = time(nullptr) + (time_t)days * 86400; char b[16]; strftime(b, sizeof b, "%Y-%m-%d", gmtime(&t)); return b; }
static double numOr(const J* j, double d) { return j && j->t == J::NUM ? j->n : d; }
static string strOr(const J* j) { return j && j->t == J::STR ? j->s : string(); }

static J productJ(const Product& p) {
    return J{{"id", p.id}, {"sku", p.sku}, {"name", p.name}, {"category", p.category}, {"stock", p.stock}, {"demand", p.demand}, {"lead", p.lead}, {"safety", p.safety}};
}
static J orderJ(const Order& o) {
    return J{{"id", o.id}, {"pid", o.pid}, {"qty", o.qty}, {"status", o.status}, {"expected", o.expected}, {"received", o.received}};
}
static void save() {
    J ps = J::Arr(), os = J::Arr();
    for (auto& p : products) ps.add(productJ(p));
    for (auto& o : orders) os.add(orderJ(o));
    ofstream(DB) << dumps(J{{"nextId", nextId}, {"buffer", bufferDays}, {"review", reviewDays}, {"products", ps}, {"orders", os}});
}
static void seed() {
    products.clear(); orders.clear(); nextId = 1;
    struct R { const char *sku, *name, *cat; int stock; double dem; int lead, saf; };
    static const R rows[] = {
        {"BK-1001","A4 notebook, ruled","Stationery",14,6,5,12},{"BK-1002","Gel pen, blue (10 pack)","Stationery",180,9,4,20},
        {"BK-1003","Highlighter set","Stationery",32,3,6,8},{"BK-1004","Stapler, standard","Stationery",9,0.5,10,2},
        {"EL-2001","USB-C cable 1 m","Electronics",22,4,7,10},{"EL-2002","Wireless mouse","Electronics",7,1.5,9,5},
        {"EL-2003","Power bank 10,000 mAh","Electronics",40,1.2,12,6},{"EL-2004","Scientific calculator","Electronics",5,2,8,6},
        {"EL-2005","Earphones","Electronics",0,2.5,7,6},{"SN-3001","Bottled water 500 ml","Snacks",96,40,2,30},
        {"SN-3002","Chips, salted","Snacks",58,22,3,25},{"SN-3003","Instant noodles","Snacks",75,14,3,15},
        {"SN-3004","Chocolate bar","Snacks",110,12,4,20},{"SN-3005","Energy drink","Snacks",16,8,3,10},
        {"HY-4001","Hand sanitizer 100 ml","Hygiene",44,5,5,10},{"HY-4002","Tissue box","Hygiene",120,6,5,15},
        {"HY-4003","Face masks (50)","Hygiene",3,1,6,4},{"HY-4004","Soap bar","Hygiene",64,7,4,12},
        {"AP-5001","Campus hoodie","Apparel",18,0.8,21,4},{"AP-5002","Cotton T-shirt","Apparel",26,1.4,18,6},
        {"AP-5003","Tote bag","Apparel",35,0.6,14,5},{"LB-6001","Lab coat","Lab supplies",12,0.4,15,3},
        {"LB-6002","Safety goggles","Lab supplies",9,1,10,5},{"LB-6003","Graph paper pad","Lab supplies",50,0,7,0}};
    for (auto& r : rows) products.push_back({nextId++, r.sku, r.name, r.cat, r.stock, r.dem, r.lead, r.saf});
    orders.push_back({nextId++, 5, 60, "In progress", dateIn(3), 0});
}
static void load() {
    ifstream f(DB); stringstream ss; ss << f.rdbuf(); J j;
    if (f && parseJson(ss.str(), j) && j.get("products") && j.get("orders")) {
        for (auto& x : j.get("products")->a)
            products.push_back({(int)numOr(x.get("id"), 0), strOr(x.get("sku")), strOr(x.get("name")), strOr(x.get("category")), (int)numOr(x.get("stock"), 0),
                                numOr(x.get("demand"), 0), (int)numOr(x.get("lead"), 7), (int)numOr(x.get("safety"), 0)});
        for (auto& x : j.get("orders")->a)
            orders.push_back({(int)numOr(x.get("id"), 0), (int)numOr(x.get("pid"), 0), (int)numOr(x.get("qty"), 0), strOr(x.get("status")), strOr(x.get("expected")), (int)numOr(x.get("received"), 0)});
        nextId = (int)numOr(j.get("nextId"), 1); bufferDays = (int)numOr(j.get("buffer"), 2); reviewDays = (int)numOr(j.get("review"), 7);
        return;
    }
    products.clear(); orders.clear(); seed(); save();
}

struct Info { Product p; double rp = 0, cover = 1e18; bool hasCover = false, open = false; string status, why; int qty = 1; };
static bool hasOpenOrder(int pid) { for (auto& o : orders) if (o.pid == pid && o.status == "In progress") return true; return false; }
static Info analyze(const Product& p) {
    Info i; i.p = p;
    i.rp = p.demand * p.lead + p.safety;                                   // reorder point
    i.hasCover = p.demand > 0;
    if (i.hasCover) i.cover = p.stock / p.demand;                          // days of cover
    if (p.stock <= 0) i.status = "out";
    else if (p.stock <= i.rp || (i.hasCover && i.cover <= p.lead + bufferDays)) i.status = "low";
    else i.status = "ok";
    i.qty = max(1, (int)ceil(p.demand * (p.lead + reviewDays) + p.safety - p.stock));   // suggested order
    i.open = hasOpenOrder(p.id);
    if (i.status == "out") i.why = "Out of stock." + (p.demand > 0 ? " About " + f1(p.demand) + " sell each day." : string());
    else if (i.status == "low") i.why = (i.hasCover ? "About " + f1(i.cover) + " days of stock left" : "Only " + to_string(p.stock) + " left") + "; delivery takes " + to_string(p.lead) + " days.";
    else i.why = "Stock level is fine.";
    return i;
}
static int tier(const Info& a) { return a.status == "out" ? 0 : (a.p.stock <= a.rp ? 1 : 2); }
struct MoreUrgent {
    bool operator()(const Info& a, const Info& b) const {
        if (tier(a) != tier(b)) return tier(a) < tier(b);
        if (a.cover != b.cover) return a.cover < b.cover;
        if (a.p.demand != b.p.demand) return a.p.demand > b.p.demand;
        return a.p.sku < b.p.sku;
    }
};
static vector<Info> ranked() {                                             // heap-ordered, most urgent first
    Heap<Info, MoreUrgent> h; for (auto& p : products) h.push(analyze(p));
    vector<Info> out; while (!h.empty()) out.push_back(h.pop()); return out;
}
static J infoJ(const Info& i) {
    J j = productJ(i.p);
    j.set("status", i.status).set("cover", i.hasCover ? J(i.cover) : J()).set("reorderPoint", i.rp)
     .set("suggestedQty", i.qty).set("why", i.why).set("hasOpenOrder", i.open);
    return j;
}
static Product* findProduct(int id) { for (auto& p : products) if (p.id == id) return &p; return nullptr; }
static Order* findOrder(int id) { for (auto& o : orders) if (o.id == id) return &o; return nullptr; }
static J orderView(const Order& o) {
    Product* p = findProduct(o.pid); J j = orderJ(o);
    j.set("name", p ? p->name : "Removed product").set("quantity", o.qty); return j;
}
static bool wholeNum(const J& b, const char* k, double lo, int& out) {
    const J* g = b.get(k); if (!g || g->t != J::NUM) return false;
    if (g->n != floor(g->n) || g->n < lo || g->n > 1e9) return false;
    out = (int)g->n; return true;
}
static J validate(const J& b, int selfId, Product& p) {                    // returns field errors (empty = valid)
    J e = J::Obj();
    p.sku = trim(strOr(b.get("sku"))); p.name = trim(strOr(b.get("name"))); p.category = trim(strOr(b.get("category")));
    if (p.category.empty()) p.category = "General";
    if (p.sku.empty()) e.set("sku", "Enter a product ID.");
    else { AVL t; for (auto& x : products) t.insert(lower(x.sku), x.id); int f = t.find(lower(p.sku)); if (f != -1 && f != selfId) e.set("sku", "This ID is already used."); }
    if (p.name.empty()) e.set("name", "Enter a product name.");
    if (!wholeNum(b, "stock", 0, p.stock)) e.set("stock", "Use a whole number, 0 or more.");
    const J* d = b.get("demand"); if (d && d->t == J::NUM && d->n >= 0) p.demand = d->n; else e.set("demand", "Use a number, 0 or more.");
    if (!wholeNum(b, "lead", 1, p.lead)) e.set("lead", "Use a whole number, 1 or more.");
    if (!wholeNum(b, "safety", 0, p.safety)) e.set("safety", "Use a whole number, 0 or more.");
    return e;
}

/* ===================== PART 4 - HTTP API + STATIC FILES ===================== */
struct Req { string method, path, body; map<string, string> q; string param(const string& k, const string& def) const { auto it = q.find(k); return it == q.end() ? def : it->second; } };
struct Res { int code = 200; string type = "application/json"; string body; };
static Res jres(int code, const J& j) { return {code, "application/json", dumps(j)}; }
static Res err(int code, const string& msg, const J& errors = J()) {
    J j = {{"message", msg}}; if (errors.t == J::OBJ && !errors.keys.empty()) j.set("errors", errors); return jres(code, j);
}
static Res fieldErr(const string& field, const string& msg) { return err(400, msg, J{{field, msg}}); }
static Res ok() { return jres(200, J{{"ok", true}}); }
static vector<string> split(const string& s, char c) { vector<string> v(1); for (char ch : s) { if (ch == c) v.emplace_back(); else v.back() += ch; } return v; }
static int idOf(const string& s) { return s.empty() || s.size() > 9 || s.find_first_not_of("0123456789") != string::npos ? -1 : atoi(s.c_str()); }
static string urlDecode(const string& s) {
    string o; for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '+') o += ' ';
        else if (s[i] == '%' && i + 2 < s.size() && isxdigit((unsigned char)s[i + 1]) && isxdigit((unsigned char)s[i + 2])) { o += (char)strtol(s.substr(i + 1, 2).c_str(), nullptr, 16); i += 2; }
        else o += s[i];
    } return o;
}

static Res api(const Req& r) {
    auto P = split(r.path.substr(5), '/');                                 // parts after "/api/"
    const string& m = r.method; const string& a = P[0];
    J body = J::Obj(); bool hasBody = !r.body.empty() && parseJson(r.body, body);

    if (a == "overview" && m == "GET") {
        auto list = ranked(); int ok_ = 0, low = 0, out = 0, need = 0, pend = 0; J att = J::Arr();
        for (auto& i : list) {
            if (i.status == "ok") ok_++; else if (i.status == "low") low++; else out++;
            if (i.status != "ok") { if (!i.open) need++; if (att.a.size() < 6) att.add(infoJ(i)); }
        }
        for (auto& o : orders) if (o.status == "In progress") pend++;
        return jres(200, J{{"total", list.size()}, {"lowStock", low + out}, {"needAction", need}, {"pending", pend},
                           {"counts", J{{"ok", ok_}, {"low", low}, {"out", out}}}, {"attention", att}});
    }
    if (a == "products") {
        if (P.size() == 1 && m == "GET") {
            string q = lower(trim(r.param("q", ""))), st = r.param("status", "all"), cat = r.param("category", "all"), srt = r.param("sort", "urgency");
            AVL idx; for (auto& p : products) idx.insert(lower(p.sku), p.id);
            int exact = q.empty() ? -1 : idx.find(q);                      // exact ID hit through the AVL tree
            vector<Info> out;
            for (auto& i : ranked()) {
                bool hit = q.empty() || i.p.id == exact || lower(i.p.sku).find(q) != string::npos || lower(i.p.name).find(q) != string::npos;
                if (!hit) continue;
                if (st != "all" && !(i.status == st || (st == "lowout" && i.status != "ok"))) continue;
                if (cat != "all" && i.p.category != cat) continue;
                out.push_back(i);
            }
            if (srt == "name") mergeSort(out, [](const Info& x, const Info& y) { return lower(x.p.name) < lower(y.p.name); });
            else if (srt == "stock") mergeSort(out, [](const Info& x, const Info& y) { return x.p.stock < y.p.stock; });
            else if (srt == "demand") mergeSort(out, [](const Info& x, const Info& y) { return x.p.demand > y.p.demand; });
            J arr = J::Arr(); for (auto& i : out) arr.add(infoJ(i)); return jres(200, arr);
        }
        if (P.size() == 1 && m == "POST") {
            if (!hasBody) return err(400, "Invalid request.");
            Product p; J e = validate(body, 0, p); if (!e.keys.empty()) return err(400, "Please fix the highlighted fields.", e);
            p.id = nextId++; products.push_back(p); save(); return jres(201, infoJ(analyze(p)));
        }
        if (P.size() == 2) {
            Product* t = findProduct(idOf(P[1])); if (!t) return err(404, "Product not found.");
            if (m == "GET") return jres(200, infoJ(analyze(*t)));
            if (m == "PUT") {
                if (!hasBody) return err(400, "Invalid request.");
                Product p; J e = validate(body, t->id, p); if (!e.keys.empty()) return err(400, "Please fix the highlighted fields.", e);
                p.id = t->id; *t = p; save(); return jres(200, infoJ(analyze(*t)));
            }
            if (m == "DELETE") {
                int id = t->id;
                if (hasOpenOrder(id)) return err(409, "Receive or cancel this product's open order before deleting it.");
                products.erase(remove_if(products.begin(), products.end(), [&](const Product& p) { return p.id == id; }), products.end());
                save(); return ok();
            }
        }
    }
    if (a == "restock" && m == "GET") {
        J need = J::Arr(), prog = J::Arr(), done = J::Arr();
        for (auto& i : ranked()) if (i.status != "ok" && !i.open) need.add(infoJ(i));
        for (auto& o : orders) if (o.status == "In progress") prog.add(orderView(o));
        for (auto it = orders.rbegin(); it != orders.rend(); ++it) if (it->status != "In progress") done.add(orderView(*it));
        return jres(200, J{{"need", need}, {"progress", prog}, {"done", done}});
    }
    if (a == "orders") {
        if (P.size() == 1 && m == "POST") {
            int pid = 0, qty = 0;
            if (!hasBody || !wholeNum(body, "productId", 1, pid) || !findProduct(pid)) return err(404, "Product not found.");
            if (!wholeNum(body, "quantity", 1, qty)) return fieldErr("oq", "Use a whole number, 1 or more.");
            if (hasOpenOrder(pid)) return err(409, "This product already has an order in progress.");
            Order o{nextId++, pid, qty, "In progress", strOr(body.get("expected")), 0};
            orders.push_back(o); save(); return jres(201, orderView(o));
        }
        if (P.size() == 3 && m == "POST") {
            Order* o = findOrder(idOf(P[1])); if (!o) return err(404, "Order not found.");
            if (P[2] == "receive") {
                if (o->status != "In progress") return err(409, "This delivery was already recorded.");
                int q = 0; if (!hasBody || !wholeNum(body, "quantity", 1, q)) return fieldErr("rq", "Use a whole number, 1 or more.");
                if (Product* p = findProduct(o->pid)) p->stock += q;       // stock rises ONLY here, once
                o->status = "Received"; o->received = q; save(); return jres(200, orderView(*o));
            }
            if (P[2] == "cancel") {
                if (o->status != "In progress") return err(409, "Only orders in progress can be cancelled.");
                o->status = "Cancelled"; save(); return jres(200, orderView(*o));
            }
        }
    }
    if (a == "settings") {
        if (m == "GET") return jres(200, J{{"buffer", bufferDays}, {"review", reviewDays}});
        if (m == "PUT") {
            int b = 0, v = 0; if (!hasBody || !wholeNum(body, "buffer", 0, b) || !wholeNum(body, "review", 0, v)) return err(400, "Enter whole numbers of 0 or more.");
            bufferDays = b; reviewDays = v; save(); return ok();
        }
    }
    if (a == "export" && m == "GET") {
        Res res; res.type = "text/csv"; res.body = "id,name,category,quantity,demand,delivery days,buffer\n";
        auto clean = [](string s) { replace(s.begin(), s.end(), ',', ' '); return s; };   // commas would break the CSV
        for (auto& p : products) res.body += clean(p.sku) + "," + clean(p.name) + "," + clean(p.category) + "," + to_string(p.stock) + "," + f1(p.demand) + "," + to_string(p.lead) + "," + to_string(p.safety) + "\n";
        return res;
    }
    if (a == "import" && m == "POST") {
        int added = 0, skipped = 0; bool first = true;
        for (auto line : split(strOr(body.get("csv")), '\n')) {
            line = trim(line); if (line.empty()) continue;
            auto c = split(line, ','); for (auto& x : c) x = trim(x);
            if (first) { first = false; if (lower(c[0]) == "id") continue; }
            if (c.size() < 6) { skipped++; continue; }
            J b = {{"sku", c[0]}, {"name", c[1]}, {"category", c[2]}, {"stock", atof(c[3].c_str())}, {"demand", atof(c[4].c_str())}, {"lead", atof(c[5].c_str())}, {"safety", c.size() > 6 ? atof(c[6].c_str()) : 0}};
            Product p; if (!validate(b, 0, p).keys.empty()) { skipped++; continue; }
            p.id = nextId++; products.push_back(p); added++;
        }
        save(); return jres(200, J{{"added", added}, {"skipped", skipped}});
    }
    if (a == "reset" && m == "POST") { seed(); save(); return ok(); }
    if (a == "data" && m == "DELETE") { products.clear(); orders.clear(); save(); return ok(); }
    return err(404, "Not found.");
}

static Res serveStatic(const string& path) {
    static const map<string, pair<string, string>> files = {                 // URL -> (file, content type)
        {"/", {"index.html", "text/html; charset=utf-8"}}, {"/index.html", {"index.html", "text/html; charset=utf-8"}},
        {"/style.css", {"style.css", "text/css; charset=utf-8"}}, {"/script.js", {"script.js", "application/javascript; charset=utf-8"}}};
    auto it = files.find(path); if (it == files.end()) return err(404, "Not found.");
    const string& file = it->second.first;
    ifstream f(file, ios::binary); if (!f) return err(404, file + " not found. Run the program from the Stockora folder.");
    stringstream ss; ss << f.rdbuf(); return {200, it->second.second, ss.str()};
}
static Res handle(const Req& r) {
    if (r.path.rfind("/api/", 0) == 0) { lock_guard<mutex> lk(mtx); return api(r); }
    return serveStatic(r.path);
}

static const char* statusText(int code) {
    switch (code) { case 200: return "OK"; case 201: return "Created"; case 400: return "Bad Request"; case 404: return "Not Found"; case 409: return "Conflict"; default: return "Error"; }
}
static void serve(sock_t c) {
    string data; char buf[8192]; size_t hdrEnd = string::npos, need = 0;
    while (data.size() < 8u * 1024 * 1024) {
        int n = recv(c, buf, sizeof buf, 0); if (n <= 0) break; data.append(buf, (size_t)n);
        if (hdrEnd == string::npos && (hdrEnd = data.find("\r\n\r\n")) != string::npos) {
            string h = lower(data.substr(0, hdrEnd)); size_t p = h.find("content-length:");
            need = hdrEnd + 4 + (p == string::npos ? 0 : (size_t)atol(h.c_str() + p + 15));
        }
        if (hdrEnd != string::npos && data.size() >= need) break;
    }
    if (hdrEnd != string::npos) {
        Req r; istringstream ls(data.substr(0, data.find("\r\n"))); string target, ver; ls >> r.method >> target >> ver;
        size_t qm = target.find('?'); r.path = urlDecode(target.substr(0, qm));
        if (qm != string::npos) for (auto& kv : split(target.substr(qm + 1), '&')) { size_t e = kv.find('='); if (e != string::npos) r.q[urlDecode(kv.substr(0, e))] = urlDecode(kv.substr(e + 1)); }
        r.body = data.substr(hdrEnd + 4);
        Res res = handle(r);
        string out = "HTTP/1.1 " + to_string(res.code) + " " + statusText(res.code) + "\r\nContent-Type: " + res.type + "\r\nContent-Length: " + to_string(res.body.size()) + "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n" + res.body;
        for (size_t sent = 0; sent < out.size();) { int n = send(c, out.data() + sent, (int)(out.size() - sent), 0); if (n <= 0) break; sent += (size_t)n; }
    }
    CLOSESOCK(c);
}

int main(int argc, char** argv) {
    int port = argc > 1 ? atoi(argv[1]) : 8080;
#ifdef _WIN32
    WSADATA w; WSAStartup(MAKEWORD(2, 2), &w);
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    load();
    sock_t s = socket(AF_INET, SOCK_STREAM, 0); int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); addr.sin_port = htons((unsigned short)port);
    if (bind(s, (sockaddr*)&addr, sizeof addr) < 0 || listen(s, 64) < 0) { fprintf(stderr, "Could not start on port %d (is it already in use?)\n", port); return 1; }
    printf("Stockora is running at http://localhost:%d  (press Ctrl+C to stop)\n", port);
    for (;;) { sock_t c = accept(s, nullptr, nullptr); if (c < 0) continue; thread(serve, c).detach(); }
}
