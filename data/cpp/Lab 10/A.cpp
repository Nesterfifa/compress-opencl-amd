#include <bits/stdc++.h>

using namespace std;

typedef long long ll;
typedef long double ld;

#define pb push_back

const double pi = 3.14159265359;
const double eps = 1e-9;
const int mod = 1e9 + 7;
const int mod1 = 999999937;
const int INF = 1e9;
const ll INFLL = 6e18;

vector<vector<int> > g;
vector<int> mt, used;

bool dfs(int v) {
    if (used[v]) {
        return false;
    }
    used[v] = true;
    for (auto u : g[v]) {
        if (mt[u] == -1 || dfs(mt[u])) {
            mt[u] = v;
            return true;
        }
    }
    return false;
}

int main() {
    ios_base::sync_with_stdio(0);
    cin.tie(0);
    cout.tie(0);

    int n, m, k;
    cin >> n >> m >> k;
    g.resize(n);
    mt.resize(m, -1);
    used.resize(n);
    for (int i = 0; i < k; i++) {
        int x, y;
        cin >> x >> y;
        x--; y--;
        g[x].pb(y);
    }
    for (int i = 0; i < n; i++) {
        fill(used.begin(), used.end(), false);
        dfs(i);
    }
    int ans = 0;
    for (int i = 0; i < m; i++) {
        if (mt[i] != -1) {
            ans++;
        }
    }
    cout << ans;
    return 0;
}
