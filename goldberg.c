#include"bch.h"
#include<stdlib.h>
#include<stdio.h>
#include<assert.h>


/* n_partitions[n] = number of partitions of given number n, hardcoded for simplicity.
 * See: https://oeis.org/A000041
 */
static int n_partitions[33] = {1, 1, 2, 3, 5, 7, 11, 15, 22, 30, 42, 56, 77, 
           101, 135, 176, 231, 297, 385, 490, 627, 792, 1002, 1255, 1575, 1958, 
           2436, 3010, 3718, 4565, 5604, 6842, 8349};


static void partitions(int n, uint8_t **P) {
    /* INPUT: n
     * OUTPUT: P[j][i]=a[i] for the jth partition 
     *     a[0]+...+a[m-1]=n, a[0]>=a[1]>=...>=a[m-1]>=1
     *     of n, where  0<=j<n_partitions[n], P[j][m]=0.
     * METHOD: Algorithm P in Section 7.2.14 of
     *    D.E. Knuth: The Art of Computer Programming, Volume 4A
     */
    /* P1: Initialize */
    int np = 0;
    int a[n+1]; 
    a[0] = 0; 
    for (int m=2; m<=n; m++) {
        a[m] = 1;
    }
    int m = 1;
    while (1) { 
        /* P2: Store the final part */
        a[m] = n;
        int q = m - (n==1 ? 1 : 0);
        while (1) {
            /* # P3: Visit */
            for (int j=0; j<m; j++) {
                P[np][j] = a[j+1];
            }
            P[np][m] = 0;
            np++;        
            if (a[q]!=2) {
                break;
            }
            /* P4: Change 2 to 1+1 */
            a[q] = 1;
            q--;
            m++;
        }
        /* P5: Decrease a[q] */
        if (q==0) {
            break;
        }
        int x = a[q]-1;
        a[q] = x;
        n = m-q+1;
        m = q+1;
        /* P6: Copy x if necessary */
        while (n>x) {
            a[m] = x;
            m++;
            n -= x;
        }
    }  
}

goldberg_t goldberg(size_t n) {
    double t0 = tic();

    goldberg_t G;
    int jj[n];
    for (int j=0; j<n; j++) {
        jj[j] = 0;
    }
    int ii[n+1];
    ii[0] = 0;
    int mem_len = 0;
    for (int j=1; j<=n; j++) {
        ii[j] = ii[j-1] + n_partitions[j];
        mem_len += n_partitions[j]*(j+1);
    }

    G.N = n;
    int np = ii[n];
    G.n_partitions = np;

    G.c = malloc(np*sizeof(INTEGER));     
    G.P = malloc(np*sizeof(uint8_t *));     
    G.P[0] = malloc(2*mem_len*sizeof(uint8_t));
    int i=0;
    for (int j=1; j<=n; j++) {
        for (int k=0; k<n_partitions[j]; k++) {
            if (i>0) {
                G.P[i] = G.P[i-1]+j+1;
            }
            i++;
        }
    }
    
    uint8_t **Pn = G.P+ii[n-1];
    partitions(n, Pn);

    G.denom = common_denominator(n);

    expr_t *A = generator(0);
    expr_t *B = generator(1);
    expr_t *ex = logarithm(product(exponential(A), exponential(B)));

    INTEGER e[n+1];
    for (int j=0; j<n; j++){
        e[j] = 0;
    }
    e[n] = G.denom;

    generator_t w[n];
    INTEGER t[n+1];
    for (int k=0; k<n_partitions[n]; k++) {
        int m=0;
        int l=0;
        for (int j=0; Pn[k][j]!=0; j++) {
            l++;
            for (int i=0; i<Pn[k][j]; i++) {
                w[m] = j & 1;
                m++;
            }
        }

        phi(t, n+1, w, ex, e);

        int j = n-1;
        int q = Pn[k][0];
        while (1) {
            G.c[ii[j]+jj[j]] = t[n-j-1];
            if (j<n-1) {
                G.P[ii[j]+jj[j]][0] = q;
                for (int i=1; i<l; i++) {
                    G.P[ii[j]+jj[j]][i] = Pn[k][i];
                }
                G.P[ii[j]+jj[j]][l] = 0;
            }
            jj[j]++;
            if (!((l==1 && q>1) || (l>1 && q>Pn[k][1]))) {
                break;
            }
            j--;
            q--;
        }
    }

    if (get_verbosity_level()>=1) {
        double t1 = toc(t0);
        printf("#compute goldberg coefficients: time=%g sec\n", t1);
        if (get_verbosity_level()>=2) {
            fflush(stdout);
        }
    }

    return G;
}


static int cumsum_partitions[33] = {1, 2, 4, 7, 12, 19, 30, 45, 67, 97, 139, 195, 272, 
    373, 508, 684, 915, 1212, 1597, 2087, 2714, 3506, 4508, 5763, 7338, 9296, 11732, 
    14742, 18460, 23025, 28629, 35471, 43820};

INTEGER goldberg_coefficient(int n, generator_t w[], goldberg_t *G) {
    int x[n];
    for (int i=0; i<n; i++) {
        x[i] = 0;
    }
    int k = 0;
    for (int i=1; i<n; i++) {
        if (w[i-1]==w[i]) {
            k++;
        }
        else {
            x[k]++;
            k = 0;
        }
    }
    x[k]++;
    
    uint8_t p[n+1];
    k = 0;
    for (int i=n-1; i>=0; i--) {
        for (int j=0; j<x[i]; j++) {
            p[k] = i+1;
            k++;
        }
    }
    for (; k<n+1; k++) {
        p[k] = 0;
    }

    int a = cumsum_partitions[n-1]-1;
    int b = a + n_partitions[n] - 1;
    while (a<=b) {
        int c = a + (b-a)/2;
        int i = 0;
        while (p[i]==G->P[c][i]) {
            if (p[i]==0) {
                return ((w[0]==1) && !(n&1)) ? -G->c[c] : G->c[c];
            }
            i+=1;
        }
        if (p[i]>G->P[c][i]) {
            b = c-1;
        }
        else {
            a = c+1;
        }
    }
    fprintf(stderr, "ERROR: partition not found"); 
    exit(EXIT_FAILURE);
}


void print_goldberg(goldberg_t *G) {
    for (int i=0; i<G->n_partitions; i++) {
        for (int j=0; G->P[i][j]!=0; j++) {
            printf("%3i", G->P[i][j]);
        }
        printf("\t");
        print_RATIONAL(G->c[i], G->denom);
        printf("\n");
    }
}


void free_goldberg(goldberg_t G) {
    free(G.P[0]);
    free(G.P);
    free(G.c);
}

