Key:
- is an item  

##is a cancelled item

= is a done item



TODO:
================================================================================================
- make v14 (extra histograms) see paper

- make output_steps readable from commandline

- make histograms of order param data (#nearest neighbours, q4.q4, frac xtalline cubes, etc.)
	- doing in v11.  
		##make separate file for each step

- write latex on positional, orientational order param code

- somehow write transl_order and orient_order as the same function, because there is a lot of
	duplicate code in v10.

- make nice fluids for pressures 6-9 (go as high as possible) and differing angles (see notes):
	##make v3 do: stop volume moves after reaching desired pf, i.e. make it do NVT
	= make v5 do: do one run and save inside a file at which snapshots/after how many
	  runs we find 0.4x (for x = 0, 1, ... , 9) and 0.50

- make readme of visualization tool, explain how h_slc_cl_read.c works?





DONE:
================================================================================================
= make this todo in markdown format  

= make a simulation which does not crush, but slowly increases pressure  
	= done in v13

= make orientational order parameter code (done in v9)  
	= understand orientational order param code  
	= test orient. o.p. in limits where it should be 0 (random) and 1 (perfect order)  
		= done with v12 and v10

= check orientational order parameter axes as:  
	= v9: normals of slanted cube  
	##v10: normals of unslanted cube  
	##v11: edges of slanted cube  
	= v10: command line option for all of the above

= make output file name(s) readable from command line  
	= done in v8

= make standalone positional, orientational order param code  
	= only print cluster snapshot every ~1000 steps  
	= make switch statements to enable doing both pos. and orient. at the same time.  
	= DONE, is v9

= also show density in snapshot

= make g(r) (nice for thesis) to find density-dependent first-shell-radius  
	= max -> 1st minimum  
	= first shell ~ 2 * edge length

##make write_data2 to write data in the format for order_parameter

= make v4 do: v3 + snapshots for clustervisualization  
	- Note: I will put the datafiles in the same datafolder, but call them clustcoord...poly

= make the clusters visible in normal visualization format, call this program visualize3  
	= note: not visualize3, just visualize. Will alias this to show now.

= TODO: BetaP in crystal_v4.c has been hacked together. Fix this.

= make crystal_modified.c work  
	= Segmentation fault (core dumped)

= make read_data2 to read from .poly files (i.e. to make it able to read the files it makes)  
	= make the initial configuration file something to be entered on the command line

= check read_data2, is probably still bugged.  
	= nope, i set packing fraction when reading but that should only happen when creating

= check if cell lists are fixed  
	= YES

= read_data2 can be found in tests/test.c!

= generate initial configuration



Notes:
================================================================================================
	Currently (2019-02-11) in use:
		v1: simulate normally
		v8: simulate NVT, starting from a coord file
		v11: make order param data + histograms 
		v13: slowly increase pressure
	Currently (2019-01-30) in use:
		v1: simulate normally
		v8: simulate NVT, starting from a coord file
		v9: read all coord files from a datafolder and give orient. or transl. order data
			note: orient with normals of slanted cube

##vtemp is temporary (duh) v1 but saves snapshot every 200 steps

- v14 is just v11 but with q4 dot neighbours averaged per cube hist and #nghbr Xtalline hist
- v13 is just v6 but don't immediately create the crystal order parameter, and slowly increases pressure, i.e. BetaP += 5 every 10k steps
- v12 is a quick test maker which reads a coordinate file and spits out the same coordinate file, but with (1) each cube in the standard orientation (2) then rotate each cube about x, y, or z-axis a few times (3) then rotate every cube around a random axis a few times
- v11 is v10 but which also makes #neighbours histogram and |q_4|^2 histogram
- v10 is v9 but with extra functionality: it can choose whether to use: slanted normals, unslanted normals, or edges of the slanted cube. (sl_norm/unsl_norm/edge)
- bond length 2 is waaaay too large for transl. o.p.
- v9 is read coordsstep (from e.g. v6) and make (p)osition or (o)rientation or (b)oth or (a)ll clust coords (read from	cmd line every how many 100 steps to make an pos_coordsstep%07d.poly.)
	- v9 usage: program datafolder_name output_per p/o/b/a
		- explanation: look for coords_step%07d in datafolder/datafolder_name until none more can be found. Output clust_pos_... or clust_orient_... every output_per files found. do pos. ordering for p, orient. ordering for o. or both (b) or all (a)
- v8 is read a coord file and continue an nvt or npt simulation.
	- usage: nvt datafolder/..input.. step_number mc_steps datafolder/..output..\n\
	- usage2 (not implemented yet): program.exe npt BetaP datafolder/..input.. step_number datafolder/..output..";
- v8 might detect overlap but that is due to precision errors when printing coordinate files, these have "only" six decimal places.- v7 is just v5 but prints density next to 'count' in g%07d.txt every step too.
- v6 is just v4 but with the colors fixed
- v5 is g_of_r_maker, it simulates a new system and fprints the # of neighbours it counts and the histogram for each step
- v4 is v3 but uses crystal_v4.c to also save the clusters

		slant angles we wish to study: arccos[x] (for x = 0.0, 0.1, ... , 0.4), in radians:
			1.57079632679490, 1.47062890563334, 1.36943840600457, 1.26610367277950, 1.15927948072741
			in degrees:   90, 84.26,            78.46,            72.54,            66.42.
			(0.5, 60 degrees, 1.04719755119660 we will not do due to six-fold symmetry possibility)

- OOPS, I don't need 16 programs running and letting them stop at different packing fractions! Because they start at the same time, their seed is the same, so each simulation is exactly the same! Just save the snapshot when we have reached a higher packing fraction. Implementing this idea in v3_read_crush.c
- v3 is as above, start low pf., do 10k steps, then set pressure to 10k. Record when density
	reachers 0.40, 0.41, ... , 0.50
- v2_read_crush.c will start at low density and crush it to the specified density, i.e.
  put pressure to 10000 until the desired density is reached, making a snapshot at that moment.
------------------------------------------------------------------------------------------------
Last meeting:
- make "nice fluids library" for densities 6-9 (go as high as possible)
	- do this by making a low density fluid, crushing as fast as possible until reaching desired density and checking if the system has order at this point.
- work on communication
------------------------------------------------------------------------------------------------
Now the idea is to just use the crystal.c file, but use my own read_file method and convert
my positions "r[N]" into particle array called "p".

------------------------------------------------------------------------------------------------
Last runs:
- _6fix: apply per bd cond upon find
- _6test: exit upon find
	- no exits
- newcl: new (conventional) method of doing celllists + prints & apply per. bd. cnd.
	- no prints
------------------------------------------------------------------------------------------------
Last meeting:  
= Instead of celllength >= diam + max_step_size, and update celllist after completed step, do celllength >= diam and update celllist after move, and again if move back.  
= Pipe stdout to a log file rather than /dev/null

------------------------------------------------------------------------------------------------
- Since overlap seems to occur right at the boundary of the box, maybe something funny is going on when saying x_new = (int) r_new.x / CellLength;. HOWEVER, sometimes, e.g. at pressure 1 for one particular simulation (TODO: which one), it happens everywhere. Nevertheless, the following tests will run for the weekend:

- sl10v2: increased MAXPC, probably not gonna do anything (PNGDA)
	- indeed, overlap still happens.

- sl10v3: increased MAXPC, pos_mod_f in update_cell_list(), PNGDA
	- exit status (5), Too many particles in one cell???

- dbg: increased MAXPC, pos_mod_f in update_cell_list(), exit(6) upon overshooting, PNGDA
	- exit status (5), Too many particles in one cell???

- dbg2: increased MAXPC, exit(6) upon overshooting, PNGDA
	- exit status (6), overshoot indeed happens!

- sl10d: increased MAXPC, increased precision of math4d.h (math5d.h, where float --> double)
	- (seemingly) no overlaps!



Questions:
================================================================================================
- Should we expand e.g. a cube heightmap in spherical harmonics to see what the coefficients
	for the spherical harmonics are, to find nice cubic symmetry?

------------------------------------------------------------------------------------------------
= 3 axes of cube well-defined (i.e. the normals/edges). What about 3 axes of slanted cube? should we go with the normals or edges?  
	= to be decided

= Ask Frank:  
	== What is typedef struct {double nz; double si; double co ;int n;}bndT;  
		=== a bond between two particles (i.e. they are close enough to each other, where the position w.r.t eachother is given in polar coords, z-dist, sin and cos of the angle around z-axis.  
	== What is typedef struct {int n; bndT * bnd;}blistT;  
		=== a list of pointers to these bndTs, with the length of this list.  
	== Is particlestocount == n_part always true?  
		=== yes

= Randomly orienting, is 12 rotations of \pm pi/2 around random axes aligned with box enough?  
	== yes, probably, we will check this using the positional order parameter
