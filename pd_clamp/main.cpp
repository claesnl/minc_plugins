#include <minc2.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <iostream>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <ParseArgv.h>

mihandle_t	minc_volume1, minc_volume2, new_volume, mask_volume;
midimhandle_t dimensions1[3], dimensions2[3], dimensions3[3], *dimensions_new;
double      min = 1;
double		max = 1;
char 		*pname;
static int second = FALSE;
static int first = TRUE;
static double constant2[2] = {-100.0, 100.0};
static char *mask = NULL;
char 		**infiles, **outfiles;
int 		nfiles, nout, i;
unsigned long start[3], count[3];
unsigned int  sizes1[3], sizes2[3], sizes3[3];
double        *slab1;
double        *slab2;
double		  *slab_mask;
double        *slab_new;

/* Argument table */
static ArgvInfo argTable[] = {
	{NULL, ARGV_HELP, (char *) NULL, (char *) NULL, "General options:"},
	{"-first", ARGV_CONSTANT, (char *) TRUE, (char *) &first, "Normalize to the first (default)."},
	{"-second", ARGV_CONSTANT, (char *) TRUE, (char *) &second, "Normalize to the second."},
	{"-mask", ARGV_STRING, (char*) 1, (char *) &mask, "Specify location of mask to multiply the resulting PD image with."},
   	{"-const2", ARGV_FLOAT, (char *) 2, (char *) constant2, "Specify two constant arguments (default: -100 to 100)."},
   	{NULL, ARGV_END, NULL, NULL, NULL}
};

int main(int argc, char **argv)
{
	
	pname = argv[0];
	if(ParseArgv(&argc, argv, argTable, 0) || argc < 4)
	{
      	(void) fprintf(stderr, "\nUsage: %s [options] <in1.mnc> <in2.mnc> <out.mnc>\n", pname);
		(void) fprintf(stderr, "       %s -help\n\n", pname);
		exit(EXIT_FAILURE);
	}
	nout = 1;
	outfiles = &argv[argc-1];
	
	/* Get the list of input files either from the command line */
	nfiles = argc - 2;
	infiles = &argv[1];

	/* Make sure that we have something to process */
	if (nfiles == 0) {
	   (void) fprintf(stderr, "No input files specified\n");
	   exit(EXIT_FAILURE);
	}
	
	// fprintf(stdout, "pname: %s\n", pname);
	// fprintf(stdout, "nfiles: %d\n", nfiles);
	// fprintf(stdout, "const2: %f %f\n", constant2[0], constant2[1]);
	// fprintf(stdout, "Second?: %s\n",(second)?"TRUE":"FALSE");
	// fprintf(stdout, "infiles[0]: %s\n",infiles[0]);
	// fprintf(stdout, "infiles[1]: %s\n",infiles[1]);
	// fprintf(stdout, "outfiles: %s\n", (char *) outfiles[0]);
	// fprintf(stdout, "mask: %s\n", (char *) mask);
	
	/* make temporary minc 2.0 files */
	std::system("mkdir /tmp/minc_plugins/");
	std::string in1 = "mincconvert ";
	in1 += infiles[0];
	in1 += " -2 /tmp/minc_plugins/in1.mnc";
	std::system(in1.c_str());
	std::string in2 = "mincconvert ";
	in2 += infiles[1];
	in2 += " -2 /tmp/minc_plugins/in2.mnc";
	std::system(in2.c_str());

	if (miopen_volume("/tmp/minc_plugins/in1.mnc", MI2_OPEN_RDWR, &minc_volume1) != MI_NOERROR)
	{
		fprintf(stderr, "Error opening input file: %s.\n",infiles[0]);
		exit(EXIT_FAILURE);
	}
	if (miopen_volume("/tmp/minc_plugins/in2.mnc", MI2_OPEN_RDWR, &minc_volume2) != MI_NOERROR)
	{
		fprintf(stderr, "Error opening input file: %s.\n",infiles[1]);
		exit(EXIT_FAILURE);
	}
	if (mask != NULL){
		std::string in_mask = "mincconvert ";
		in_mask += mask;
		in_mask += " -2 /tmp/minc_plugins/mask.mnc";
		std::system(in_mask.c_str());
		if (miopen_volume("/tmp/minc_plugins/mask.mnc", MI2_OPEN_RDWR, &mask_volume) != MI_NOERROR)
		{
			fprintf(stderr, "Error opening mask file: %s.\n",mask);
			exit(EXIT_FAILURE);
		}
	}
	
	/* get the dimension sizes */
	miget_volume_dimensions(minc_volume1, MI_DIMCLASS_SPATIAL, MI_DIMATTR_ALL, MI_DIMORDER_FILE, 3, dimensions1);
	miget_dimension_sizes(dimensions1, 3, sizes1);
	miget_volume_dimensions(minc_volume2, MI_DIMCLASS_SPATIAL, MI_DIMATTR_ALL, MI_DIMORDER_FILE, 3, dimensions2);
	miget_dimension_sizes(dimensions2, 3, sizes2);
	if(mask != NULL)
		miget_volume_dimensions(mask_volume, MI_DIMCLASS_SPATIAL, MI_DIMATTR_ALL, MI_DIMORDER_FILE, 3, dimensions3);
	miget_dimension_sizes(dimensions3, 3, sizes3);
	
	if(sizes1[0] != sizes2[0] || sizes1[1] != sizes2[1] || sizes1[2] != sizes2[2] || 
	   (mask != NULL && (sizes1[0] != sizes3[0] || sizes1[1] != sizes3[1] || sizes1[2] != sizes3[2]))){
		fprintf(stderr, "Dimensions of files does not match.\n");
		fprintf(stderr, "   Size of file 1: %d,%d,%d\n",sizes1[0],sizes1[1],sizes1[2]);
		fprintf(stderr, "   Size of file 2: %d,%d,%d\n",sizes2[0],sizes2[1],sizes2[2]);
		fprintf(stderr, "   Size of file 3: %d,%d,%d\n",sizes3[0],sizes3[1],sizes3[2]);
		exit(EXIT_FAILURE);
	}
	
	/* allocate new dimensions */
	dimensions_new = (midimhandle_t *) malloc(sizeof(midimhandle_t) * 3);

	/* copy the dimensions */
	for(i=0; i < 3; i++) {
		micopy_dimension(dimensions1[i], &dimensions_new[i]);
	}

	start[0] = start[1] = start[2] = 0;

	for (i=0; i < 3; i++) {
		count[i] = (unsigned long) sizes1[i];
	}

	/* allocate memory for the hyperslab - make it size of entire volume */
	slab1 = (double *) malloc(sizeof(double) * sizes1[0] * sizes1[1] * sizes1[2]);
	slab2 = (double *) malloc(sizeof(double) * sizes1[0] * sizes1[1] * sizes1[2]);
	slab_new = (double *) malloc(sizeof(double) * sizes1[0] * sizes1[1] * sizes1[2]);
	if(mask != NULL)
		slab_mask = (double *) malloc(sizeof(double) * sizes1[0] * sizes1[1] * sizes1[2]);

	if (miget_real_value_hyperslab(minc_volume1, MI_TYPE_DOUBLE, start, count, slab1) != MI_NOERROR) {
		fprintf(stderr, "Error getting hyperslab for file 1\n");
		return(1);
	}
	if (miget_real_value_hyperslab(minc_volume2, MI_TYPE_DOUBLE, start, count, slab2) != MI_NOERROR) {
		fprintf(stderr, "Error getting hyperslab for file 2\n");
		return(1);
	}
	if (mask != NULL){
		if (miget_real_value_hyperslab(mask_volume, MI_TYPE_DOUBLE, start, count, slab_mask) != MI_NOERROR) {
			fprintf(stderr, "Error getting hyperslab for the mask\n");
			return(1);
		}
	}

	/* loop over all voxels */
	for (i=0; i < sizes1[0] * sizes1[1] * sizes1[2]; i++) {
		slab_new[i] = 0.0;
		
		if(mask == NULL || slab_mask[i] > 0.0){
			/* calculate new PD value */
			if(slab1[i] == 0.0){
				if(slab2[i] > 0.0){
					slab_new[i] = -100.0;
				} else if (slab2[i] < 0.0){
					slab_new[i] = 100.0;
				}
			} else {
				if(second)
					slab_new[i] = 100.0 * ( slab1[i] - slab2[i] ) / slab2[i];
				else
					slab_new[i] = 100.0 * ( slab1[i] - slab2[i] ) / slab1[i];
			}
		}
		
		/* clamp values */
		if(slab_new[i] < constant2[0])
			slab_new[i] = constant2[0];
		if(slab_new[i] > constant2[1])
			slab_new[i] = constant2[1];

		/* check if min or max should be changed */
		if (slab_new[i] < min || min == -1) {
			min = slab_new[i];
		}
		else if (slab_new[i] > max) {
			max = slab_new[i];
		}
	}
	
	/* create the new volume */
	if (micreate_volume("/tmp/minc_plugins/new_volume.mnc", 3, dimensions_new, MI_TYPE_UBYTE,
	MI_CLASS_REAL, NULL, &new_volume) != MI_NOERROR) {
		fprintf(stderr, "Error creating new volume\n");
		return(1);
	}
	
	/* create the data for the new volume */
	if (micreate_volume_image(new_volume) != MI_NOERROR) {
		fprintf(stderr, "Error creating volume data\n");
		return(1);
	}
		
	/* set valid and real range */
	miset_volume_valid_range(new_volume, 255, 0);
	miset_volume_range(new_volume, max, min);

	/* write the modified hyperslab to the file */
	if (miset_real_value_hyperslab(new_volume, MI_TYPE_DOUBLE,
		start, count, slab_new) != MI_NOERROR) {
		fprintf(stderr, "Error setting hyperslab\n");
		return(1);
	}

	/* closes the volume and makes sure all data is written to file */
	miclose_volume(minc_volume1);
	miclose_volume(minc_volume2);
	miclose_volume(new_volume);

	/* free memory */
	free(dimensions_new);
	free(slab1);
	free(slab2);
	free(slab_new);
	
	if(mask != NULL){
		free(slab_mask);	
		miclose_volume(mask_volume);
	}
	
	std::string s_outfiles = "mincconvert -clobber /tmp/minc_plugins/new_volume.mnc ";
	s_outfiles += outfiles[0];
	std::cout << s_outfiles << std::endl;
	std::system(s_outfiles.c_str());
	
	std::system("rm -rf /tmp/minc_plugins/");

	return(0);
}
