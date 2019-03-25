////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Generate a plane wave
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Generates a plane wave function used to CTEM simulations. The value of the real part can be set (though often it is
/// just left as 1). The complex part is always 0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// output - empty buffer to be filled with the generated wave function
/// width - width of output
/// height - height of output
/// value - value for the real part of the generate wave function
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
__kernel void init_plane_wave_d( global double2* output,
										unsigned int width,
										unsigned int height,
										double value)
{
	int xid = get_global_id(0);
	int yid = get_global_id(1);
	if(xid < width && yid < height)
	{
		int id = xid + width*yid;
		output[id].x = value;
		output[id].y = 0.0;
	}
}