#include "math_3d.h"
#include <malloc.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h> // mkdir on Windows
#elif __linux__
#include <sys/stat.h> // mkdir on Linux
#include <sys/types.h> // mkdir on Linux
#endif

#define MAX_PART_CELL 40
#define MAX_NEIGHBORS 50
#define INVFPI 0.07957747154594766788444188168625718101
#define M_PI 3.14159265358979323846
#define N 8000
#define NDIM 3

//tunable parameters:
double bndLength = 1.55; //distance cutoff for bonds
double bnd_cuttoff = 0.6; //Order to be called a correlated bond
int nbnd_cuttoff = 4; //Number of correlated bonds for a crystalline particle
double obnd_cuttoff = 0.0; //Order to be in the same cluster (0.0 for all touching clusters 0.9 to see defects)

//////////////////////////////////////////////////////////////// my additions
const char usage_string[] = "usage: program.exe datafolder output_per (p/pos or o/orient) \n";
static double Phi;
static int output_per;
static int n_particles;
static double ruud_box[3];
static vec3_t ruud_r[N]; // position of center of cube
static mat4_t ruud_m[N]; // rotation matrix of cube
static vec3_t Normal[3];
static double SinPhi;
static double CosPhi;
static double ParticleVolume;
bool is_pos = true;
static char datafolder_name[128];
//////////////////////////////////////////////////////////////// end of my additions

typedef struct {
    double re;
    double im;
} compl_t;
typedef struct {
    double nz;
    double si;
    double co;
    int n;
} bndT_t;
typedef struct {
    int n;
    bndT_t* bnd;
} blistT_t;
blistT_t* blist;
int* numconn;
double xsize, ysize, zsize;

double bndLengthSq;

int n_cells;
double avr_d, max_d, min_cell;
int nx, ny, nz;

double percentage;
int maxsize;
int numclus;

typedef struct {
    double x;
    double y;
    double z;
    double xhalf;
    double yhalf;
    double zhalf;
    double oneoverx;
    double oneovery;
    double oneoverz;
    double min;
} tbox_t;
tbox_t box;

typedef struct {
    double x;
    double y;
    double z;
    double d;
    double r;
    int cell;
    char c;
    char str[256];
} tpart_t;
tpart_t* part;

typedef struct scell {
    double x1;
    double y1;
    double z1;
    double x2;
    double y2;
    double z2;
    int buren[27];
    int n;
    int particles[MAX_PART_CELL];
} tcells_t;
tcells_t* cells;

double sqr(double x)
{
    return x * x;
}

/************************************************
 *             DIST_PART
 * Distance between two particles
 ***********************************************/
double dist_part(int i, int j)
{
    double dx = part[i].x - part[j].x;
    double dy = part[i].y - part[j].y;
    double dz = part[i].z - part[j].z;
    if (dx > box.xhalf)
        dx = dx - box.x;
    else if (dx < -box.xhalf)
        dx = dx + box.x;
    if (dy > box.yhalf)
        dy = dy - box.y;
    else if (dy < -box.yhalf)
        dy = dy + box.y;
    if (dz > box.zhalf)
        dz = dz - box.z;
    else if (dz < -box.zhalf)
        dz = dz + box.z;
    return dx * dx + dy * dy + dz * dz;
}

/************************************************
 *             PLGNDR
 * Legendre polynomial
 ***********************************************/
float plgndr(int l, int m, double x)
{
    double fact, pll = 0.0, pmm, pmmp1, somx2;
    int i, ll;
    if (m < 0 || m > l || fabs(x) > 1.0)
        printf("Bad arguments in routine plgndr %i %i %f\n", l, m, fabs(x));
    pmm = 1.0;
    if (m > 0) {
        somx2 = sqrt((1.0 - x) * (1.0 + x));
        fact = 1.0;
        for (i = 1; i <= m; i++) {
            pmm *= -fact * somx2;
            fact += 2.0;
        }
    }
    if (l == m)
        return pmm;
    else {
        pmmp1 = x * (2 * m + 1) * pmm;
        if (l == (m + 1))
            return pmmp1;
        else {
            for (ll = m + 2; ll <= l; ll++) {
                pll = (x * (2 * ll - 1) * pmmp1 - (ll + m - 1) * pmm) / (ll - m);
                pmm = pmmp1;
                pmmp1 = pll;
            }
            return pll;
        }
    }
}

/************************************************
 *             GAMMLN
 * Log of the gamma function
 ***********************************************/
double gammln(double xx)
{
    double x, y, tmp, ser;
    static double cof[6] = { 76.18009172947146, -86.50532032941677,
        24.01409824083091, -1.231739572450155,
        0.1208650973866179e-2, -0.5395239384953e-5 };
    int j;
    y = x = xx;
    tmp = x + 5.5;
    tmp -= (x + 0.5) * log(tmp);
    ser = 1.000000000190015;
    for (j = 0; j <= 5; j++)
        ser += cof[j] / ++y;
    return -tmp + log(2.5066282746310005 * ser / x);
}

/************************************************
 *             FACS
 * Calculate factorials
 ***********************************************/
double facs(int l, int m)
{
    static double* fac_table[14] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }; //max l=10
    int a, b;
    if (fac_table[l] == NULL) {
        fac_table[l] = malloc(sizeof(double) * (2 * l + 1));
        for (a = 0; a < 2 * l + 1; a++) {
            b = a - l;
            fac_table[l][a] = exp(gammln(l - b + 1) - gammln(l + b + 1));
        }
    }
    return fac_table[l][m + l];
}

/************************************************
 *             MINPOW
 * Powers of -1
 ***********************************************/
double minpow(int m)
{
    if ((m & 1) == 1)
        return -1.0;
    else
        return 1.0;
}

/************************************************
 *             ORIENT_ORDER
 * Calculate i for a particle along one axis
 ***********************************************/
// TODO: choose the normal axes or box-edge aligned axes?
void orient_order(int l, int i, compl_t* res, int axis)
{
    vec3_t original_dir = vec3(0 & axis, 1 & axis, 2 & axis);
    vec3_t dir = v3_norm(m4_mul_dir(ruud_m[i], original_dir));
    double fc, p, f, s, r, sp, spp, c, cp, cpp;
    double z;
    int m = 0;
    // z = bnd->nz;
    z = dir.z;
    double si, co;
    if (dir.x == 0 && dir.y == 0) {
        si = 0;
        co = 1;
    } else {
        double norm = 1.0 / sqrt(dir.x * dir.x + dir.y * dir.y);
        si = dir.x > 0 ? dir.y * norm : -dir.y * norm;
        co = fabs(dir.x) * norm;
    }

    //for m=0
    p = plgndr(l, 0, z);
    fc = facs(l, 0);
    f = sqrt((2 * l + 1) * INVFPI * fc);
    r = p * f;
    (res + 0)->re += r;
    (res + 0)->im += 0;
    // (res2 + 0)->re += r * minpow(l);
    // (res2 + 0)->im += 0;
    s = 0;
    sp = 0;
    c = 1;
    cp = 0;

    for (m = 1; m <= l; m++) {
        //positive m
        p = plgndr(l, m, z);
        fc = facs(l, m);
        f = sqrt((2 * l + 1) * INVFPI * fc);
        r = p * f;
        cpp = cp;
        cp = c;
        if (m == 1)
            // c = bnd->co;
            c = co;
        else
            // c = 2.0 * bnd->co * cp - cpp; //some cosine tricks
            c = 2.0 * co * cp - cpp; //some cosine tricks

        spp = sp;
        sp = s;
        if (m == 1)
            // s = bnd->si;
            s = si;
        else
            // s = 2.0 * bnd->co * sp - spp; //some sine tricks
            s = 2.0 * co * sp - spp; //some sine tricks

        (res + m)->re += r * c;
        (res + m)->im += r * s;
        // (res2 + m)->re += r * c;
        // (res2 + m)->im += r * s;

        //negative m
        r *= minpow(m);
        (res - m)->re += r * c;
        (res - m)->im += -r * s;
        // (res2 - m)->re += r * c;
        // (res2 - m)->im += -r * s;

        //printf ("Test: %d, %lf, %lf (cumu: %lf, %lf) \n", m, r*c,-r*s, (res1-m)->re, (res1-m)->im);
    }
}

/************************************************
 *             POS_ORDER
 * Calculate q for a pair of particles
 ***********************************************/
void pos_order(int l, bndT_t* bnd, compl_t* res1, compl_t* res2)
{
    double fc, p, f, s, r, sp, spp, c, cp, cpp;
    double z;
    int m = 0;
    z = bnd->nz;

    //for m=0
    p = plgndr(l, 0, z);
    fc = facs(l, 0);
    f = sqrt((2 * l + 1) * INVFPI * fc);
    r = p * f;
    (res1 + 0)->re += r;
    (res1 + 0)->im += 0;
    (res2 + 0)->re += r * minpow(l);
    (res2 + 0)->im += 0;
    s = 0;
    sp = 0;
    c = 1;
    cp = 0;

    for (m = 1; m <= l; m++) {
        //positive m
        p = plgndr(l, m, z);
        fc = facs(l, m);
        f = sqrt((2 * l + 1) * INVFPI * fc);
        r = p * f;
        cpp = cp;
        cp = c;
        if (m == 1)
            c = bnd->co;
        else
            c = 2.0 * bnd->co * cp - cpp; //some cosine tricks

        spp = sp;
        sp = s;
        if (m == 1)
            s = bnd->si;
        else
            s = 2.0 * bnd->co * sp - spp; //some sine tricks

        (res1 + m)->re += r * c;
        (res1 + m)->im += r * s;
        (res2 + m)->re += r * c;
        (res2 + m)->im += r * s;

        //negative m
        r *= minpow(m);
        (res1 - m)->re += r * c;
        (res1 - m)->im += -r * s;
        (res2 - m)->re += r * c;
        (res2 - m)->im += -r * s;

        //printf ("Test: %d, %lf, %lf (cumu: %lf, %lf) \n", m, r*c,-r*s, (res1-m)->re, (res1-m)->im);
    }
}

/************************************************
 *             DOTPROD
 * Dot product
 ***********************************************/
double dotprod(compl_t* vec1, compl_t* vec2, int l)
{
    double res = 0;
    int m;
    for (m = -l; m <= l; m++)
        res += (*(vec1 + m + l)).re * (*(vec2 + m + l)).re + (*(vec1 + m + l)).im * (*(vec2 + m + l)).im;
    return res;
}

/************************************************
 *             COORDS2CELL
 * Find cell associated with particle position
 ***********************************************/
int coords2cell(double x, double y, double z)
{
    int i = ((int)(nx * (x + box.xhalf) * box.oneoverx) % nx) * ny * nz + ((int)(ny * (y + box.yhalf) * box.oneovery) % ny) * nz + ((int)(nz * (z + box.zhalf) * box.oneoverz) % nz);
    return i;
}

/************************************************
 *             UPDATE_NBLISTP
 * Find neighbors of a particle
 ***********************************************/
void update_nblistp(int p)
{
    int i, j, c, cellp, id;
    bndT_t* bnd;
    double d, dxy, dx, dy, dz;
    cellp = part[p].cell;
    blist[p].n = 0;
    for (i = 26; i >= 0; i--) {
        c = cells[cellp].buren[i];
        for (j = cells[c].n - 1; j >= 0; j--) {
            id = cells[c].particles[j];
            if (id != p) {
                dx = part[p].x - part[id].x;
                dy = part[p].y - part[id].y;
                dz = part[p].z - part[id].z;

                if (dx > box.xhalf)
                    dx -= box.x;
                else if (dx < -box.xhalf)
                    dx += box.x;
                if (dy > box.yhalf)
                    dy -= box.y;
                else if (dy < -box.yhalf)
                    dy += box.y;
                if (dz > box.zhalf)
                    dz -= box.z;
                else if (dz < -box.zhalf)
                    dz += box.z;

                d = dx * dx + dy * dy + dz * dz;

                if (d < bndLengthSq) {
                    bnd = &(blist[p].bnd[blist[p].n]);
                    blist[p].n++;
                    //          printf ("Test: %d, %d, %d, %d, %d\n", p, id, blist[p].n, i, j);
                    bnd->n = id;
                    if (id > p) {
                        d = 1.0 / sqrt(d);
                        //            bnd->nx = dx*d;
                        //            bnd->ny = dy*d;
                        bnd->nz = dz * d;
                        if (dx == 0 && dy == 0) {
                            bnd->si = 0;
                            bnd->co = 1;
                        } else {
                            dxy = 1.0 / sqrt(dx * dx + dy * dy);
                            bnd->si = dy * dxy;
                            bnd->co = dx * dxy;
                        }
                    }
                }
            }
        }
    }
}

/************************************************
 *             UPDATE_NBLIST
 * Make neighbor list for all particles
 ***********************************************/
void update_nblist(void)
{
    int p;
    for (p = n_particles - 1; p >= 0; p--) {
        update_nblistp(p);
    }
}

/************************************************
 *             INIT_NBLIST
 * Initialize the neighbor list
 ***********************************************/
void init_nblist(void)
{
    int p;
    blist = (blistT_t*)malloc(sizeof(blistT_t) * n_particles);
    numconn = (int*)malloc(sizeof(int) * n_particles);
    for (p = n_particles - 1; p >= 0; p--) {
        blist[p].bnd = (bndT_t*)malloc(sizeof(bndT_t) * MAX_NEIGHBORS);
        numconn[p] = 0;
        update_nblistp(p);
    }
}

/************************************************
 *             INIT_CELLS
 * Initialize the cell list
 ***********************************************/
void init_cells(void)
{
    n_cells = 0;
    int x, y, z, i, j, k;
    int dx, dy, dz, a, b, c;
    // double cellsize = max_d;
    double cellsize = bndLength;
    // if (cellsize < bndLength)
    //     cellsize = bndLength;
    double ruud_min_cell_size = sqrt(3 + 2 * CosPhi);
    if (cellsize < ruud_min_cell_size)
        cellsize = ruud_min_cell_size;
    nx = floor(box.x / cellsize);
    ny = floor(box.y / cellsize);
    nz = floor(box.z / cellsize);
    min_cell = box.x / nx;
    if (box.y / ny < min_cell)
        min_cell = box.y / ny;
    if (box.z / nz < min_cell)
        min_cell = box.z / nz; //set min_cell
    // printf("cells: %d, %d, %d (%lf)\n", nx, ny, nz, cellsize);
    n_cells = nx * ny * nz;
    // printf ("Cells: %d (%lf)\n", n_cells, cellsize);
    cells = (tcells_t*)malloc(n_cells * sizeof(tcells_t));
    for (x = 0; x < nx; x++)
        for (y = 0; y < ny; y++)
            for (z = 0; z < nz; z++) {
                i = x * ny * nz + y * nz + z;
                cells[i].x1 = x * box.x / nx - box.xhalf;
                cells[i].y1 = y * box.y / ny - box.yhalf;
                cells[i].z1 = z * box.z / nz - box.zhalf;
                cells[i].x2 = (x + 1) * box.x / nx - box.xhalf;
                cells[i].y2 = (y + 1) * box.y / ny - box.yhalf;
                cells[i].z2 = (z + 1) * box.z / nz - box.zhalf;
                cells[i].n = 0;
                k = 0;
                for (a = -1; a < 2; a++)
                    for (b = -1; b < 2; b++)
                        for (c = -1; c < 2; c++) { //adding one self as a neighbor now (handy or not??)
                            dx = a;
                            dy = b;
                            dz = c;
                            if (x + dx < 0)
                                dx = nx - 1;
                            if (x + dx > nx - 1)
                                dx = -nx + 1;
                            if (y + dy < 0)
                                dy = ny - 1;
                            if (y + dy > ny - 1)
                                dy = -ny + 1;
                            if (z + dz < 0)
                                dz = nz - 1;
                            if (z + dz > nz - 1)
                                dz = -nz + 1;
                            j = (x + dx) * ny * nz + (y + dy) * nz + (z + dz);
                            //      printf("%i   %i %i %i  %i %i %i\n",j,dx,dy,dz, x+dx,y+dy,z+dz);
                            cells[i].buren[k] = j;
                            k++;
                        }
            }
    for (i = 0; i < n_particles; i++) {
        j = coords2cell(part[i].x, part[i].y, part[i].z);
        if (cells[j].n + 1 >= MAX_PART_CELL) {
            printf("ERROR: Too many particles in a cell!\n");
            exit(666);
        }
        cells[j].particles[cells[j].n] = i;
        part[i].cell = j;
        cells[j].n++;
    }
}

/************************************************
 *             GETLINE_NUMBER
 * Read a line from a file
 * Ignore lines starting with #
 ***********************************************/
void getline_number(char* str, int n, FILE* f)
{
    int comment = 1;
    while (comment) {
        char* res = fgets(str, n, f);
        if (!res)
            return;
        if (str[0] != '#')
            comment = 0;
    }
}

/************************************************
 *             INIT_BOX
 * Initialize box parameters
 ***********************************************/
void init_box(double x_box, double y_box, double z_box)
{
    box.x = x_box;
    box.y = y_box;
    box.z = z_box;
    box.min = box.x;
    if (box.min > box.y)
        box.min = box.y;
    if (box.min > box.z)
        box.min = box.z;
    box.xhalf = box.x * 0.5;
    box.yhalf = box.y * 0.5;
    box.zhalf = box.z * 0.5;
    box.oneoverx = 1.0 / box.x;
    box.oneovery = 1.0 / box.y;
    box.oneoverz = 1.0 / box.z;
}

/************************************************
 *             CONVERT_DATA
 * converts data from my type to this file's weird type
 ***********************************************/
int convert_data()
{
    // n_part = particlestocount = n_particles;
    part = malloc(n_particles * sizeof(tpart_t));
    for (int n = 0; n < n_particles; n++) {
        part[n].x = ruud_r[n].x;
        part[n].y = ruud_r[n].y;
        part[n].z = ruud_r[n].z;
        part[n].c = 'a';
        part[n].d = 1.0;
    }
    init_box(ruud_box[0], ruud_box[1], ruud_box[2]);

    bndLengthSq = bndLength * bndLength;
    init_cells();
    init_nblist();
    return 0;
}

/************************************************
 *             CALC_ORIENT_ORDER
 * Calculate i_4 for all particles
 ***********************************************/
compl_t* calc_orient_order(void)
{
    compl_t *q1, *q2, *q3;
    const int l = 4;
    // allocate two spaces too many to make place for q2, q3
    compl_t* orderp = (compl_t*)malloc(sizeof(compl_t) * (n_particles + 2) * (l * 2 + 1));
    memset(orderp, (int)0.0, sizeof(compl_t) * n_particles * (l * 2 + 1));
    for (int i = 0; i < n_particles; i++) {
        q1 = (orderp + i * (2 * l + 1) + l);
        q2 = (orderp + (i + 1) * (2 * l + 1) + l); // temporary place for q2, q3
        q3 = (orderp + (i + 2) * (2 * l + 1) + l); // will be overwritten next loop
        
        // the folling gives order vectors i_{4,m} corresponding to the three particle axes
        // and stores them in q1, q2, q3. Next we have to add q2, q3 to q1 and normalize 
        orient_order(l, i, q1, 0);
        orient_order(l, i, q2, 1);
        orient_order(l, i, q3, 2);
        for (int m = -l; m <= l; m++) {
            (q1 + m)->re += (q2 + m)->re + (q3 + m)->re;
            (q1 + m)->im += (q2 + m)->im + (q3 + m)->im;
        }
        //normalize vector
        double temp = 1.0 / sqrt(dotprod(q1, q1, l));
        for (int m = -l; m < l; m++) {
            (q1 + m)->re *= temp;
            (q1 + m)->im *= temp;
        }
    }
    return orderp;
}

/************************************************
 *             CALC_POS_ORDER
 * Calculate q_4 for all particles
 ***********************************************/
compl_t* calc_pos_order(void)
{
    int i, j, m;
    compl_t *q1, *q2;
    const int l = 4;
    double temp;
    compl_t* orderp = (compl_t*)malloc(sizeof(compl_t) * n_particles * (l * 2 + 1));
    memset(orderp, (int)0.0, sizeof(compl_t) * n_particles * (l * 2 + 1));
    for (i = 0; i < n_particles; i++) {
        q1 = (orderp + i * (2 * l + 1) + l);
        for (j = 0; j < blist[i].n; j++) {
            if (blist[i].bnd[j].n > i) {
                q2 = (orderp + blist[i].bnd[j].n * (2 * l + 1) + l);
                pos_order(l, &(blist[i].bnd[j]), q1, q2);
            }
        }
    }
    //normalize vector
    for (i = 0; i < n_particles; i++) {
        temp = 1.0 / sqrt(dotprod(orderp + i * (2 * l + 1), orderp + i * (2 * l + 1), l));
        for (m = -l; m <= l; m++) {
            (*(orderp + i * (2 * l + 1) + m + l)).re *= temp;
            (*(orderp + i * (2 * l + 1) + m + l)).im *= temp;
        }
        //end
    }
    return orderp;
}

/************************************************
 *             CALC_CONN
 * Find crystalline bonds between particles
 ***********************************************/
int* calc_conn(compl_t* orderp) //calculates "connected" particles
{
    int i, j, z, np = 0;
    const int l = 4;
    int* conn = malloc(sizeof(int) * n_particles);
    for (i = 0; i < n_particles; i++) {
        z = 0;
        for (j = 0; j < blist[i].n; j++) {
            if (dotprod(orderp + i * (2 * l + 1), orderp + blist[i].bnd[j].n * (2 * l + 1), l) > bnd_cuttoff) {
                z++;
                //        printf ("Test %d, %d\n", i, particlestocount);
            }
        }
        if (z >= nbnd_cuttoff) {
            np++;
            conn[i] = 1;
        } else {
            conn[i] = 0;
        }
        numconn[i] = z;
    }
    return conn;
}

/************************************************
 *             SAVE_CLUSS
 * Output snapshot in which clusters are colored
 * Fluid particles printed small
 ***********************************************/
void save_cluss(int step, int* cluss, int* size, int big, int nn)
{
    int i;

    char buffer[128] = "datafolder/";
    strcat(buffer, datafolder_name);
    if (is_pos) {
        strcat(buffer, "/clust_pos_coords%07d.poly");
    } else {
        strcat(buffer, "/clust_orient_coords%07d.poly");
    }
    char fn[128] = "";
    sprintf(fn, buffer, step);
    char dfn[128] = "datafolder/";
    strcat(dfn, datafolder_name);
    strcat(dfn, "/npos");

    FILE* data_file;
    if ((data_file = fopen(dfn, "a")) == NULL) {
        printf("couldn't open clust_file dfn = %s\n", dfn);
        exit(-2);
    }
    fprintf(data_file, "%lf  %d  %d\n", percentage, numclus, maxsize);
    fclose(data_file);

    FILE* clust_file;
    if ((step % (100 * output_per)) == 0) {
        if ((clust_file = fopen(fn, "w")) == NULL) {
            printf("couldn't open clust_file fn = %s\n", fn);
            exit(-2);
        }
    } else {
        return;
    }
    printf("step %d, saving to file %s\n", step, fn);
    fprintf(clust_file, "%d\n0 0 0\n", n_particles);
    fprintf(clust_file, "%lf 0 0\n0 %lf 0\n0 0 %lf\n", box.x, box.y, box.z);

    // printf("defining sorta ");
    int* sorta = malloc(sizeof(int) * nn);
    // printf("and rank\n");
    int* rank = malloc(sizeof(int) * nn);
    // printf("looping over sorta\n");
    for (i = 0; i < nn; i++)
        sorta[i] = i;

    int cmpr(const void* a, const void* b)
    {
        return -size[*((int*)a)] + size[*((int*)b)];
    }

    // printf("sorting sorta[] if nn = %d >= 2\n", nn);
    // if (nn >= 2)
    //     qsort(sorta, nn, sizeof(int), &cmpr);
    // printf("sorting sorta[]\n");
    qsort(sorta, nn, sizeof(int), &cmpr);

    for (i = 0; i < nn; i++) {
        int bha;
        for (bha = 0; sorta[bha] != i; bha++)
            ;
        rank[i] = bha;
    }

    // printf("saving to files\n");
    for (i = 0; i < n_particles; i++) {
        int rnk = 0;
        if (cluss[i] && size[cluss[i]] > 2) {
            rnk = rank[cluss[i]];
            if (rnk > 0)
                rnk = ((rnk - 1) % 25) + 1;
        } else {
            part[i].d *= 0.1;
        }
        fprintf(clust_file, "%lf %lf %lf %lf ", part[i].x, part[i].y, part[i].z, part[i].d);
        for (int d1 = 0; d1 < NDIM; d1++) {
            for (int d2 = 0; d2 < NDIM; d2++) {
                fprintf(clust_file, "%lf ", ruud_m[i].m[d1][d2]);
            }
        }
        fprintf(clust_file, "10 %lf %d\n", Phi, rnk);
    }
    fclose(clust_file);
    free(sorta);
    free(rank);
}

/************************************************
 *             CALC_CLUSTERS
 * Find clusters of bonded particles
 ***********************************************/
void calc_clusters(int step, int* conn, compl_t* orderp)
{
    int* cluss = malloc(sizeof(int) * n_particles);
    int* size = malloc(sizeof(int) * n_particles);
    int i, cs, cn = 1, big = 0;
    // int bc = -1;
    const int l = 4;
    // printf("definition setcluss\n");
    void setcluss(int pn)
    {
        cluss[pn] = cn;
        int jj;
        for (jj = 0; jj < blist[pn].n; jj++) {
            int tmp = blist[pn].bnd[jj].n;
            if (conn[tmp] != 0 && cluss[tmp] == 0 && dotprod(orderp + pn * (2 * l + 1), orderp + tmp * (2 * l + 1), l) > obnd_cuttoff) {
                //0.9 gives nice results 0.6 gives all touching nuclei as one big nuclei
                cs++;
                setcluss(tmp);
            }
        }
    }

    // printf("initializing cluss[] and size[] to zero\n");
    for (i = 0; i < n_particles; i++) {
        cluss[i] = 0;
        size[i] = 0;
    }

    // printf("loop to setcluss(i) and initialize cn\n");
    for (i = 0; i < n_particles; i++) {
        cs = 0;
        if (conn[i] == 1 && cluss[i] == 0) {
            cs++; //has at least size 1;
            setcluss(i);
            size[cn] = cs;
            if (cs > big) {
                big = cs;
                // bc = cn;
            }
            cn++;
        }
    }

    // printf("calculating average cluster size\n");
    //calculate average cluster size
    int tcs = 0;
    for (i = 0; i < cn; i++) {
        tcs += size[i];
    }

    // printf("save_cluss\n");
    // if (dontsave == 0) {
    save_cluss(step, cluss, size, big, cn);
    // }
    percentage = tcs / (double)n_particles;
    maxsize = big;
    numclus = cn - 1;
    printf("% i clusters, Max size: %i, Percentage of crystalline particles %f\n", numclus, big, percentage);

    free(cluss);
    free(size);
}

/// This function reads the initial configuration of the system,
/// it reads data in the same format as it outputs data.
/// It also initializes SinPhi, CosPhi, Normal, ParticleVolume, r, and m (rot. mx.).
int read_data2(char* init_file, int is_not_first_time)
{
    FILE* pFile = fopen(init_file, "r");
    if (NULL == pFile) {
        printf("file not found: %s\n", init_file);
        return 1;
    }
    // read n_particles
    if (!fscanf(pFile, "%d", &n_particles)) {
        printf("failed to read num of particles\n");
        return 2;
    }
    if (n_particles < 1 || n_particles > N) {
        printf("num particles %d, go into code and change N (max nparticles)\n", n_particles);
        return 2;
    }
    double garbagef; // garbage (double) float

    // three zeroes which do nothing
    if (!fscanf(pFile, "%lf %lf %lf", &garbagef, &garbagef, &garbagef)) {
        printf("failed to read three zeroes\n");
        return 2;
    }

    for (int d = 0; d < 9; ++d) { // dimensions of box
        if (d % 4 == 0) {
            if (!fscanf(pFile, "%lf\t", &(ruud_box[d / 4]))) {
                printf("failed to read dimensions of box\n");
                return 2;
            }
        } else {
            if (!fscanf(pFile, "%lf\t", &garbagef)) {
                printf("failed to read dimensions of box\n");
                return 2;
            }
        }
    }

    // now read the particles
    bool rf = true; // read flag. if false, something went wrong
    for (int n = 0; n < n_particles; ++n) {
        // We use pointers to loop over the x, y, z members of the vec3_t type.
        double* pgarbage = &(ruud_r[n].x);
        for (int d = 0; d < NDIM; ++d) // the position of the center of cube
            rf = rf && fscanf(pFile, "%lf\t", (pgarbage + d));
        rf = rf && fscanf(pFile, "%lf\t", &garbagef); // Edge_Length == 1
        for (int d1 = 0; d1 < NDIM; d1++) {
            for (int d2 = 0; d2 < NDIM; d2++) {
                rf = rf && fscanf(pFile, "%lf\t", &(ruud_m[n].m[d1][d2]));
                // rf = rf && fscanf(pFile, "%lf\t", &garbagef);
            }
        }
        rf = rf && fscanf(pFile, "%lf", &garbagef);
        if (n == 0) {
            rf = rf && fscanf(pFile, "%lf\n", &Phi); // only read Phi once
        } else {
            if (EOF == fscanf(pFile, "%lf\n", &garbagef)) {
                printf("reached end of read file unexpectedly\n");
                return 2;
            }
        }
    }
    if (!rf) { // if read flag false, something went wrong.
        printf("failed to read (one of the) particles\n");
        return 2;
    }
    fclose(pFile);

    if (!is_not_first_time) { // only initialize these once
        SinPhi = sin(Phi);
        CosPhi = cos(Phi);
        ParticleVolume = SinPhi; // Edge_Length == 1

        // now initialize the normals, put everything to zero first:
        for (int i = 0; i < 3; i++) {
            Normal[i].x = Normal[i].y = Normal[i].z = 0;
        }
        Normal[0].x = SinPhi; // normal on x-dir
        Normal[0].z = -CosPhi;
        Normal[1].y = 1.; // normal on y-dir
        Normal[2].z = 1.; // normal on z-dir
    }

    return 0;
    // initialize_cell_list();
}

/// Put parsing the commandline in a function.
/// If something goes wrong, return != 0
int parse_commandline(int argc, char* argv[])
{
    if (argc != 4) {
        printf("need 3 arguments:\n");
        return 3;
    }
    if (EOF == sscanf(argv[1], "%s", datafolder_name)) {
        printf("reading Phi has failed\n");
        return 1;
    }
    if (EOF == sscanf(argv[2], "%d", &output_per)) {
        printf("reading output_per has failed\n");
        return 1;
    }
    if (output_per <= 0 || output_per > 100) {
        printf("0 < output_per < 101\n");
        return 2;
    }
    if (strcmp(argv[3], "p") == 0 || strcmp(argv[3], "pos") == 0 || strcmp(argv[3], "position") == 0) {
        printf("doing positionional ordering\n");
        is_pos = true;
        return 0;
    } else if (strcmp(argv[3], "o") == 0 || strcmp(argv[3], "orient") == 0 || strcmp(argv[3], "orientation") == 0) {
        printf("doing orientational ordering\n");
        is_pos = false;
        // printf("not implemented yet\n");
        // return -1;
        return 0;
    } else {
        printf("reading last argument failed\n");
        return 1;
    }

    return 0; // no exceptions, run the program
}

/************************************************
 *             MAIN
 ***********************************************/
int main(int argc, char** argv)
{
    // int f = 0, i = 0;
    compl_t* order = NULL;
    int* connections = NULL;

    if (parse_commandline(argc, argv)) { // if ... parsing fails
        printf(usage_string);
        return 1;
    };

    printf("datafolder_name: %s\n", datafolder_name);
    char buffer2[128] = "";
    if (is_pos) {
        strcat(buffer2, "/v11_pos");
    } else {
        strcat(buffer2, "/v11_orient");
    }
    remove(buffer2);
    char buffer[128] = "datafolder/";
    strcat(buffer, datafolder_name);
    strcat(buffer, "/coords_step%07d.poly");

    // int loop_count = 0;
    for (int step = 0; step <= 5000000; step += 100) {
        char datafile_name[128] = "";
        // replace all %d, %lf in buffer with values and put in datafile_name
        sprintf(datafile_name, buffer, step);
        // printf("datafile_name: %s\n", datafile_name);

        int status = read_data2(datafile_name, step);
        if (status == 1) {
            printf("last file number %d\n", step);
            return 0;
        } else if (status == 2) {
            printf("something wrong with filename %s\n", datafile_name);
            return 2;
        }

        convert_data();
        // printf("calculating order\n");
        if (is_pos) {
            order = calc_pos_order();
        } else {
            order = calc_orient_order();
        }
        // printf("calculating connections\n");
        connections = calc_conn(order);
        // printf("calculating clusters\n");
        calc_clusters(step, connections, order); // and save
        free(order);
        free(connections);
        free(part);
        free(cells);
        free(numconn);
        for (int i = 0; i < n_particles; i++)
            free(blist[i].bnd);
        free(blist);
    }
    printf("\n");

    return 0;
}