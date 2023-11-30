#include <iostream>

using namespace std;

static int summation(int n_max, int m_max, int p_max, int q_max, int ifmap_i[][][][], int filter_f[][][][], int biases_b[], int stride_U)
{
    int output[n_max][m_max][p_max][q_max];

    int sum = 0;
    
    for(int n = 0; n < n_max; n++) {
        for(int m = 0; m < m_max; m++) {
            for(int p = 0; p < p_max; p++) {
                for(int q = 0; q < q_max; q++) { 
                    output[n][m][p][q] = 0; // TODO
                } 
            }
        }
    }

    cout << sum << "\n";

    return sum;
}

int main() {
    // summation(2,3,3);

}