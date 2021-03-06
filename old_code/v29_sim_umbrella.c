#include "21.h"
#include "crystal_coords_v18_for_v29.c" // int biggest_cluster_size_and_order(int what_order) returns the largest cluster this step
#include "math_3d.h" // https://github.com/arkanis/single-header-file-c-libs/blob/master/math_3d.h
#include "mt19937.h" // Mersenne Twister (dsmft_genrand();)
#include <malloc.h>
#ifdef _WIN32
#include <direct.h> // mkdir on Windows
#elif __linux__
#include <sys/stat.h> // mkdir on Linux
#include <sys/types.h> // mkdir on Linux
#endif
#include <stdbool.h> // C requires this for (bool, true, false) to work
#include <string.h> // This is for C (strcpy, strcat, etc. ). For C++, use #include <string>
#include <time.h> // time(NULL)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

#define NBINS 200 // number of bins for g(r)

/* Initialization variables */
int mc_steps;
static double packing_fraction;
static double BetaP;
double Phi; // angle of slanted cube

const char labelstring[] = "v29_%02d_pf%04.2lf_p%04.1lf_a%04.2lf_%c_t%04d_c%5.3lf_%04.2lf";
// CubesPerDim, pack_frac, pressure, angle, order_mode, target cl.sz., npt/nvt, coupling_parameter
// if p == -1, it means NVT ensemble
const char usage_string[] = "\nusage: v29.exe r/c readf/#cube outputf/pf #steps #outsteps p/nvt phi order target_clsz lambda cutoff\n\
more detailed usage: (readfile / # cubes per dim) (output_folder / packing_fraction) mc_steps output_steps BetaP/NVT Phi order_mode \
target_cluster_size coupling_parameter order_cutoff\nhere order_mode: transl = 1, sl = 2, unsl = 3, edge = 4\n";
char output_folder[128] = "";

int output_steps = 100;
// const int g_of_r_cutoff = 5;
int counter_for_g;

/* Simulation variables */
system_t* sim;

static double Delta = 0.05; // delta, deltaV, deltaR are dynamic, i.e. every output_steps steps,
static double DeltaR = 0.05; // they will be nudged a bit to keep
static double DeltaV = 2.0; // the move and volume acceptance in between 0.4 and 0.6.

static bool IsCreated; // Did we create a system or read a file
static bool IsNVT; // do or do not do volume moves

static int CellsPerDim; // number of cells per dimension
vec3_t Normal[3]; // the normal vector of an unrotated cube. Normal[0] is the normal in the x-dir, etc.
int n_particles = 0;
int CubesPerDim;
double order_cutoff;
double ParticleVolume;
double CosPhi; // cos and sin of Phi appear a lot, and are expensive to calculate.
double SinPhi; // Since Phi doesn't change, it's faster to calculate only once.

// static double gof[NBINS];

int what_order = 0; // transl = 1, sl = 2, unsl = 3, edge = 4
char order_character;
int target_cluster_size = 0;
double coupling_parameter = 0.01;

/* Functions */
inline static double ran(double low, double high);
void scale(double scale_factor);

// initialization
int parse_commandline(int argc, char* argv[]);
void read_data2(char* init_file);
void create_system(void);
void set_packing_fraction(void);
void set_random_orientation(void);
void remove_overlap(void);
void remove_overlap_smart(void);
void initialize_phi_normal_offset(void);
void initialize_cell_list(void);
vec3_t random_cube_axis(void);

// mc steps
int move_particle_cell_list(void);
int rotate_particle(void);
int change_volume(void);
void nudge_deltas(double mov, double vol, double rot);
// void write_data(int step, FILE* fp_density, FILE* fp_g, char datafolder_name[128]);
void write_data(int step, FILE* fp_density, char datafolder_name[128]);
// void sample_g_of_r(void);

// collision detection
bool is_overlap_between(int i, int j);
bool is_separation_along_axis(vec3_t axis, int i, int j, vec3_t r2_r1);
bool is_separation_along_axis_fast1(vec3_t axis, int i, int j, vec3_t r2_r1, int which_axis);
bool is_separation_along_axis_fast2(vec3_t axis, int i, int j, vec3_t r2_r1, int which_axis);
inline vec3_t get_offset(int i, int j);
bool is_overlap_from(int index);
void update_cell_list(int index);
inline static void update_CellLength(void);
bool is_overlap(void);

/* Main */

int main(int argc, char* argv[])
{
    dsfmt_seed(time(NULL));

    // allocate room for two systems. The second system will be the old system
    // to jump back to, if the umbrella sampling potential rejects the new system.
    sim = (system_t*)malloc(2 * sizeof(system_t));

    if (parse_commandline(argc, argv)) { // if ... parsing fails
        printf(usage_string);
        return 1;
    };

    if (IsCreated) {
        set_packing_fraction();
        set_random_orientation(); // to stop pushing to rhombic phase
        remove_overlap_smart(); // due to too high a packing fraction or rotation
    } else {
        if (is_overlap()) {
            printf("\n\n\tWARNING\n\n\nThe read file contains overlap.\n");
            // remove_overlap();
            // return 2;
        } else {
            printf("No overlap detected, continuing simulation.\n");
        }
    }
    initialize_cell_list();

    sim->clust_size = biggest_cluster_size_and_order(1, NULL);
    int diff = biggest_cluster_size_and_order(1, NULL) - target_cluster_size;
    sim->energy = 0.5 * coupling_parameter * diff * diff;

    char datafolder_name[128] = "";
    char densityfile_name[128] = "";
    if (IsCreated) {
        char buffer_df[128] = "datafolder/";
        char buffer_rho[128] = "densities/";
        strcat(buffer_df, labelstring);
        strcat(buffer_rho, labelstring);
        // replace all %d, %lf in the buffers with values and put in density_filename
        sprintf(datafolder_name, buffer_df, CubesPerDim, packing_fraction, BetaP, Phi, order_character, target_cluster_size, coupling_parameter, order_cutoff);
        sprintf(densityfile_name, buffer_rho, CubesPerDim, packing_fraction, BetaP, Phi, order_character, target_cluster_size, coupling_parameter, order_cutoff);
    } else {
        sprintf(datafolder_name, "datafolder/%s", output_folder);
        sprintf(densityfile_name, "densities/%s", output_folder);
    }

    // char gofrfile_name[128] = "";
    // strcat(gofrfile_name, datafolder_name);
    // strcat(gofrfile_name, "/g.txt");
    char largest_clusterfile_name[128] = "";
    strcat(largest_clusterfile_name, datafolder_name);
    strcat(largest_clusterfile_name, "/clsz.txt");
    char orderfile_name[128] = "";
    strcat(orderfile_name, datafolder_name);
    strcat(orderfile_name, "/order.txt");

    // printf("\nsaving to:\n%s\n%s\n%s\n%s\n%s\n\n",
    //     datafolder_name, densityfile_name, gofrfile_name, largest_clusterfile_name, orderfile_name);
    printf("\nsaving to:\n%s\n%s\n%s\n%s\n\n",
        datafolder_name, densityfile_name, largest_clusterfile_name, orderfile_name);

// make the folder to store all the data in, if it already exists do nothing.
#ifdef _WIN32
    mkdir("datafolder");
    mkdir(datafolder_name);
    mkdir("densities");
#elif __linux__
    // Linux needs me to set rights, this gives rwx to me and just r to all others.
    mkdir("datafolder", S_IRWXU | S_IRGRP | S_IROTH);
    mkdir(datafolder_name, S_IRWXU | S_IRGRP | S_IROTH);
    mkdir("densities", S_IRWXU | S_IRGRP | S_IROTH);
#elif
    printf("Please use Linux or Windows\n");
    exit(3);
#endif

    FILE* fp_density = fopen(densityfile_name, "w");
    // FILE* fp_g = fopen(gofrfile_name, "w");
    FILE* fp_clsz = fopen(largest_clusterfile_name, "w");
    FILE* fp_order = fopen(orderfile_name, "w");
    if (!fp_density) {
        printf("couldnt open %s, exiting\n", densityfile_name);
        exit(8);
    }
    // if (!fp_g) {
    //     printf("couldnt open %s, exiting\n", gofrfile_name);
    //     exit(8);
    // }
    if (!fp_clsz) {
        printf("couldnt open %s, exiting\n", largest_clusterfile_name);
        exit(8);
    }
    if (!fp_order) {
        printf("couldnt open %s, exiting\n", orderfile_name);
        exit(8);
    }


    int mov_accepted = 0, vol_accepted = 0, rot_accepted = 0;
    int mov_attempted = 0, vol_attempted = 0, rot_attempted = 0;
    int umbr_accepted = 0;

    // the way I implement nvt/npt simulations
    const int MAX_RAN = IsNVT ? 2 * n_particles : 2 * n_particles + 2;

    system_t* previous_step = sim + 1;
    copy_system(previous_step, sim); // copy to previous_step: sim

    printf("first we let the system equilibrate\n");
    printf("step\tdensity\t  acceptances\t\t\t  deltas\t\t  clsz\tE diff\tsz diff\n");

    for (int step = 0; step <= 5000; step++) { // 5000 equilibration sweeps
        for (int n = 0; n < MAX_RAN; ++n) {
            // Have to randomize order of moves to obey detailed balance
            int temp_ran = (int)ran(0, MAX_RAN);
            if (temp_ran < n_particles) {
                mov_attempted++;
                mov_accepted += move_particle_cell_list();
            } else if (temp_ran < 2 * n_particles) {
                rot_attempted++;
                rot_accepted += rotate_particle();
            } else {
                vol_attempted++;
                vol_accepted += change_volume();
            }
        }

        // we do an umbrella sampling step every two MC sweeps
        int diff = 0;
        double new_energy = 0;
        if (step % 2 == 0) {
            // difference in cluster size
            diff = biggest_cluster_size_and_order(1, NULL) - target_cluster_size;
            // bias potential energy
            new_energy = 0.5 * coupling_parameter * diff * diff;
            double boltzmann = exp(sim->energy - new_energy); // beta absorbed in coupling_parameter

            // if (step % output_steps == 1) { //diff + target_cluster_size; = clust_size
            //     printf("%d\t%7.2lf\t%d", diff + target_cluster_size, new_energy - sim->energy, diff);
            // }

            if (ran(0, 1) < boltzmann) { // if move is accepted
                // printf(" v");
                copy_system(previous_step, sim); // new previous step
                sim->clust_size = diff + target_cluster_size; // == biggest_cluster_size(what_order);
                sim->energy = new_energy;
                umbr_accepted++;
            } else {
                copy_system(sim, previous_step); // go back
            }
        }

        if (step % 50 == 0) {
            double move_acceptance = (double)mov_accepted / mov_attempted;
            double rotation_acceptance = (double)rot_accepted / rot_attempted;
            double volume_acceptance = (double)vol_accepted / vol_attempted;
            double umbrella_acceptance = (double)umbr_accepted / (50. / 2.);

            // Here is where delta, deltaR, deltaV might get changed if necessary
            nudge_deltas(move_acceptance, volume_acceptance, rotation_acceptance);
            // And reset for the next loop
            mov_attempted = rot_attempted = vol_attempted = 0;
            mov_accepted = rot_accepted = vol_accepted = 0;
            umbr_accepted = 0;

            printf("%d\t%.3lf\t  %.3lf\t%.3lf\t%.3lf\t%.3lf\t  %.3lf\t%.3lf\t%.3lf\t  %d\t%-7.3lf\t%d\n",
                step,
                n_particles * ParticleVolume / (sim->box[0] * sim->box[1] * sim->box[2]),

                move_acceptance,
                rotation_acceptance,
                volume_acceptance,
                umbrella_acceptance,

                Delta, DeltaR, DeltaV,

                diff + target_cluster_size, // = cluster_size
                new_energy - sim->energy,
                diff);
        }
    }

    printf("\nEnd of equilibration, start of simulation\n\n");

    printf("step\tdensity\t  acceptances\t\t\t  clsz\tE diff\tsz diff\n");
    for (int step = 0; step <= mc_steps; ++step) {
        // if (step % (50 * output_steps) == 0) {
        //     printf("step\tdensity\t  acceptances\t\t  deltas\t\t  clsize\tE diff\tsz diff\n");
        //     printf("\n#step\tclsize\tE diff\tsz diff");
        // }

        if (step % output_steps == 0) {
            // printf("\n%d\t", step);
            // sample_g_of_r();
            // write_data(step, fp_density, fp_g, datafolder_name);
            write_data(step, fp_density, datafolder_name);
        }

        for (int n = 0; n < MAX_RAN; ++n) {
            // Have to randomize order of moves to obey detailed balance
            int temp_ran = (int)ran(0, MAX_RAN);
            if (temp_ran < n_particles) {
                mov_attempted++;
                mov_accepted += move_particle_cell_list();
            } else if (temp_ran < 2 * n_particles) {
                rot_attempted++;
                rot_accepted += rotate_particle();
            } else {
                vol_attempted++;
                vol_accepted += change_volume();
            }
        }

        // we do an umbrella sampling step every two MC sweeps
        int diff = 0;
        double new_energy = 0;
        if (step % 2 == 0) {
            // difference in cluster size
            diff = biggest_cluster_size_and_order(step, fp_order) - target_cluster_size;
            // bias potential energy
            new_energy = 0.5 * coupling_parameter * diff * diff;
            double boltzmann = exp(sim->energy - new_energy); // beta absorbed in coupling_parameter

            // if (step % output_steps == 1) { //diff + target_cluster_size; = clust_size
            //     printf("%d\t%7.2lf\t%d", diff + target_cluster_size, new_energy - sim->energy, diff);
            // }

            if (ran(0, 1) < boltzmann) { // if move is accepted
                // printf(" v");
                copy_system(previous_step, sim); // new previous step
                sim->clust_size = diff + target_cluster_size; // == biggest_cluster_size(what_order);
                sim->energy = new_energy;
                umbr_accepted++;
            } else {
                copy_system(sim, previous_step); // go back
            }
        }

        if (step % 50 == 0) {
            fprintf(fp_clsz, "%d\n", sim->clust_size);
            if (step % 10000 == 0) {
                fflush(fp_density);
                // fflush(fp_g);
                fflush(fp_clsz);
                fflush(fp_order);
            }
        }

        if (step % output_steps == 0) {
            double move_acceptance = (double)mov_accepted / mov_attempted;
            double rotation_acceptance = (double)rot_accepted / rot_attempted;
            double volume_acceptance = (double)vol_accepted / vol_attempted;
            double umbrella_acceptance = (double)umbr_accepted * 2. / output_steps;

            // And reset for the next loop
            mov_attempted = rot_attempted = vol_attempted = 0;
            mov_accepted = rot_accepted = vol_accepted = 0;
            umbr_accepted = 0;
            if (step % 10000 == 0) {
                if (is_overlap()) {
                    printf("Found overlap in this step!\n");
                }
            }
            printf("%d\t%.3lf\t  %.3lf\t%.3lf\t%.3lf\t%.3lf\t  %d\t%-7.3lf\t%d\n",
                step,
                n_particles * ParticleVolume / (sim->box[0] * sim->box[1] * sim->box[2]),

                move_acceptance,
                rotation_acceptance,
                volume_acceptance,
                umbrella_acceptance,

                diff + target_cluster_size, // = cluster_size
                new_energy - sim->energy,
                diff);
        }
    }

    fclose(fp_density); // densities/...
    // fclose(fp_g); // datafolder/.../g.txt
    fclose(fp_clsz); // datafolder/.../clsz.txt
    fclose(fp_order); // datafolder/.../order.txt
    free(sim);

    return 0;
}

/* Functions implementation */

/// Returns a random number between low and high (including exactly low, but not exactly high).
inline static double ran(double low, double high)
{
    return (high - low) * dsfmt_genrand() + low;
}

/// Scales the system with the scale factor
void scale(double scale_factor)
{
    for (int n = 0; n < n_particles; ++n) {
        // We use pointers to loop over the x, y, z members of the vec3_t type.
        for (int d = 0; d < NDIM; ++d)
            *(&(sim->r[n].x) + d) *= scale_factor;
    }
    for (int d = 0; d < NDIM; ++d)
        sim->box[d] *= scale_factor;
}

/// This function returns the offset of the jth vertex (j = 0, ... , 7)
/// from the center of cube number i:     3----7
/// (y points into the screen)           /|   /|
///     z                               1-+--5 |
///     | y                             | 2--+-6
///     |/                              |/   |/
///     0----x                          0----4
/// The angle Phi is the angle ∠104 in the picture above, i.e. the angle of
/// "the z-axis of the cube" with "the x-axis of the cube"
inline vec3_t get_offset(int i, int j)
{
    return m4_mul_dir(sim->m[i], sim->offsets[j]);
}

/// Checks if there is separation between cubes along axis, between cubes i, j.
/// Also the difference vector r2-r1 is given as it has already been calculated
bool is_separation_along_axis(vec3_t axis, int i, int j, vec3_t r2_r1)
{
    // The axis doesn't need to be normalized, source:
    // https://gamedevelopment.tutsplus.com/tutorials/collision-detection-using-the-separating-axis-theorem--gamedev-169)
    // axis = v3_norm(axis);

    double min1, min2, max1, max2, temp;
    min1 = max1 = v3_dot(axis, v3_add(r2_r1, get_offset(i, 0)));
    min2 = max2 = v3_dot(axis, get_offset(j, 0));
    for (int n = 1; n < 8; n++) {
        temp = v3_dot(axis, v3_add(r2_r1, get_offset(i, n)));
        min1 = fmin(min1, temp);
        max1 = fmax(max1, temp);
        temp = v3_dot(axis, get_offset(j, n));
        min2 = fmin(min2, temp);
        max2 = fmax(max2, temp);
    }

    if (max1 < min2 || max2 < min1) {
        return true; // separation!
    } else {
        return false; // collision
    }
}

/// same as above, but when we check normals,
/// we only need to check two (or three) points of the first or second cube
/// if the axis in question is not the y-axis, we also need vertex number 5
/// don't worry don't touch it works exactly the same as the "dumb" algorithm
/// for a 12^3 system for 1000 MC sweeps
bool is_separation_along_axis_fast1(vec3_t axis, int i, int j, vec3_t r2_r1, int which_axis)
{
    double min1, min2, max1, max2, temp;
    min1 = max1 = v3_dot(axis, v3_add(r2_r1, get_offset(i, 0)));
    min2 = max2 = v3_dot(axis, get_offset(j, 0));

    temp = v3_dot(axis, v3_add(r2_r1, get_offset(i, which_axis)));
    min1 = fmin(min1, temp);
    max1 = fmax(max1, temp);
    if (which_axis != 2) { // special case for slanted cubes
        temp = v3_dot(axis, v3_add(r2_r1, get_offset(i, 5)));
        min1 = fmin(min1, temp);
        max1 = fmax(max1, temp);
    }
    for (int n = 1; n < 8; n++) {
        temp = v3_dot(axis, get_offset(j, n));
        min2 = fmin(min2, temp);
        max2 = fmax(max2, temp);
    }

    if (max1 < min2 || max2 < min1) {
        return true; // separation!
    } else {
        return false; // collision
    }
}

/// same as above but now the axis is from the second cube
bool is_separation_along_axis_fast2(vec3_t axis, int i, int j, vec3_t r2_r1, int which_axis)
{
    double min1, min2, max1, max2, temp;
    min1 = max1 = v3_dot(axis, v3_add(r2_r1, get_offset(i, 0)));
    min2 = max2 = v3_dot(axis, get_offset(j, 0));

    temp = v3_dot(axis, get_offset(j, which_axis));
    min2 = fmin(min2, temp);
    max2 = fmax(max2, temp);
    if (which_axis != 2) { // special case for slanted cubes
        temp = v3_dot(axis, get_offset(j, 5));
        min2 = fmin(min2, temp);
        max2 = fmax(max2, temp);
    }
    for (int n = 1; n < 8; n++) {
        temp = v3_dot(axis, v3_add(r2_r1, get_offset(i, n)));
        min1 = fmin(min1, temp);
        max1 = fmax(max1, temp);
    }

    if (max1 < min2 || max2 < min1) {
        return true; // separation!
    } else {
        return false; // collision
    }
}

/// Checks if cube i and cube j overlap using the separating axis theorem
bool is_overlap_between(int i, int j)
{
    // the next line shouldn't be necessary!
    // if (i == j) return false; // don't check on overlap with yourself!
    vec3_t r2_r1 = v3_sub(sim->r[i], sim->r[j]); // read as r2 - r1

    // We need to apply nearest image convention to r2_r1.
    // We use pointers to loop over the x, y, z members of the vec3_t type.
    for (int d = 0; d < NDIM; d++) {
        if (*(&(r2_r1.x) + d) > 0.5 * sim->box[d])
            *(&(r2_r1.x) + d) -= sim->box[d];
        if (*(&(r2_r1.x) + d) < -0.5 * sim->box[d])
            *(&(r2_r1.x) + d) += sim->box[d];
    }

    // If the cubes are more than their circumscribed sphere apart, they couldn't possibly overlap.
    // Similarly, if they are less than their inscribed sphere apart, they couldn't possibly NOT overlap.
    double len2 = v3_dot(r2_r1, r2_r1); // sqrtf is slow so test length^2
    if (len2 > (3 + 2 * CosPhi)) // * Edge_Length * Edge_Length) // Edge_Length == 1
        return false;
    if (len2 < SinPhi * SinPhi)
        return true;

    // Now we use the separating axis theorem. Check for separation along all normals
    // and crossproducts between edges of the cubes. Only if along all these axes
    // we find no separation, we may conclude there is overlap.
    vec3_t axes[6 + 9]; // 6 normals of r1 and r2, 9 cross products between edges
    for (int k = 0; k < 3; k++) {
        axes[k] = m4_mul_dir(sim->m[i], Normal[k]);
        axes[k + 3] = m4_mul_dir(sim->m[j], Normal[k]);
    }

    // Now load the cross products between edges
    vec3_t edges1[3], edges2[3];
    edges1[0] = v3_sub(get_offset(i, 0), get_offset(i, 1)); // z
    edges1[1] = v3_sub(get_offset(i, 0), get_offset(i, 2)); // y
    edges1[2] = v3_sub(get_offset(i, 0), get_offset(i, 4)); // x
    edges2[0] = v3_sub(get_offset(j, 0), get_offset(j, 1));
    edges2[1] = v3_sub(get_offset(j, 0), get_offset(j, 2));
    edges2[2] = v3_sub(get_offset(j, 0), get_offset(j, 4));

    for (int k = 0; k < 9; k++) {
        axes[k + 6] = v3_cross(edges1[k / 3], edges2[k % 3]);
    }

    if (is_separation_along_axis_fast1(axes[0], i, j, r2_r1, 1)
        || is_separation_along_axis_fast1(axes[1], i, j, r2_r1, 2)
        || is_separation_along_axis_fast1(axes[2], i, j, r2_r1, 4)
        || is_separation_along_axis_fast2(axes[3], i, j, r2_r1, 1)
        || is_separation_along_axis_fast2(axes[4], i, j, r2_r1, 2)
        || is_separation_along_axis_fast2(axes[5], i, j, r2_r1, 4))
        return false; // found separation, no overlap!
    for (int k = 6; k < 15; k++)
        if (is_separation_along_axis(axes[k], i, j, r2_r1))
            return false; // found separation, no overlap!

    // overlap on all axes ==> the cubes overlap
    return true;
}

/// This function checks if the move- and volume-acceptances are too high or low,
/// and adjusts delta and deltaV accordingly.
void nudge_deltas(double mov, double vol, double rot)
{
    if (mov < 0.3 && Delta > 0.0001)
        Delta *= 0.9; // acceptance too low  --> decrease delta
    if (mov > 0.4 && Delta < 0.5) // Edge_Length == 1
        Delta *= 1.1; // acceptance too high --> increase delta
    if (vol < 0.1 && DeltaV > 0.001)
        DeltaV *= 0.9;
    if (vol > 0.2 && DeltaV < 1000)
        DeltaV *= 1.1;
    if (rot < 0.3 && DeltaR > 0.0001)
        DeltaR *= 0.9;
    if (rot > 0.4 && DeltaR < M_PI)
        DeltaR *= 1.1;
}

/// This function attempts to change the volume, returning 1 if succesful and 0 if not.
int change_volume(void)
{
    double dV = ran(-DeltaV, DeltaV);
    double oldvol = 1;
    for (int d = 0; d < NDIM; d++)
        oldvol *= sim->box[d];

    double newvol = oldvol + dV;
    double scale_factor = pow(newvol / oldvol, 1. / NDIM); // the . of 1. is important, otherwise 1 / NDIM == 1 / 3 == 0

    // change the configuration
    scale(scale_factor);

    // now we need to check for overlaps (only if scale_factor < 1), and reject if there are any
    bool is_collision = false;
    if (scale_factor < 1.0) {
        for (int i = 0; i < n_particles; i++) {
            if (is_overlap_from(i)) {
                is_collision = true;
                break;
            }
        }
    }

    if (is_collision) {
        scale(1. / scale_factor); // move everything back
        return 0; // unsuccesful change
    }

    // Now that there are no collisions, we need to accept the change at a rate like the boltzmann factor.
    // Effectively the above code simulates a hard sphere potential (U = \infty if r < d, U = 0 else)
    double boltzmann = pow(M_E, -BetaP * dV) * pow(newvol / oldvol, n_particles);
    if (ran(0, 1) < boltzmann) { // if boltzmann > 1, ΔE < 0, and this move will always be accepted.
        update_CellLength();
        return 1; // succesful change
    } else {
        scale(1. / scale_factor); // move everything back
        return 0; // unsuccesful change
    }
}

/// This function reads the initial configuration of the system,
/// it reads data in the same format as it outputs data.
/// It also initializes SinPhi, CosPhi, Normal, ParticleVolume, r, and m (rot. mx.).
void read_data2(char* init_file)
{
    FILE* pFile = fopen(init_file, "r");
    if (NULL == pFile) {
        printf("file not found: %s\n", init_file);
        exit(1);
    }
    // read n_particles
    if (!fscanf(pFile, "%d", &n_particles)) {
        printf("failed to read num of particles\n");
        exit(2);
    }
    if (n_particles < 1 || n_particles > N) {
        printf("num particles %d, go into code and change N (max nparticles)\n", n_particles);
        exit(2);
    }
    double garbagef; // garbage (double) float

    // three zeroes which do nothing
    if (!fscanf(pFile, "%lf %lf %lf", &garbagef, &garbagef, &garbagef)) {
        printf("failed to read three zeroes\n");
        exit(2);
    }

    for (int d = 0; d < 9; ++d) { // dimensions of box
        if (d % 4 == 0) {
            if (!fscanf(pFile, "%lf\t", &(sim->box[d / 4]))) {
                printf("failed to read dimensions of box\n");
                exit(2);
            }
        } else {
            if (!fscanf(pFile, "%lf\t", &garbagef)) {
                printf("failed to read dimensions of box\n");
                exit(2);
            }
        }
    }

    // now read the particles
    bool rf = true; // read flag. if false, something went wrong
    for (int n = 0; n < n_particles; ++n) {
        // We use pointers to loop over the x, y, z members of the vec3_t type.
        for (int d = 0; d < NDIM; ++d) // the position of the center of cube
            rf = rf && fscanf(pFile, "%lf\t", (&(sim->r[n].x) + d));
        rf = rf && fscanf(pFile, "%lf\t", &garbagef); // Edge_Length == 1
        for (int d1 = 0; d1 < NDIM; d1++) {
            for (int d2 = 0; d2 < NDIM; d2++) {
                rf = rf && fscanf(pFile, "%lf\t", &(sim->m[n].m[d1][d2]));
            }
        }
        rf = rf && fscanf(pFile, "%lf", &garbagef);
        if (n == 0) {
            rf = rf && fscanf(pFile, "%lf\n", &Phi); // only read Phi once
        } else {
            if (EOF == fscanf(pFile, "%lf\n", &garbagef)) {
                printf("reached end of read file unexpectedly\n");
                exit(2);
            }
        }
    }
    if (!rf) { // if read flag false, something went wrong.
        printf("failed to read (one of the) particles\n");
        exit(2);
    }
    fclose(pFile);

    initialize_phi_normal_offset();
}

/// This function creates the initial configuration of the system.
/// It also initializes SinPhi, CosPhi, Normal, ParticleVolume, r, and m (rot. mx.).
void create_system(void)
{
    // initialize n_particles
    n_particles = CubesPerDim * CubesPerDim * CubesPerDim;
    if (n_particles > N) {
        printf("num particles too large, go into code and change N (max nparticles)\n");
        exit(2);
    }

    // initialize box
    for (int d = 0; d < NDIM; d++) {
        sim->box[d] = CubesPerDim; // this will be changed in set_packingfraction
    }

    // initialize the particle positions (r) on a simple cubic lattice
    {
        int index = 0;
        for (int i = 0; i < CubesPerDim; i++) {
            for (int j = 0; j < CubesPerDim; j++) {
                for (int k = 0; k < CubesPerDim; k++) {
                    sim->r[index++] = vec3(i + 0.5, j + 0.5, k + 0.5);
                }
            }
        }
    }

    initialize_phi_normal_offset();

    // now initialize the rotation matrices
    for (int i = 0; i < n_particles; i++) {
        for (int temp = 0; temp < 16; temp++)
            sim->m[i].m[temp % 4][temp / 4] = 0; // everything zero first
        for (int d = 0; d < NDIM; d++) {
            sim->m[i].m[d][d] = 1; // 1 on the diagonal
        }
    }
}

/// initializes a couple of things, made into own function to avoid duplicate code:
/// SinPhi, CosPhi, ParticleVolume, Normal[3], sim->offsets[8]
void initialize_phi_normal_offset(void)
{
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

    // offsets are calculated only once because the only thing that changes is the
    // orientation of the cubes. Thus no need to calculate these every time, so do it here
    for (int i = 0; i < 8; i++) {
        vec3_t offset = vec3(-0.5 * (1 + CosPhi), -0.5, -0.5 * SinPhi);
        if (i & 4) //   x+ = 4, 5, 6, 7
            offset.x += 1;
        if (i & 2) //   y+ = 2, 3, 6, 7
            offset.y += 1;
        if (i & 1) { // z+ = 1, 3, 5, 7
            offset.z += SinPhi;
            offset.x += CosPhi;
        }
        sim->offsets[i] = offset;
    }
}

/// Initializes CellsPerDim, CellLength,
/// NumCubesInCell[][][], WhichCubesInCell[][][][] and InWhichCellIsThisCube[]
void initialize_cell_list(void)
{
    // first initialize the lists to zero
    for (int i = 0; i < NC; i++)
        for (int j = 0; j < NC; j++)
            for (int k = 0; k < NC; k++) {
                sim->NumCubesInCell[i][j][k] = 0;
                for (int l = 0; l < MAXPC; l++) {
                    sim->WhichCubesInCell[i][j][k][l] = -1;
                }
            }

    // the minimum cell length is the diameter of circumscribed sphere
    double min_cell_length = sqrt(3 + 2 * CosPhi);
    // the minimum box size is (the volume of a maximally packed box)^(1/3)
    double min_box_size = pow(n_particles * ParticleVolume, 1. / 3.);

    // CellsPerDim must not be too large, so that
    // box[0] / CellsPerDim = CellLength >= min_cell_length
    // therefore, rounding CellsPerDim down is okay.
    CellsPerDim = (int)(min_box_size / min_cell_length);
    // NC is the maximum number of cells
    CellsPerDim = (CellsPerDim < NC) ? CellsPerDim : NC;

    update_CellLength();
    // Update CellLength every volume change, so that
    // all particles stay in the same cell when the system is scaled

    // now assign each cube to the cell they are in,
    // and count how many cubes are in each cell.
    for (int i = 0; i < n_particles; i++) {
        int x = sim->r[i].x / sim->CellLength;
        int y = sim->r[i].y / sim->CellLength;
        int z = sim->r[i].z / sim->CellLength;
        // add particle i to WhichCubesInCell at the end of the list
        // and add one to the counter of cubes of this cell (hence the ++)
        sim->WhichCubesInCell[x][y][z][(sim->NumCubesInCell[x][y][z])++] = i;
        // and keep track of in which cell this cube is
        sim->InWhichCellIsThisCube[i] = NC * NC * x + NC * y + z;
    }
}

/// Must be called every succesful volume change, and during initialization
inline static void update_CellLength(void)
{
    sim->CellLength = sim->box[0] / CellsPerDim;
}

/// This function returns if cube number index overlaps, using cell lists
bool is_overlap_from(int index)
{
    bool is_collision = false;
    int cell = sim->InWhichCellIsThisCube[index];
    // convert cell number to x, y, z coordinates
    int x = cell / (NC * NC);
    int y = (cell / NC) % NC;
    int z = cell % NC;
    // loop over all neighbouring cells
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            for (int k = -1; k < 2; k++) {
                // now loop over all cubes in this cell, remember periodic boundary conditions
                int loop_x = (x == 0 || x == CellsPerDim - 1) ? (x + i + CellsPerDim) % CellsPerDim : x + i;
                int loop_y = (y == 0 || y == CellsPerDim - 1) ? (y + j + CellsPerDim) % CellsPerDim : y + j;
                int loop_z = (z == 0 || z == CellsPerDim - 1) ? (z + k + CellsPerDim) % CellsPerDim : z + k;
                int num_cubes = sim->NumCubesInCell[loop_x][loop_y][loop_z];
                for (int cube = 0; cube < num_cubes; cube++) {
                    int index2 = sim->WhichCubesInCell[loop_x][loop_y][loop_z][cube];
                    // if checking your own cell, do not check overlap with yourself
                    if (index == index2) {
                        continue;
                    }

                    if (is_overlap_between(index, index2)) {
                        is_collision = true;
                        // and break out of all loops
                        cube = N;
                        i = j = k = 2;
                    }
                }
            }
        }
    }
    return is_collision;
}

/// This moves a random particle in a cube of volume (2 * delta)^3.
/// Note this gives particles a tendency to move to one of the 8 corners of the cube,
/// however it obeys detailed balance.^{[citation needed]}
/// It checks for overlap using the cell lists.
/// returns 1 on succesful move, 0 on unsuccesful move.
int move_particle_cell_list(void)
{
    // first choose the cube and remember its position
    int index = (int)ran(0., n_particles); // goes from 0 to n_particles - 1
    vec3_t r_old = sim->r[index];

    // move the cube
    // We use pointers to loop over the x, y, z members of the vec3_t type.
    for (int d = 0; d < NDIM; d++) {
        *(&(sim->r[index].x) + d) += ran(-Delta, Delta);
        // periodic boundary conditions happen here. Since delta < box[dim],
        // the following expression will always return a positive number.
        *(&(sim->r[index].x) + d) = fmodf(*(&(sim->r[index].x) + d) + sim->box[d], sim->box[d]);
    }

    update_cell_list(index);

    // and check for overlaps
    if (is_overlap_from(index)) {
        sim->r[index] = r_old; // move back
        update_cell_list(index); // and re-update the cell list.
        // inefficient but < 5% speed gain at a massive cost in readability if this is improved, see vp27_sim.c or vp28_sim.c
        return 0; // unsuccesful move
    } else {
        // remember to update (Num/Which)CubesInCell and InWhichCellIsThisCube
        return 1; // succesful move
    }
}

/// This function updates the cell list. At this point, r[index] contains the new
/// (accepted) position, and we need to check if is in the same cell as before.
/// If not, update (Num/Which)CubesInCell and InWhichCellIsThisCube.
void update_cell_list(int index)
{
    int cell_old = sim->InWhichCellIsThisCube[index];
    int x_old = cell_old / (NC * NC);
    int y_old = (cell_old / NC) % NC;
    int z_old = cell_old % NC;
    vec3_t r_new = sim->r[index];
    int x_new = r_new.x / sim->CellLength;
    int y_new = r_new.y / sim->CellLength;
    int z_new = r_new.z / sim->CellLength;
    if (x_new == CellsPerDim || y_new == CellsPerDim || z_new == CellsPerDim) {
        // printf(" !");
        // exit(6); // yeah so this actually happens for floats.
        // and apparently it happens enough for doubles as well
        x_new = x_new % CellsPerDim;
        y_new = y_new % CellsPerDim;
        z_new = z_new % CellsPerDim;
    }
    if (x_old == x_new && y_old == y_new && z_old == z_new) {
        return; // still in same box, don't have to change anything
    } else {
        // update in which cell this cube is
        sim->InWhichCellIsThisCube[index] = NC * NC * x_new + NC * y_new + z_new;
        // update WhichCubesInCell, first check at what index the moved cube was
        int cube = 0;
        while (index != sim->WhichCubesInCell[x_old][y_old][z_old][cube]) {
            cube++;
            if (cube >= MAXPC) {
                printf("infinite loop in update_cell_list\n");
                exit(5); // not so infinite anymore eh
            }
        }
        // now cube contains the index in the cell list

        // add the cube to the new cell and add one to the counter (hence ++)
        sim->WhichCubesInCell[x_new][y_new][z_new][(sim->NumCubesInCell[x_new][y_new][z_new])++] = index;
        // and remove the cube in the old cell by replacing it with the last
        // in the list and remove one from the counter (hence --)
        int last_in_list = sim->WhichCubesInCell[x_old][y_old][z_old][--(sim->NumCubesInCell[x_old][y_old][z_old])];
        sim->WhichCubesInCell[x_old][y_old][z_old][cube] = last_in_list;
        return;
    }
}

/// This rotates a random particle around a random axis
/// by a random angle \in [-DeltaR, DeltaR]
/// returns 1 on succesful rotation, 0 on unsuccesful rotation.
int rotate_particle(void)
{
    // first choose a random particle and remember its position
    int index = (int)ran(0., n_particles);

    // then choose a random axis (by picking points randomly in a ball)
    double x[3], dist;
    do {
        for (int i = 0; i < 3; i++)
            x[i] = ran(-1, 1);
        dist = x[0] * x[0] + x[1] * x[1] + x[2] * x[2];
    } while (dist > 1 || dist == 0); // dist == 0 doesn't seem to happen
    vec3_t rot_axis = vec3(x[0], x[1], x[2]); // the axis doesn't need to be normalized
    mat4_t rot_mx = m4_rotation(ran(-DeltaR, DeltaR), rot_axis);

    // rotate the particle
    sim->m[index] = m4_mul(rot_mx, sim->m[index]);

    // and check for overlaps
    if (is_overlap_from(index)) {
        // overlap, rotate back
        sim->m[index] = m4_mul(m4_invert_affine(rot_mx), sim->m[index]);
        return 0; // unsuccesful rotation
    } else {
        // no overlap, rotation is accepted!
        // since the cube didn't move, there is no need to update the cell lists
        return 1; // succesful rotation
    }
}

void write_data(int step, FILE* fp_density, char datafolder_name[128])
{
    if (fp_density) {
        fprintf(fp_density, "%lf\n", n_particles * ParticleVolume / (sim->box[0] * sim->box[1] * sim->box[2]));
        // fflush(fp_density); // write the densities everytime we have one, otherwise it waits for ~400 lines
    }

    // if (fp_g) {
    //     fprintf(fp_g, "%d ", counter_for_g);
    //     for (int i = 0; i < NBINS; i++) {
    //         fprintf(fp_g, "%lf ", gof[i]);
    //     }
    //     fprintf(fp_g, "\n");
    //     // fflush(fp_g); // blablabla same text as 9 lines up // I'm going to regret this line aren't I
    // }

    char buffer[128];
    strcpy(buffer, datafolder_name);
    strcat(buffer, "/coords_step%07d.poly");

    char datafile[128];
    sprintf(datafile, buffer, step); // replace %07d with step and put in output_file.

    FILE* fp = fopen(datafile, "w");
    fprintf(fp, "%d\n", n_particles);
    fprintf(fp, "0.0\t0.0\t0.0\n");
    for (int d = 0; d < 9; ++d) { // dimensions of box
        if (d % 4 == 0) {
            fprintf(fp, "%lf\t", sim->box[d / 4]);
        } else {
            fprintf(fp, "0.000000\t");
        }
        if (d % 3 == 2)
            fprintf(fp, "\n");
    }
    for (int n = 0; n < n_particles; ++n) {
        // We use pointers to loop over the x, y, z members of the vec3_t type.
        for (int d = 0; d < NDIM; ++d)
            fprintf(fp, "%lf\t", *(&(sim->r[n].x) + d)); // the position of the center of cube
        fprintf(fp, "1\t"); // Edge_Length == 1
        for (int d1 = 0; d1 < NDIM; d1++) {
            for (int d2 = 0; d2 < NDIM; d2++) {
                fprintf(fp, "%lf\t", sim->m[n].m[d1][d2]);
            }
        }
        fprintf(fp, "10 %lf\n", Phi); // 10 is for slanted cubes.
    }
    fclose(fp);
}

/* void sample_g_of_r(void)
{
    counter_for_g = 0;
    for (int i = 0; i < NBINS; i++) {
        gof[i] = 0;
    }

    for (int i = 0; i < n_particles; i++) {
        for (int j = i + 1; j < n_particles; j++) {
            vec3_t r2_r1 = v3_sub(sim->r[i], sim->r[j]);
            // We use pointers to loop over the x, y, z members of the vec3_t type.
            for (int d = 0; d < NDIM; d++) {
                if (*(&(r2_r1.x) + d) > 0.5 * sim->box[d])
                    *(&(r2_r1.x) + d) -= sim->box[d];
                if (*(&(r2_r1.x) + d) < -0.5 * sim->box[d])
                    *(&(r2_r1.x) + d) += sim->box[d];
            }

            double dist2 = v3_length2(r2_r1); // square of the distance for computational efficiency
            if (dist2 >= g_of_r_cutoff * g_of_r_cutoff) {
                continue;
            } else {
                double r = (NBINS * sqrt(dist2)) / g_of_r_cutoff;
                gof[(int)r] += 2;
                counter_for_g += 2;
            }
        }
    }

    double num_density = n_particles / (sim->box[0] * sim->box[1] * sim->box[2]);
    double R;
    double dR = (double)g_of_r_cutoff / NBINS;
    const double PI4DR = 4 * M_PI * dR;
    for (int i = 0; i < NBINS; i++) {
        R = i * dR;
        // 4pi/3 ((r + d)^3 - r^) = 4pi (dr^2 + d^2r + d^3/3) = the line below:
        double volume_of_this_shell = PI4DR * (R * (R + dR) + dR * dR / 3.);
        double expected_number = num_density * volume_of_this_shell;
        gof[i] /= expected_number * n_particles; // normalize by distance and number
    }
} */

// will not set packing fraction higher than 0.95 * the "optimal" packing fraction,
// pow(1.0 + 0.5 * CosPhi, -3.0).
void set_packing_fraction(void)
{
    double volume = 1.0;
    for (int d = 0; d < NDIM; ++d)
        volume *= sim->box[d];

    double optimal_packing_fraction = pow(1.0 + 0.5 * CosPhi, -3.0) * 0.95;

    if (packing_fraction > optimal_packing_fraction) { // then this is not gonna happen boy
        printf("packing fraction %5.3lf entered, \
> %5.3lf (optimal pf the way we do it)\nSetting initial pf to %5.2lf\n",
            packing_fraction, optimal_packing_fraction, optimal_packing_fraction);
        // printf("\n\tactually I'm too lazy I'm just gonna stop right here.\n");
        // exit(7);
    }
    double target_pf = fmin(packing_fraction, optimal_packing_fraction);
    double target_volume = (n_particles * ParticleVolume) / (target_pf);
    double scale_factor = pow(target_volume / volume, 1. / NDIM); // the . of 1. is important, otherwise 1 / NDIM == 1 / 3 == 0

    scale(scale_factor);
}

vec3_t random_cube_axis(void)
{
    vec3_t axis;
    int random = (int)ran(0, 6);
    if (random < 2) {
        axis = vec3(0, 0, 1);
    } else if (random < 4) {
        axis = vec3(0, 1, 0);
    } else {
        axis = vec3(1, 0, 0);
    }
    if (random & 1) {
        axis = v3_muls(axis, -1);
    }
    return axis;
}

/// After putting the (slanted) cubes at the right position,
/// rotate each cube 90 degrees around a random axis-aligned axis a few times
void set_random_orientation(void)
{
    // first rotate every particle a number of times along one of the box' axes
    for (int i = 0; i < n_particles; i++) {
        for (int j = 0; j < 12; j++) {
            sim->m[i] = m4_mul(m4_rotation(M_PI / 2, random_cube_axis()), sim->m[i]);
        }
    }
}

/// returns true if there is overlap in the system, else false.
bool is_overlap(void)
{
    for (int i = 0; i < n_particles; i++) {
        for (int j = i + 1; j < n_particles; j++) {
            if (is_overlap_between(i, j)) {
                return true;
            }
        }
    }
    return false;
}

/// reduces initial packing fraction if there is overlap
void remove_overlap(void)
{
    while (is_overlap()) {
        scale(1.01); // make the system bigger and check for collisions again
        printf("initial packing fraction lowered to %lf\n",
            n_particles * ParticleVolume / (sim->box[0] * sim->box[1] * sim->box[2]));
    }
}

/// needs the system to be in at most the optimal density, (1 + 0.5 * CosPhi)^-3.
void remove_overlap_smart(void)
{
    // check if density is below optimal density, this must be the case, otherwise exit
    if (n_particles * ParticleVolume / (sim->box[0] * sim->box[1] * sim->box[2]) > pow(1.0 + 0.5 * CosPhi, -3.0) * 0.96) {
        printf("somehow the density still isn't small enough. Exiting\n");
        exit(4);
    }

    // we use cell lists, so initialize the cell list here
    initialize_cell_list();

    int iterations = 1;
    do {
        int overlap_list[N] = { 0 }; // I'm told this initializes the rest also to zero
        int overlaps = 0; // this will count how many overlaps there are
        for (int i = 0; i < n_particles; i++) {
            if (is_overlap_from(i)) {
                overlap_list[i] = 1;
                overlaps++;
            }
        }
        printf("iteration %d: %d overlaps in the system\n", iterations, overlaps);

        // now try to resolve the overlaps by rotating
        for (int i = 0; i < n_particles; i++) {
            if (overlap_list[i]) {
                mat4_t original_orientation = sim->m[i];
                bool is_good_rotation = false;
                for (int j = 0; j < 24; j++) { // try 24 rotations per particle
                    sim->m[i] = m4_mul(m4_rotation(M_PI / 2, random_cube_axis()), sim->m[i]); // try a random rotation
                    if (is_overlap_from(i)) {
                        continue; // try again
                    } else { // move on to the next particle
                        is_good_rotation = true;
                        break;
                    }
                }
                if (!is_good_rotation) // rotate back to avoid creating additional overlaps
                    sim->m[i] = original_orientation;
            }
        }
        iterations++;
    } while (is_overlap());
}

/// Put parsing the commandline in a function.
/// If something goes wrong, return != 0
int parse_commandline(int argc, char* argv[])
{
    if (argc != 12) {
        printf("need 11 arguments:\n");
        return 3;
    }

    if (EOF == sscanf(argv[7], "%lf", &Phi)) {
        printf("reading Phi has failed\n");
        return 1;
    }
    if (Phi <= 0 || Phi > M_PI / 2) {
        printf("0 < Phi < 1.57079632679\n");
        return 2;
    }
    // read data from a file or create a system with CubesPerDim cubes per dimension
    if (strcmp(argv[1], "read") == 0 || strcmp(argv[1], "r") == 0) {
        printf("\nreading file %s...\n", argv[2]);
        read_data2(argv[2]);
        IsCreated = false;
    } else if (strcmp(argv[1], "create") == 0 || strcmp(argv[1], "c") == 0) {
        printf("\ncreating system with %d cubes...\n", n_particles);
        CubesPerDim = atoi(argv[2]);
        if (CubesPerDim < 4 || CubesPerDim > 41) {
            printf("3 < CubesPerDim < 42, integer!\n%s", usage_string);
            return 1;
        }
        create_system();
        IsCreated = true;
    } else {
        printf("error reading first argument: %s\n", argv[1]);
        return 1;
    }
    if (IsCreated) {
        if (EOF == sscanf(argv[3], "%lf", &packing_fraction)) {
            printf("reading packing_fraction has failed\n");
            return 1;
        }
    } else { // read ==> give output_folder
        sscanf(argv[3], "%s", output_folder);
        printf("saving to datafolder/%s\n", output_folder);
    }
    if (EOF == sscanf(argv[4], "%d", &mc_steps)) {
        printf("reading mc_steps has failed\n");
        return 1;
    }
    if (EOF == sscanf(argv[5], "%d", &output_steps)) {
        printf("reading output_steps has failed\n");
        return 1;
    }
    BetaP = -1.; // if we type nvt/n in the BetaP slot, BetaP will retain this value
    if (EOF == sscanf(argv[6], "%lf", &BetaP)) {
        printf("reading BetaP has failed\n");
        return 1;
    }
    if (BetaP == -1.) { // i.e. if we didn't give a pressure, we want an NVT sim
        IsNVT = true;
    } else if (BetaP <= 0 || BetaP >= 100) {
        printf("0 < BetaP < 100\n");
        return 2; // exits the program so doesn't need to know IsNVT
    } else {
        IsNVT = false;
    }

    char whatorder_string[64] = "";
    switch (argv[8][0]) {
    case '1':
    case 't': // t = transl
        what_order = 1;
        order_character = 't';
        strcat(whatorder_string, "translational");
        break;
    case '2':
    case 's': // s = slanted face normals
        what_order = 2;
        order_character = 's';
        strcat(whatorder_string, "orientational (slanted face normals)");
        break;
    case '3':
    case 'u': // u = unslanted face normals
        what_order = 3;
        strcat(whatorder_string, "orientational (unslanted face normals)");
        order_character = 'u';
        break;
    case '4':
    case 'e': // e = edges
        what_order = 4;
        order_character = 'e';
        strcat(whatorder_string, "orientational (edges)");
        break;
    default:
        printf("reading what_order has failed\n");
        return 1;
    }

    if (EOF == sscanf(argv[9], "%d", &target_cluster_size)) {
        printf("reading target_cluster_size has failed\n");
        return 1;
    }
    if (EOF == sscanf(argv[10], "%lf", &coupling_parameter)) {
        printf("reading coupling_parameter has failed\n");
        return 1;
    }
    if (EOF == sscanf(argv[11], "%lf", &order_cutoff)) {
        printf("reading order_cutoff has failed\n");
        return 1;
    }

    if (mc_steps < 100) {
        printf("mc_steps > 99\n");
        return 2;
    }
    if (IsCreated) { // create ==> give starting packing_fraction
        if (packing_fraction <= 0.01 || packing_fraction > 1) {
            printf("0.01 < packing_fraction <= 1\n");
            return 2;
        }
    }
    if (BetaP <= 0 || BetaP >= 100) {
        printf("0 < BetaP < 100\n");
        return 2;
    }
    if (output_steps < 1) {
        printf("output_steps >= 1\n");
        return 2;
    }
    if (what_order < 1 || what_order > 4) {
        printf("1 <= what_order <= 4\n(transl = 1, sl = 2, unsl = 3, edge = 4)\n");
        return 2;
    }
    if (target_cluster_size < 1 || target_cluster_size > n_particles) {
        printf("1 <= target_cluster_size <= 4\n");
        return 2;
    }
    if (coupling_parameter <= 0 || coupling_parameter >= 1) {
        printf("0 < coupling_parameter < 1\n");
        return 2;
    }
    if (order_cutoff <= 0 || order_cutoff >= 1) {
        printf("0 < order_cutoff < 1\n");
        return 2;
    }

    printf("\nmc_steps\t\t%d\n", mc_steps);
    printf("output_steps\t\t%d\n", output_steps);
    printf("packing_fraction\t%lf\n", packing_fraction);
    printf("BetaP\t\t\t%lf (-1 means NVT)\n", BetaP);
    printf("sim\t\t\t%s\n", IsNVT ? "NVT" : "NpT");
    printf("Phi\t\t\t%lf\n", Phi);
    printf("what_order\t\t%s\n", whatorder_string);
    printf("target_cluster_size\t%d\n", target_cluster_size);
    printf("coupling_parameter\t%lf\n", coupling_parameter);
    printf("order_cutoff\t\t%lf\n\n", order_cutoff);

    return 0; // no exceptions, run the program
}
