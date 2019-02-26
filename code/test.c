#include "math_3d.h"
#include "mt19937.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // malloc(), free
#include <time.h> // time()
// #include "test2.c"

inline static double ran(double low, double high)
{
    return (high - low) * dsfmt_genrand() + low;
}

typedef struct {
    double re;
    double im;
} compl_t;

/* void write(char datafolder_name[128])
{
    char buffer[128];
    strcpy(buffer, datafolder_name);

    strcat(buffer, "/coords_step%07d.poly");
    char output_file[128];
    sprintf(output_file, buffer, 100); // replace %07d with step and put in output_file.

    printf("output_file = %s\n", output_file);

    char datafile[128] = "";
    strcpy(datafile, datafolder_name);
    strcat(datafile, "/coords_step%07d.poly");
    sprintf(datafile, datafile, 100); // replace %07d with step and put in output_file.

    printf("output_file = %s\n", output_file);
}

const char labelstring[] = "v1_%02dpf%04.2lfp%04.1lfa%04.2lf"; */

/* int compare_test(const void* a, const void* b)
{
    return -(*(int*)a - *(int*)b);
} */

// static char chars[128] = "";

/* void my_write(char* aap)
{
    printf("my_write says: %s\n", aap);
} */

/* int aap_with_arg(int aap) // needs arg or error
{
    return aap + 1;
}

int aap_without_arg() // can give arg, no error
{
    return 3;
}

int aap_with_void(void) // needs 0 args or error
{
    return 4;
} */

int main(int argc, char* argv[])
{
    dsfmt_seed(time(NULL));
    /* vec3_t aap = vec3(2, 3, 4);
    float *noot = &aap.x;
    *(noot + 1) = 5;
    for (int i = 0; i < 3; i++) {
        printf("%3.3lf\n", *(noot + i));
    } */

    /*     mat4_t aap = mat4(1,2,3,0,-1,2,-3,0,5,4,3,0,0,0,0,0);
    vec3_t noot = vec3(1, -1, 2);
    mat4_t mies = m4_rotation(0.1, noot);

    m4_print(aap); printf("\n");

    aap = m4_mul(mies, aap);

    m4_print(aap);printf("\n");

    aap = m4_mul(m4_invert_affine(mies), aap);

    m4_print(aap); */

    /*     vec3_t* aap = malloc(10 * sizeof(vec3_t));
    if(!aap){
        printf("mem alloc failed");
        return 1;
    }
    printf("mem alloc succeeded\n");
    for (int i = 0; i < 10; i++) {
        *(aap + i) = vec3(i, 10 - i, (i - 3) * (i - 3));
    }
    printf("init succeeded\n");

    float* pv3;
    for (int i = 0; i < 10; i++) {
        pv3 = &(aap + i)->x; // == &((aap + i)->x);
        for (int d = 0; d < 3; d++) {
            printf("%f ", *(pv3 + d));
        }
        printf("\n");
    }

    printf("size = %ld\n", sizeof(vec3_t));
    printf("aap     = %ld\naap + 1 = %ld\n", aap, aap + 1);

    free(aap); */
    /*     int aap = 0;
    double noot = 0.1 / aap;
    if(noot < 0.4)
        printf("inf < 0.4\n");
    if(noot > 0.6)
        printf("inf > 0.6\n"); */
    /*     for(int random = 0; random < 6; random++)
        if(random & 1)
            printf("%d\n", random); //1 3 5*/

    /* int aap = 2;
    int noot = 2;
    int mies = 3;

    if(aap == noot == mies)
        printf("ja");
    if(aap == noot == mies - 1)
        printf("nee"); // doesn't print because ((2 == 2) == true) != 2?? */
    /* for (int i = -3; i < 3; i++) {
        printf("%d\n", ((i + 3) % 3));
    } */

    /* const int N = 38;
    int aap[N][N][N][N]; // N = 38 is the max for which it works, 38^4 < 2^21
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                for (int l = 0; l < N; l++) {
                    aap[i][j][k][l] = i * N * N * N + j * N * N + k * N + l;
                }
            }
        }
    }
    for (int i = 0; i < N * N; i++) {
        printf("%d ", aap[0][0][i/N][i%N]);
        if((i + 1 % N) != 0)
            printf("\n");
    } */

    /* while(true)
    { // geen fucking warning ech nie????
        printf("ech nie\n");
    } */

    /* int aap = 1;
    printf("%d %d   ", aap++, aap++); // 2 1
    printf("%d\n", aap); // 3 */

    /* int aap[3];
    printf("%d", aap[3000]); // segmentation fault (core dumped) */

    /* FILE* fp = fopen("read_file_test.txt", "r");
    int a=0,b=0;
    double e=0,f=0;
    int garbage;
    fscanf(fp, "%d %d %d %d", &a, &b, &garbage, &garbage);
    fscanf(fp, "%lf %lf %lf %lf", &e, &f, &garbage, &garbage);
    printf("%d %d %lf %lf\n", a, b, e, f);
    fclose(fp); */

    // printf("%d\n", (int)(14.015651999999999/14.015652));
    /* const char string[] = "this is a \
        multiline string, does\
                an enter or tab  or two appear?\
            hello\n"; // no enter, yes tab
    printf(string); */

    /* if (strcmp(argv[1], "3") == 0) {
        printf("aap\n");
    } */

    /* char string[64] = "";
    strcat(string, "dit en zo: %02d %4.2lf\n"); // dit gaat helemaal naar de tering
    sprintf(string, string, 8, 612321423234315675675676523.3324328);
    printf("%s", string); */

    // char buffer[128] = "datafolder/";
    // strcat(buffer, labelstring);
    // // char datafolder_name[128] = "";
    // // replace %4.1lf with packing_fraction and BetaP and Phi
    // printf("%s\n", buffer);
    // sprintf(buffer, buffer, // undefined behaviour, sometimes works, sometimes not
    // /* CubesPerDim */ 8,
    // /* packing_fraction */ 0.5,
    // /* BetaP */ 12.,
    // /* Phi */ 1.25);

    // printf("%s\n", buffer);
    // write(buffer);

    /* int aap = 1, noot = 0, mies = -1;
    printf("%d, %d, %d becomes\n%d, %d, %d\n", aap, noot, mies, ~aap, ~noot, ~mies);
    for (int i = 31; i >= 0; i--) {
        if (mies & (1 << i)) printf("1");
        else printf("0");
    }
    printf("\n"); */

    /* char aap[32] = "aap";
    char noot[32] = "noot baby";
    strcpy(noot, aap);
    printf("noot now contains %s\n", noot); // prints aap
    printf("looping over noot, though:\n");
    for (int i = 0; i < 32; i++) {
        printf("%c", noot[i]); // prints aap baby
    }
    printf("\n"); */

    /* char aap[32];
    double noot[32];
    printf("%ld, %ld\n", sizeof aap, sizeof noot); // prints 32, 256 */

    /* double noot = 0, mies = 0;
    int aap = sscanf("a 3.0 4.0 5.0\n", "%lf %lf", &noot, &mies);
    printf("read %d arguments, namely:\n1st: %lf,\n2nd: %lf\n", aap, noot, mies); */
    // prints 0 arguments, namely 0 and 0.

    /* double aap = 0.1;
    printf("aap is %lf\n", aap); // 0.1
    printf("aap is now %lf\n", (aap+=0.1) - 0.1); // 0.1
    printf("aap is %lf\n", aap); // 0.2 */

    /* aap = 3;
    add_one_to_aap();
    printf("%d\n", aap); */

    /* #define N 6
    int* aap = malloc(sizeof(int) * N);
    int noot[N] = { 5, 1, 1, 1, 8, 4 }; //, 2, 1, 7, 8};
    for (int i = 0; i < N; i++) {
        aap[i] = noot[i];
    }

    qsort(noot, sizeof(noot) / sizeof(noot[0]), sizeof(noot[0]), compare_test);

    for (int i = 0; i < N; i++) {
        printf("aap[%d] = %d, noot[%d] = %d\n", i, aap[i], i, noot[i]);
    }

    int* mies = malloc(sizeof(int) * N);
    for(int i=0;i<N;i++) mies[i]=-1;
    int* wim = malloc(sizeof(int) * N);
    for (int i = 0; i < N; i++) {
        int j;
        for (j = 0; noot[j] != aap[i]; j++)
            ;
        bool not_encountered_yet;
        label:
        not_encountered_yet = true;
        for (int k = 0; k < N && not_encountered_yet; k++) {
            not_encountered_yet = (mies[k] != j);
        }
        if (not_encountered_yet) {
            mies[i] = j;
        } else {
            j++;
            goto label;
        }
    }
    for (int i = 0; i < N; i++) {
        int j;
        for (j = 0; mies[j] != i; j++)
            ;
        wim[i] = j;
    }

    int cmpr(const void* a, const void* b)
    {
        return -aap[*((int*)a)] + aap[*((int*)b)];
    }
    int* zus = malloc(sizeof(int) * N);
    for (int i = 0; i < N; i++) {
        zus[i] = i;
    }
    qsort(zus, N, sizeof(int), &cmpr);
    int* jet = malloc(sizeof(int) * N);
        for (int i = 0; i < N; i++) {
        int bha;
        for (bha = 0; zus[bha] != i; bha++)
            ;
        jet[i] = bha;
    }

    for (int i = 0; i < N; i++) {
        printf("aap[%d] = %d, noot[%d] = %d, mies[%d] = %d, wim[%d] = %d, zus[%d] = %d, \
jet[%d] = %d\n", i, aap[i], i, noot[i], i, mies[i], i, wim[i], i, zus[i], i, jet[i]);
    }

    free(aap);
    free(mies);
    free(wim); // wim is the same as zus, requires aap and noot (noot is aap sorted)
    free(zus);
    free(jet); */

    /*
    aap[0] = 5, noot[0] = 8, mies[0] = 1, wim[0] = 4, zus[0] = 4, jet[0] = 1
    aap[1] = 1, noot[1] = 5, mies[1] = 3, wim[1] = 0, zus[1] = 0, jet[1] = 3
    aap[2] = 1, noot[2] = 4, mies[2] = 4, wim[2] = 5, zus[2] = 5, jet[2] = 4
    aap[3] = 1, noot[3] = 1, mies[3] = 5, wim[3] = 1, zus[3] = 1, jet[3] = 5
    aap[4] = 8, noot[4] = 1, mies[4] = 0, wim[4] = 2, zus[4] = 2, jet[4] = 0
    aap[5] = 4, noot[5] = 1, mies[5] = 2, wim[5] = 3, zus[5] = 3, jet[5] = 2
    */

    /* int aap = 0;
    for (int i = 0; i < 100; i++) {
        if (++aap >= 10) {
            printf("YEAH ");
            aap = 0;
        }
        printf("%d\n", i);
    } */

    /* if (argc < 2) return 1;
    sscanf(argv[1], "%s", chars);
    printf("chars = %s\n", chars); */

    /* int aap = 3;
    int* noot = &aap;
    int* mies = noot;
    printf("aap = %d\n", aap); // 3
    *noot = *noot + 3;
    printf("aap = %d\n", aap); // 6
    *noot += 3;
    printf("aap = %d\n", aap); // 9
    *mies += 3;
    printf("aap = %d\n", aap); // 12 */

    /* enum aap{a, b, c, d} noot;
    noot = b;
    switch(noot){
        case a:
            printf("a\n");
            break;
        case b:
            printf("b\n"); // b
        case c:
            printf("c\n"); // c
            break;
        case d:
            printf("d\n");
            break;
    } */

    // char aap[128] = "aap";
    // my_write(aap); // my_write says: aap

    /* enum {a=1, b=2, c=4} aap;
    aap = a | b;
    if (aap & a) {
        printf("a\n"); // yes
    }
    if (aap & b) {
        printf("b\n"); // yes
    }
    if (aap & (a | b)) {
        printf("a&b\n"); // yes
    }
    if (aap == (a | b)) {
        printf("a&b\n"); // yes
    }
    if (aap & c) {
        printf("c\n"); // no
    }
    if (aap & (a | c)) {
        printf("a | c\n"); // yes
    } */
    /* printf("%d ", aap_with_arg(1)); // ok, 2
    printf("%d ", aap_without_arg(10)); // ok, 3
    printf("%d\n", aap_with_void(20)); // error */

    /* if (NULL) printf("if NULL\n");
    if (!NULL) printf("if !NULL\n"); // !NULL */

    /* vec3_t aap = vec3(0,0,0);
    printf("aap.x = %lf\n", aap.x); // 0
    printf("aap.y = %lf\n", aap.y); // 0
    printf("aap.z = %lf\n", aap.z); // 0
    // add one to the second member of aap in the ugliest way possible
    *(&(aap.x) + 1) += 1;
    printf("aap.x = %lf\n", aap.x); // 0
    printf("aap.y = %lf\n", aap.y); // 1
    printf("aap.z = %lf\n", aap.z); // 0 */

    /* double aap[1000];
    memset(aap, 0, 1000 * sizeof(double)); // necessary
    printf("i\taap[i]\n");
    for (int i = 0; i < 1000; i++) {
        printf("%d\t%lf\n", i, aap[i]);
    } */
    
    // two pointers to one object test
    /* compl_t aap;
    aap.re = 1; aap.im = 2;
    printf("aap = %4.2lf + %4.2lfi\n", aap.re, aap.im); // aap = 1.00 + 2.00i
    compl_t *pnoot = &aap;
    compl_t *pmies = pnoot;
    pnoot->re = 3;
    printf("aap = %4.2lf + %4.2lfi\n", aap.re, aap.im); // aap = 3.00 + 2.00i
    pmies->im = 4;
    printf("aap = %4.2lf + %4.2lfi\n", aap.re, aap.im); // aap = 3.00 + 4.00i */

// #define groottevan sizeof
// #define dubbel double

//     printf("grootte van dubbel: %d\n", groottevan(dubbel));

    /* compl_t aap;
    printf("size of compl_t = %ld\n", sizeof(aap)); // 16
    compl_t *noot = &aap;
    printf("address of aap = %p\n", noot);
    printf("address of aap + 1 = %p\n", noot + 1); // + 16 */

    /* printf("int sz = %ld\n", sizeof(int)); // 4
    printf("short sz = %ld\n", sizeof(short)); // 2
    printf("long sz = %ld\n", sizeof(long)); // 8
    printf("long long sz = %ld\n", sizeof(long long)); // 4
    printf("float sz = %ld\n", sizeof(float)); // 4
    printf("double sz = %ld\n", sizeof(double)); // 8
    printf("long double sz = %ld\n", sizeof(long double)); // 16 */

    /* FILE *fp = fopen("tmptest.txt","w");
    fprintf(fp, "%le\n", 100000000.); // 1.000000e+08
    fprintf(fp, "%le\n", 10000000.); // 1.000000e+07
    fprintf(fp, "%le\n", 1000000.); // 1.000000e+06
    fprintf(fp, "%le\n", 100000.); // 1.000000e+05
    fprintf(fp, "%le\n", 10000.); // 1.000000e+04
    fprintf(fp, "%le\n", 1000.); // 1.000000e+03
    fprintf(fp, "%le\n", 100.); // 1.000000e+02
    fprintf(fp, "%le\n", 10.); // 1.000000e+01
    fprintf(fp, "%le\n", 1.); // 1.000000e+00
    fprintf(fp, "%le\n", 0.1); // 1.000000e-01
    fprintf(fp, "%le\n", 0.01); // 1.000000e-02
    fprintf(fp, "%le\n", 0.001); // 1.000000e-03
    fprintf(fp, "%le\n", 0.0001); // 1.000000e-04
    fprintf(fp, "%le\n", 0.00001); // 1.000000e-05
    fprintf(fp, "%le\n", 0.000001); // 1.000000e-06
    fprintf(fp, "%le\n", 0.0000001); // 1.000000e-07
    fprintf(fp, "%le\n", 0.00000001); // 1.000000e-08
    fprintf(fp, "%lg\n", 100000000.); // 1e+08
    fprintf(fp, "%lg\n", 10000000.); // 1e+07
    fprintf(fp, "%lg\n", 1000000.); // 1e+06
    fprintf(fp, "%lg\n", 100000.); // 100000
    fprintf(fp, "%lg\n", 10000.); // 10000
    fprintf(fp, "%lg\n", 1000.); // 1000
    fprintf(fp, "%lg\n", 100.); // 100
    fprintf(fp, "%lg\n", 10.); // 10
    fprintf(fp, "%lg\n", 1.); // 1
    fprintf(fp, "%lg\n", 0.1); // 0.1
    fprintf(fp, "%lg\n", 0.01); // 0.01
    fprintf(fp, "%lg\n", 0.001); // 0.001
    fprintf(fp, "%lg\n", 0.0001); // 0.0001
    fprintf(fp, "%lg\n", 0.00001); // 1e-05
    fprintf(fp, "%lg\n", 0.000001); // 1e-06
    fprintf(fp, "%lg\n", 0.0000001); // 1e-07
    fprintf(fp, "%lg\n", 0.00000001); // 1e-08
    fprintf(fp, "%lg\n", 123450000.); // 1.2345e+08
    fprintf(fp, "%lg\n", 12345000.); // 1.2345e+07
    fprintf(fp, "%lg\n", 1234500.); // 1.2345e+06
    fprintf(fp, "%lg\n", 123450.); // 123450
    fprintf(fp, "%lg\n", 12345.); // 12345
    fprintf(fp, "%lg\n", 1000.); // 1000
    fprintf(fp, "%lg\n", 100.); // 100
    fprintf(fp, "%lg\n", 10.); // 10
    fprintf(fp, "%lg\n", 1.); // 1
    fprintf(fp, "%lg\n", 0.1); // 0.1
    fprintf(fp, "%lg\n", 0.01); // 0.01
    fprintf(fp, "%lg\n", 0.001); // 0.001
    fprintf(fp, "%lg\n", 0.00012345); // 0.00012345
    fprintf(fp, "%lg\n", 0.000012345); // 1.2345e-05
    fprintf(fp, "%lg\n", 0.0000012345); // 1.2345e-06
    fprintf(fp, "%lg\n", 0.00000012345); // 1.2345e-07
    fprintf(fp, "%lg\n", 0.000000012345); // 1.2345e-08
    fprintf(fp, "%lg\n", 0.000000012345123123123123123); // 1.23451e-08 */

    /* int aap;
    if (EOF == sscanf(argv[1], "%d", &aap)) {
        printf("bad\n");
    }
    printf("aap = % d\n", aap); */

    return 0;
}