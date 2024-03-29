#include <iostream>
#include <cmath>
#include <cstdlib>
#include <omp.h>

using namespace std;

int main()
{
	// random traning sets for XOR -- two inputs and one output

	cout << "topology: 2 4 4 1" << endl;
//#pragma omp parallel
//    {
//#pragma omp for
	for (int i = 200; i >= 0; --i) {
		int n1 = (int)(2.0 * rand() / double(RAND_MAX));
		int n2 = (int)(2.0 * rand() / double(RAND_MAX));
		int t = n1 ^ n2; // should be 0 or 1
		cout << "in: " << n1 << ".0 " << n2 << ".0 " << endl;
		cout << "out: " << t << ".0" << endl;
	}
//    }
}
