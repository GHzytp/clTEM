////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Generates the crystal potential (using the conventional technique, no accounting for z)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// This is really where the magic happens
/// However, I feel it is not well optimised, or at least not well organised
///
/// This kernel calculates the actual potential for each slice, which the wave function is then propagated through. Most
/// of the parameterisation stuff can be found in Kirkland's "Advanced computing in electron microscopy 2nd ed." in
/// appendix C (also where the parameters are given). The rest of this function is loading the atoms, calculating
/// whether they are relevant and maybe more.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// potential - the output potential image for the slice
/// pos_x - x position of the atoms
/// pos_y - y position of the atoms
/// pos_z - z position of the atoms
/// atomic_num - atomic number of the atoms
/// params - perameterised form of the scattering factors
/// block_start_pos - the start positions (real space) of each block
/// width - width of the output potential
/// height - height of the output potential
/// current_slice - current slice of the simulation
/// total_slices - total number of slices in the simulation
/// z - current z position
/// dz - the slice thickness
/// pixelscale - pixel scale of the image in real space
/// blocks_x - total number of blocks in x direction
/// blocks_y - total number of blocks in y direction
/// max_x - max x position (including padding)
/// min_x - min x position (including padding)
/// max_y - max y position (including padding)
/// min_y - min y position (including padding)
/// block_load_x - blocks to load in x direction
/// block_load_y - blocks to load in y direction
/// slice_load_z - blocks to load in z direction
/// sigma - the interaction parameter (given by eq. 5.6 in Kirkland)
/// startx - x start position of simulation (when simulation is cropped)
/// starty - y start position of simulation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Bessel functions (Used the the projected potential calculations
/// These function's can be found in "Numerical recipes in C, 2nd ed." Chapter 6.6.
/// bessi0 - calculate the zero order modified bessel function of the first kind (using only for bessk0)
/// bessk0 - calculate the zero order modified bessel function of the second kind
/// bessi1 - calculate the zero order modified bessel function of the first kind (using only for bessk0)
/// bessk1 - calculate the zero order modified bessel function of the second kind
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Projected potential functions
/// The lobato paper (10.1107/S205327331401643X) gives a good overview of these parameters. Kirkland's book 2nd ed. has
/// a useful table too (Table C.1) to see a list of other parameterisations.
/// kirkland - See equation C.20 from Kirkland's book (2nd ed.). Parameters are stored as:
/// a1, b1, a2, b2, a3, b3, c1, d1, c2, d2, c3, d3
/// lobato - See equation 16 from their paper (10.1107/S205327331401643X). Parameter are stored as:
/// a1, a2, a3, a4, a5, b1, b2, bb, b4, b5
/// peng - See equation 47 from the lobato paper (this needs to be integrated for the projected potential. Reference
/// 10.1107/S0108767395014371. Parameters are stored as: a1, a2, a3, a4, a5, b1, b2, bb, b4, b5
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define recip(x) (1.0f / (x))

__constant float i0a[7] = {1.0f, 3.5156229f, 3.0899424f, 1.2067492f, 0.2659732f, 0.0360768f, 0.0045813f};
__constant float i0b[9] = {0.39894228f, 0.01328592f, 0.00225319f, -0.00157565f, 0.00916281f, -0.02057706f, 0.02635537f, -0.01647633f, 0.00392377f};
__constant float i1a[7] = {0.5f, 0.87890594f, 0.51498869f, 0.15084934f, 0.02658733f, 0.00301532f, 0.00032411f};
__constant float i1b[9] = {0.39894228f, -0.03988024f, -0.00362018f, 0.00163801f, -0.01031555f, 0.02282967f, -0.02895312f, 0.01787654f, 0.00420059f};
__constant float k0a[7] = {-0.57721566f, 0.42278420f, 0.23069756f, 0.03488590f, 0.00262698f, 0.00010750f, 0.00000740f};
__constant float k0b[7] = {1.25331414f, -0.07832358f, 0.02189568f, -0.01062446f, 0.00587872f, -0.00251540f, 0.00053208f};
__constant float k1a[7] = {1.0f, 0.15443144f, -0.67278579f, -0.18156897f, -0.01919402f, -0.00110404f, -0.00004686f};
__constant float k1b[7] = {1.25331414f, 0.23498619f, -0.03655620f, 0.01504268f, -0.00780353f, 0.00325614f, -0.00068245f};

float bessi0(float x) {
	int i;
	float ax, x2, sum;

	ax = fabs(x);

	if(ax <= 3.75f) {
		x2 = x / 3.75f;
		x2 = x2 * x2;
		sum = i0a[6];
		for(i = 5; i >= 0; --i)
			sum = sum*x2 + i0a[i];
	} else {
		x2 = 3.75f / ax;
		sum = i0b[8];
		for(i=7; i>=0; --i)
			sum = sum*x2 + i0b[i];
		sum = native_exp(ax) * sum * native_rsqrt(ax);
	}

	return sum;
}

float bessi1(float x) {
    int i;
    float ax, x2, sum;

    ax = fabs(x);

    if(ax <= 3.75f) {
        x2 = x / 3.75f;
        x2 = x2 * x2;
        sum = i1a[6];
        for(i = 5; i >= 0; --i)
            sum = sum*x2 + i1a[i];
        sum *= ax;
    } else {
        x2 = 3.75f / ax;
        sum = i1b[8];
        for(i = 7; i >= 0; --i)
            sum = sum*x2 + i1b[i];
        sum = native_exp(ax) * sum * native_rsqrt(ax);
    }

    return sum;
}

float bessk0(float x) {
	int i;
	float ax, x2, sum;

	ax = fabs(x);

	if((ax > 0.0f)  && (ax <=  2.0f)) {
		x2 = ax/2.0f;
		x2 = x2 * x2;
		sum = k0a[6];
		for(i = 5; i >= 0; --i)
			sum = sum*x2 + k0a[i];
		sum = -native_log(ax/2.0f) * bessi0(x) + sum;
	} else if(ax > 2.0f) {
		x2 = 2.0f/ax;
		sum = k0b[6];
		for(i=5; i>=0; --i)
			sum = sum*x2 + k0b[i];
		sum = native_exp(-ax) * sum * native_rsqrt(ax);
	} else
		sum = FLT_MAX;

	return sum;
}

float bessk1(float x) {
    int i;
    float ax, x2, sum;

    ax = fabs(x);

    if((ax > 0.0f)  && (ax <=  2.0f)) {
        x2 = ax/2.0f;
        x2 = x2 * x2;
        sum = k1a[6];
        for(i = 5; i >= 0; --i)
            sum = sum*x2 + k1a[i];
        sum = native_log(ax/2.0f) * bessi1(x) + sum / ax;
    } else if( ax > 2.0f ) {
        x2 = 2.0f/ax;
        sum = k1b[6];
        for(i=5; i>=0; --i)
            sum = sum*x2 + k1b[i];
        sum = native_exp(-ax) * sum * native_rsqrt(ax);
    } else
        sum = FLT_MAX;

    return sum;
}

float kirkland(__constant float* params, int i_lim, int ZNum, float rad) {
    int i;
    float suml, sumg, x;
    suml = 0.0f;
    sumg = 0.0f;

    int z_ofst = (ZNum - 1) * 12;

    //
    // Lorentzians
    //
    x = 2.0f * M_PI_F * rad;

    // Loop through our parameters (a and b)
    for(i = 0; i < i_lim*2; i+=2) {
        float a = params[z_ofst + i];
        float b = params[z_ofst + i + 1];
        suml += a * bessk0( x * native_sqrt(b) );
    }

    //
    // Gaussians
    //
    x = M_PI_F * rad;
    x = x * x;

    // Loop through our parameters (a and b)
    for(i = i_lim*2; i < i_lim*4; i+=2) {
        float c = params[z_ofst + i];
        float d = params[z_ofst + i + 1];
        float d_inv = native_recip(d);
        sumg += (c * d_inv) * native_exp(-x * d_inv);
    }

    // The funny floats are from the remaining constants in equation C.20
    // Not that they use the fundamental charge as 14.4 Volt-Angstroms
    return 300.8242834f * suml + 150.4121417f * sumg;
 }

float lobato(__constant float* params, int i_lim, int ZNum, float rad) {
    int i;
    float sum, x;
    sum = 0.0f;

    int z_ofst = (ZNum - 1) * 10;

    x = 2.0f * M_PI_F * rad;

    for(i=0; i < i_lim; ++i) {
        float a = params[z_ofst+i];
        float b = params[z_ofst+i+5];
        float b_inv_root = native_rsqrt(b);
        sum += a * (b_inv_root * b_inv_root * b_inv_root) * (bessk0(x * b_inv_root) + rad * bessk1(x * b_inv_root));
    }

    return 945.090144399935f * sum;
}

float peng(__constant float* params, int i_lim, int ZNum, float rad) {
    int i;
    float sum, x;
    sum = 0.0f;

    int z_ofst = (ZNum - 1) * 10;

    x = M_PI_F * rad;
    x = x * x;

    for(i=0; i < i_lim; ++i) {
        float a = params[z_ofst+i];
        float b = params[z_ofst+i+5];
        float b_inv = native_recip(b);

        sum += a * b_inv * native_exp(-x * b_inv);
    }

    return 150.4121417f * sum;
}

__kernel void transmission_potentials_projected_f( __global float2* potential,
							         __global const float* restrict pos_x,
						  		     __global const float* restrict pos_y,
						 		     __global const float* restrict pos_z,
								     __global const int* restrict atomic_num,
								     __constant float* params,
								     unsigned int param_selector,
									 unsigned int param_i_count,
						 		     __global const int* restrict block_start_pos,
								     unsigned int width,
								     unsigned int height,
								     int current_slice,
								     int total_slices,
								     float z,
								     float dz,
								     float pixelscale, 
								     int blocks_x,
								     int blocks_y,
								     float max_x,
								     float min_x,
								     float max_y,
								     float min_y,
								     int block_load_x,
								     int block_load_y,
								     int slice_load_z,
								     float sigma,
						  		     float startx,
								     float starty,
								     float beam_theta,
								     float beam_phi)
{
	int xid = get_global_id(0);
	int yid = get_global_id(1);
	int lid = get_local_id(0) + get_local_size(0)*get_local_id(1);
	int id = xid + width * yid;
	float sumz = 0.0f;
	int gx = get_group_id(0);
	int gy = get_group_id(1);
	// convert from mrad to radians (and get beam tilt from the surface)
	beam_theta = M_PI_2_F - beam_theta * 0.001f;

	__local float atx[256];
	__local float aty[256];
	__local int atZ[256];

	// calculate the indices of the bins we will need
    // get the size of one workgroup
    float group_size_x = get_local_size(0) * pixelscale;
    float group_size_y = get_local_size(1) * pixelscale;

    // get the start and end position of the current workgroup
    float group_start_x = startx +  gx      * group_size_x;
    float group_end_x   = startx + (gx + 1) * group_size_x;

    float group_start_y = starty +  gy      * group_size_y;
    float group_end_y   = starty + (gy + 1) * group_size_y;

    // get the reciprocal of the full range (for efficiency)
    float recip_range_x = native_recip(max_x - min_x);
    float recip_range_y = native_recip(max_y - min_y);

    int starti = fmax(floor( blocks_x * (group_start_x - min_x) * recip_range_x) - block_load_x, 0);
    int endi   = fmin( ceil( blocks_x * (group_end_x   - min_x) * recip_range_x) + block_load_x, blocks_x - 1);
	int startj = fmax(floor( blocks_y * (group_start_y - min_y) * recip_range_y) - block_load_y, 0);
	int endj   = fmin( ceil( blocks_y * (group_end_y   - min_y) * recip_range_y) + block_load_y, blocks_y - 1);

    int k = current_slice;
    if (k < 0)
        k = 0;
    if (k >= total_slices)
        k = total_slices - 1;


    // loop through our bins (y only, x is handled using the workgroup)
	for (int j = startj ; j <= endj; j++) {
        // for this y block, get the range of indices to use (this is what the block_Start_pos is) from the x blocks
		int start = block_start_pos[k*blocks_x*blocks_y + blocks_x*j + starti  ];
		int end   = block_start_pos[k*blocks_x*blocks_y + blocks_x*j + endi + 1];

        // this gid is effectively where the atoms indices are looped through (using the local ids)
        // so we are parellelising this over the local workgroup
		int gid = start + lid;

		if(lid < end-start) {
			atx[lid] = pos_x[gid];
			aty[lid] = pos_y[gid];
			atZ[lid] = atomic_num[gid];
		}

        // this makes sure all the local threads have finished getting the atoms we need, atx, aty and atZ are complete
		barrier(CLK_LOCAL_MEM_FENCE);

        // now we parallelise over pixels, not atoms
		for (int l = 0; l < end-start; l++) {
			// calculate the radius from the current position in space
            float im_pos_x = startx + xid * pixelscale;
            float rad_x = im_pos_x - atx[l];

            float im_pos_y = starty + yid * pixelscale;
            float rad_y = im_pos_y - aty[l];

			//float rad = native_sqrt(rad_x*rad_x + rad_y*rad_y);
			float cos_beam_phi = native_cos(beam_phi);
			float sin_beam_phi = native_sin(beam_phi);
			float sin_beam_2theta = native_sin(2.0f * beam_theta);

			float z_prime = -0.5f * (rad_x * cos_beam_phi + rad_y * sin_beam_phi) * sin_beam_2theta;

			float z_by_tan_beam_theta = z_prime / native_tan(beam_theta);

			float x_prime = rad_x + z_by_tan_beam_theta * cos_beam_phi;
			float y_prime = rad_y + z_by_tan_beam_theta * sin_beam_phi;

            float rad = native_sqrt(z_prime*z_prime + x_prime*x_prime + y_prime*y_prime);

            float r_min = 0.25f * pixelscale;
			if(rad < r_min) // is this sensible?
				rad = r_min;

			if( rad <= 8.0f) { // Should also make sure is not too small
				if (param_selector == 0)
                    sumz += kirkland(params, param_i_count, atZ[l], rad);
                else if (param_selector == 1)
                    sumz += peng(params, param_i_count, atZ[l], rad);
                else if (param_selector == 2)
                    sumz += lobato(params, param_i_count, atZ[l], rad);
			}
		}

		barrier(CLK_LOCAL_MEM_FENCE);
	}

	if(xid < width && yid < height) {
		potential[id].x = native_cos(sigma * sumz);
		potential[id].y = native_sin(sigma * sumz);
	}
}
