#include <bits/stdc++.h>

using namespace std;

typedef long long ll;
typedef long double ld;
typedef unsigned int uint;

#define pb push_back
#define ins insert

const double pi = 3.14159265359;
const double eps = 1e-9;
const int mod = 1e9 + 7;
const int mod1 = 1e9 + 9;
const int INF = 1e9;
const ll INFLL = 3e18;

vector<int> p(400000);

int leader(int v) {
    return (p[v] == v) ? v : p[v] = leader(p[v]);
}

int main() {
    ios_base::sync_with_stdio(0);
    cin.tie(0);
    cout.tie(0);

    int n;
    cin >> n;
    for (int i = 0; i < n + 5; i++) {
        p[i] = i % n;
    }
    while (n--) {
        int x;
        cin >> x;
        x--;
        x = leader(x);
        p[x] = leader(p[x + 1]);
        cout << x + 1 << ' ';
    }
    return 0;
}
