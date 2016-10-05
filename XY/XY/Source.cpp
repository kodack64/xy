#include <Windows.h>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <cstdlib>
#include <algorithm>
using namespace std;
#define L 50
const double initran = 0.8; // initial randomness
const double temp = 0.001;
const int step = 100000;
const int sleep = 1000 / 1;
const double pi = 2 * acos(0.0);
int N = L*L;
double spin[L*L];
mt19937 mt;
uniform_real_distribution<double> dist;

double ene(int i, double s) {
	int x = i%L;
	int y = i/L;
	int dx[4] = { 0,1,0,-1 };
	int dy[4] = { 1,0,-1,0 };
	double sum = 0;
	for (int i = 0; i < 4; i++) {
		int nx = (x + dx[i]+L)%L;
		int ny = (y + dy[i]+L)%L;
		sum += -cos(2.0 * pi*(spin[nx + ny*L] - s));
	}
	return sum;
}
double totEne() {
	double sum = 0.0;
	for (int y = 0; y < L; y++) {
		for (int x = 0; x < L; x++) {
			sum += -cos(2.*pi*(spin[x + y*L] - spin[((x+1)%L) + y*L]));
			sum += -cos(2.*pi*(spin[x + y*L] - spin[x+((y+1)%L)*L]));
		}
	}
	return sum;
}
pair<double, double> totDir() {
	auto p = make_pair(0.0, 0.0);
	for (int i = 0; i < N; i++) {
		p.first += cos(2.*pi*spin[i])/N;
		p.second += sin(2.*pi*spin[i])/N;
	}
	return p;
}
void show() {
	system("cls");
	string s[8] = { "＾","┐","＞","┘","ｖ","└","＜","┌"};
	for (int y = 0; y < L; y++) {
		for (int x = 0; x < L; x++) {
			double val = spin[y*L + x];
			int v = (int)(val*8)%8;
			cout << s[v];
		}
		cout << endl;
	}
}
int main() {
	for (int i = 0; i < N; i++) spin[i] = dist(mt)*initran;
//	for (int i = 0; i < N; i++) spin[i] = 1.0*(i%L + (i/L))/L;
	while (true) {
		for (int i = 0; i < step; i++) {
			int id = mt() % N;
			double ns = dist(mt);
			double dif = ene(id, ns)-ene(id, spin[id]);
			double prob = 1. / (1. + exp(dif / temp));
			if (prob > dist(mt)) spin[id] = ns;
		}
		show();
		cout << "E=" << totEne() << endl;
		auto p = totDir(); cout << p.first << "," << p.second << endl;
		_sleep(sleep);
	}
	return 0;
}