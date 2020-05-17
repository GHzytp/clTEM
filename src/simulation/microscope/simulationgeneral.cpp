//
// Created by Jon on 31/01/2020.
//

#include "simulationgeneral.h"

template <class T>
void SimulationGeneral<T>::initialiseBuffers() {

    auto sm = job->simManager;

    // this needs to change if the parameter sizes have changed
    size_t p_sz = job->simManager->getStructureParameterData().size();
    if (size_t ps = p_sz; ps != ClParameterisation.GetSize())
        ClParameterisation = clMemory<T, Manual>(ctx, ps);

    // these need to change if the atom_count changes
    if (size_t as = sm->getStructure()->getAtoms().size(); as != ClAtomA.GetSize()) {
        ClAtomA = clMemory<int, Manual>(ctx, as);
        ClAtomX = clMemory<T, Manual>(ctx, as);
        ClAtomY = clMemory<T, Manual>(ctx, as);
        ClAtomZ = clMemory<T, Manual>(ctx, as);

        ClBlockIds = clMemory<int, Manual>(ctx, as);
        ClZIds = clMemory<int, Manual>(ctx, as);
    }

//    ClBlockStartPositions is not here as it is sorted every time the atoms are sorted (depends on block size etc..

    // change when the resolution does
    unsigned int rs = sm->getResolution();
    if (rs != clXFrequencies.GetSize()) {
        clXFrequencies = clMemory<T, Manual>(ctx, rs);
        clYFrequencies = clMemory<T, Manual>(ctx, rs);
        clPropagator = clMemory<std::complex<T>, Manual>(ctx, rs * rs);
        clTransmissionFunction = clMemory<std::complex<T>, Manual>(ctx, rs * rs);
        clWaveFunction3 = clMemory<std::complex<T>, Manual>(ctx, rs * rs);

        clWaveFunction1.clear();
        clWaveFunction2.clear();
        clWaveFunction4.clear();

        for (int i = 0; i < sm->getParallelPixels(); ++i) {
            clWaveFunction1.emplace_back(ctx, rs * rs);
            clWaveFunction2.emplace_back(ctx, rs * rs);
            clWaveFunction4.emplace_back(ctx, rs * rs);
        }
    }

    if (sm->getParallelPixels() < clWaveFunction1.size()) {
        clWaveFunction1.resize(sm->getParallelPixels());
        clWaveFunction2.resize(sm->getParallelPixels());
        clWaveFunction4.resize(sm->getParallelPixels());
    } else if (sm->getParallelPixels() > clWaveFunction1.size()) {
        for (int i = 0; i < sm->getParallelPixels() - clWaveFunction1.size(); ++i) {
            clWaveFunction1.emplace_back(ctx, rs * rs);
            clWaveFunction2.emplace_back(ctx, rs * rs);
            clWaveFunction4.emplace_back(ctx, rs * rs);
        }
    }

//    // when resolution changes (or if enabled)
//    auto sim_mode = sm->getMode();
//    if (sim_mode == SimulationMode::CTEM && (sim_mode != last_mode || rs*rs != clImageWaveFunction.GetSize())) {
//        clImageWaveFunction = clMemory<std::complex<T>, Manual>(ctx, rs * rs);
//
//        // TODO: I can further split these up, but they aren't a huge issue
//        clTempBuffer = clMemory<std::complex<T>, Manual>(ctx, rs * rs);
//        clCcdBuffer = clMemory<T, Manual>(ctx, 725);
//    }
//
//    if (sim_mode == SimulationMode::STEM && (sim_mode != last_mode || rs*rs != clTDSMaskDiff.GetSize())) {
//        clTDSMaskDiff = clMemory<T, Manual>(ctx, rs * rs);
//        clReductionBuffer = clMemory<T, Manual>(ctx, rs*rs / 256); // STEM only
//    }

    // TODO: could clear unneeded buffers when sim type switches, but there aren't many of them... (the main ones are the wavefunction vectors)
}

template <>
void SimulationGeneral<float>::initialiseKernels() {
    auto sm = job->simManager;

    unsigned int rs = sm->getResolution();
    if (rs != FourierTrans.GetWidth() || rs != FourierTrans.GetHeight())
        FourierTrans = clFourier<float>(ctx, rs, rs);

    bool isFull3D = sm->isFull3d();
    if (do_initialise_general || isFull3D != last_do_3d) {
        if (isFull3D)
            CalculateTransmissionFunction = Kernels::transmission_potentials_full_3d_f.BuildToKernel(ctx);
        else
            CalculateTransmissionFunction = Kernels::transmission_potentials_projected_f.BuildToKernel(ctx);
    }
    last_do_3d = isFull3D;

    if (do_initialise_general) {
        AtomSort = Kernels::atom_sort_f.BuildToKernel(ctx);
        fftShift = Kernels::fft_shift_f.BuildToKernel(ctx);
        BandLimit = Kernels::band_limit_f.BuildToKernel(ctx);
        GeneratePropagator = Kernels::propagator_f.BuildToKernel(ctx);
        ComplexMultiply = Kernels::complex_multiply_f.BuildToKernel(ctx);
    }

    do_initialise_general = false;
}

template <>
void SimulationGeneral<double>::initialiseKernels() {
    auto sm = job->simManager;

    unsigned int rs = sm->getResolution();
    if (rs != FourierTrans.GetWidth() || rs != FourierTrans.GetHeight())
        FourierTrans = clFourier<double>(ctx, rs, rs);

    bool isFull3D = sm->isFull3d();
    if (do_initialise_general || isFull3D != last_do_3d) {
        if (isFull3D)
            CalculateTransmissionFunction = Kernels::transmission_potentials_full_3d_d.BuildToKernel(ctx);
        else
            CalculateTransmissionFunction = Kernels::transmission_potentials_projected_d.BuildToKernel(ctx);
    }
    last_do_3d = isFull3D;

    if (do_initialise_general) {
        AtomSort = Kernels::atom_sort_d.BuildToKernel(ctx);
        fftShift = Kernels::fft_shift_d.BuildToKernel(ctx);
        BandLimit = Kernels::band_limit_d.BuildToKernel(ctx);
        GeneratePropagator = Kernels::propagator_d.BuildToKernel(ctx);
        ComplexMultiply = Kernels::complex_multiply_d.BuildToKernel(ctx);
    }

    do_initialise_general = false;
}

template <class T>
void SimulationGeneral<T>::sortAtoms() {
    CLOG(DEBUG, "sim") << "Sorting Atoms";

    bool do_phonon = job->simManager->getInelasticScattering()->getPhonons()->getFrozenPhononEnabled();
//    bool do_inelastic = job->simManager->getInelasticScattering()->getInelasticEnabled();

    // TODO: check that this is only useful here
    if (job->simManager == current_manager && !do_phonon) {
        CLOG(DEBUG, "sim") << "Atoms already sorted, reusing that data";
        return;
    }
    current_manager = job->simManager;

    std::vector<AtomSite> atoms = job->simManager->getStructure()->getAtoms();
    auto atom_count = static_cast<unsigned int>(atoms.size()); // Needs to be cast to int as opencl kernel expects that size

    std::vector<int> AtomANum;
    std::vector<T> AtomXPos;
    std::vector<T> AtomYPos;
    std::vector<T> AtomZPos;

    AtomANum.reserve(atom_count);
    AtomXPos.reserve(atom_count);
    AtomYPos.reserve(atom_count);
    AtomZPos.reserve(atom_count);

    CLOG(DEBUG, "sim") << "Getting atom positions";
    if (do_phonon)
        CLOG(DEBUG, "sim") << "Using TDS";

    // For sorting the atoms, we want the total area that the simulation covers
    // Basically, this only applies to STEM, so this atom sorting covered all the pixels,
    // even if we aren't going to be using all these atoms for each pixel
    // also used to limit the atoms we have to sort
    std::valarray<double> x_lims = job->simManager->getPaddedFullLimitsX();
    std::valarray<double> y_lims = job->simManager->getPaddedFullLimitsY();
    std::valarray<double> z_lims = job->simManager->getPaddedStructLimitsZ();

    for(int i = 0; i < atom_count; i++) {
        double dx = 0.0, dy = 0.0, dz = 0.0;
        if (do_phonon) {
            // TODO: need a log guard here or in the structure file?
            dx = job->simManager->getInelasticScattering()->getPhonons()->generateTdsFactor(atoms[i], 0);
            dy = job->simManager->getInelasticScattering()->getPhonons()->generateTdsFactor(atoms[i], 1);
            dz = job->simManager->getInelasticScattering()->getPhonons()->generateTdsFactor(atoms[i], 2);
        }

        // TODO: could move this check before the TDS if I can get a good estimate of the maximum displacement

        int new_x = atoms[i].x + dx;
        int new_y = atoms[i].y + dy;
        int new_z = atoms[i].z + dz;
        bool in_x = new_x > x_lims[0] && new_x < x_lims[1];
        bool in_y = new_y > y_lims[0] && new_y < y_lims[1];
        bool in_z = new_z > z_lims[0] && new_z < z_lims[1];

        if (in_x && in_y && in_z) {
            // puch back is OK because I have reserved the vector
            AtomANum.push_back(atoms[i].A);
            AtomXPos.push_back(atoms[i].x + dx);
            AtomYPos.push_back(atoms[i].y + dy);
            AtomZPos.push_back(atoms[i].z + dz);
        }
    }

    // update our atom count to be the atoms we have in range
    atom_count = AtomANum.size();

    CLOG(DEBUG, "sim") << "Writing to buffers";

    ClAtomX.Write(AtomXPos);
    ClAtomY.Write(AtomYPos);
    ClAtomZ.Write(AtomZPos);
    ClAtomA.Write(AtomANum);

    CLOG(DEBUG, "sim") << "Creating sort kernel";

    // NOTE: DONT CHANGE UNLESS CHANGE ELSEWHERE ASWELL!
    // Or fix it so they are all referencing same variable.
    unsigned int BlocksX = job->simManager->getBlocksX();
    unsigned int BlocksY = job->simManager->getBlocksY();

    double dz = job->simManager->getSliceThickness();
    unsigned int numberOfSlices = job->simManager->getNumberofSlices();

    AtomSort.SetArg(0, ClAtomX, ArgumentType::Input);
    AtomSort.SetArg(1, ClAtomY, ArgumentType::Input);
    AtomSort.SetArg(2, ClAtomZ, ArgumentType::Input);
    AtomSort.SetArg(3, atom_count);
    AtomSort.SetArg(4, static_cast<T>(x_lims[0]));
    AtomSort.SetArg(5, static_cast<T>(x_lims[1]));
    AtomSort.SetArg(6, static_cast<T>(y_lims[0]));
    AtomSort.SetArg(7, static_cast<T>(y_lims[1]));
    AtomSort.SetArg(8, static_cast<T>(z_lims[0]));
    AtomSort.SetArg(9, static_cast<T>(z_lims[1]));
    AtomSort.SetArg(10, BlocksX);
    AtomSort.SetArg(11, BlocksY);
    AtomSort.SetArg(12, ClBlockIds, ArgumentType::Output);
    AtomSort.SetArg(13, ClZIds, ArgumentType::Output);
    AtomSort.SetArg(14, static_cast<T>(dz));
    AtomSort.SetArg(15, numberOfSlices);

    clWorkGroup SortSize(atom_count, 1, 1);
    CLOG(DEBUG, "sim") << "Running sort kernel";
    AtomSort.run(SortSize);

    ctx.WaitForQueueFinish(); // test

    CLOG(DEBUG, "sim") << "Reading sort kernel output";

    std::vector<int> HostBlockIDs = ClBlockIds.CreateLocalCopy();
    std::vector<int> HostZIDs = ClZIds.CreateLocalCopy();

    CLOG(DEBUG, "sim") << "Binning atoms";

    // this silly initialising is to make the first two levels of our vectors, we then dynamically
    // fill the next level in the following loop :)
    std::vector<std::vector<std::vector<T>>> Binnedx( BlocksX*BlocksY, std::vector<std::vector<T>>(numberOfSlices) );
    std::vector<std::vector<std::vector<T>>> Binnedy( BlocksX*BlocksY, std::vector<std::vector<T>>(numberOfSlices) );
    std::vector<std::vector<std::vector<T>>> Binnedz( BlocksX*BlocksY, std::vector<std::vector<T>>(numberOfSlices) );
    std::vector<std::vector<std::vector<int>>>   BinnedA( BlocksX*BlocksY, std::vector<std::vector<int>>  (numberOfSlices) );

    int count_in_range = 0;
    for(int i = 0; i < atom_count; ++i) {
        if (HostZIDs[i] >= 0 && HostBlockIDs[i] >= 0) {
            Binnedx[HostBlockIDs[i]][HostZIDs[i]].push_back(AtomXPos[i]);
            Binnedy[HostBlockIDs[i]][HostZIDs[i]].push_back(AtomYPos[i]);
            Binnedz[HostBlockIDs[i]][HostZIDs[i]].push_back(AtomZPos[i]);
            BinnedA[HostBlockIDs[i]][HostZIDs[i]].push_back(AtomANum[i]);
            ++count_in_range;
        }
    }

    unsigned long long max_bin_xy = 0;
    unsigned long long max_bin_z = 0;

    //
    // This only works because x and y have the same number of blocks (so I only need to look at one of x or y)
    //
    for (auto &bx_1 : Binnedx) {
        if (bx_1.size() > max_bin_xy)
            max_bin_xy = bx_1.size();

        for (auto &bx_2 : bx_1)
            if (bx_2.size() > max_bin_z)
                max_bin_z = bx_2.size();
    }

    int atomIterator = 0;

    std::vector<int> blockStartPositions(numberOfSlices*BlocksX*BlocksY+1);

    // Put all bins into a linear block of memory ordered by z then y then x and record start positions for every block.
    CLOG(DEBUG, "sim") << "Putting binned atoms into continuous array";

    for(int slicei = 0; slicei < numberOfSlices; slicei++) {
        for(int j = 0; j < BlocksY; j++) {
            for(int k = 0; k < BlocksX; k++) {
                blockStartPositions[slicei*BlocksX*BlocksY+ j*BlocksX + k] = atomIterator;

                if(!Binnedx[j * BlocksX + k][slicei].empty()) {
                    for(int l = 0; l < Binnedx[j*BlocksX+k][slicei].size(); l++) {
                        AtomXPos[atomIterator] = Binnedx[j*BlocksX+k][slicei][l];
                        AtomYPos[atomIterator] = Binnedy[j*BlocksX+k][slicei][l];
                        AtomZPos[atomIterator] = Binnedz[j*BlocksX+k][slicei][l];
                        AtomANum[atomIterator] = BinnedA[j*BlocksX+k][slicei][l];
                        atomIterator++;
                    }
                }
            }
        }
    }

    // Last element indicates end of last block as total number of atoms.
    blockStartPositions[numberOfSlices*BlocksX*BlocksY] = count_in_range;

    ClBlockStartPositions = clMemory<int, Manual>(ctx, numberOfSlices * BlocksX * BlocksY + 1);

    CLOG(DEBUG, "sim") << "Writing binned atom posisitons to bufffers";

    // Now upload the sorted atoms onto the device..
    ClAtomX.Write(AtomXPos);
    ClAtomY.Write(AtomYPos);
    ClAtomZ.Write(AtomZPos);
    ClAtomA.Write(AtomANum);

    ClBlockStartPositions.Write(blockStartPositions);

    // wait for the IO queue here so that we are sure the data is uploaded before we start using it
    ctx.WaitForQueueFinish();
}

template <class T>
void SimulationGeneral<T>::initialiseSimulation() {
    CLOG(DEBUG, "sim") << "Initialising all buffers";
    initialiseBuffers();

    CLOG(DEBUG, "sim") << "Setting up all kernels";
    initialiseKernels();

    CLOG(DEBUG, "sim") << "Getting parameters";
    std::vector<double> params_d = job->simManager->getStructureParameterData();
    std::vector<T> params(params_d.begin(), params_d.end()); // TODO: avoid the copy if we are using a double type?
    CLOG(DEBUG, "sim") << "Uploading parameters";
    ClParameterisation.Write(params);

    CLOG(DEBUG, "sim") << "Starting general initialisation";
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Get local copies of variables (for convenience)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    auto current_pixel = job->getPixel();

    bool isFull3D = job->simManager->isFull3d();
    unsigned int resolution = job->simManager->getResolution();
    auto mParams = job->simManager->getMicroscopeParams();
    double wavenumber = mParams->Wavenumber();
    std::valarray<double> wavevector = mParams->Wavevector();
    double pixelscale = job->simManager->getRealScale();
    double startx = job->simManager->getPaddedSimLimitsX(current_pixel)[0];
    double starty = job->simManager->getPaddedSimLimitsY(current_pixel)[0];
    int full3dints = job->simManager->getFull3dInts();
    std::string param_name = job->simManager->getStructureParametersName();

    // Work out area that is to be simulated (in real space)
    double SimSizeX = pixelscale * resolution;
    double SimSizeY = SimSizeX;

    double sigma = mParams->Sigma() * wavenumber / wavevector[2];

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Set up our frequency calibrations
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLOG(DEBUG, "sim") << "Creating reciprocal space calibration";
    // This basically is all to create OpenCL buffers (1D) that let us know the frequency value of the pixels in the FFT
    // Not that this already accounts for the un-shifted nature of the FFT (i.e. 0 frequency is at 0, 0)
    // We also calculate our limit for low pass filtering the wavefunctions
    std::vector<T> k0x(resolution);
    std::vector<T> k0y(resolution);

    auto imidx = (unsigned int) std::floor(static_cast<double>(resolution) / 2.0 + 0.5);
    auto imidy = (unsigned int) std::floor(static_cast<double>(resolution) / 2.0 + 0.5);

    double temp;

    for (int i = 0; i < resolution; i++) {
        if (i >= imidx)
            temp = signed(i - resolution) / SimSizeX;
        else
            temp = i / SimSizeX;
        k0x[i] = temp;
    }

    for (int i = 0; i < resolution; i++) {
        if (i >= imidy)
            temp = signed(i - resolution) / SimSizeY;
        else
            temp = i / SimSizeY;
        k0y[i] = temp;
    }

    // Find maximum frequency for bandwidth limiting rule
    T kmaxx = std::abs(k0x[imidx]);
    T kmaxy = std::abs(k0y[imidy]);

    double bandwidthkmax = std::min(kmaxy, kmaxx);

    CLOG(DEBUG, "sim") << "Writing to buffers";
    // write our frequencies to OpenCL buffers
    clXFrequencies.Write(k0x);
    clYFrequencies.Write(k0y);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create a few buffers we will need later
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    clWorkGroup WorkSize(resolution, resolution, 1);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Set up FFT shift kernel
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLOG(DEBUG, "sim") << "Set up FFT shift kernel";

    // these will never change, so set them here
    fftShift.SetArg(0, clWaveFunction2[0], ArgumentType::Input);
    fftShift.SetArg(1, clWaveFunction3, ArgumentType::Output);
    fftShift.SetArg(2, resolution);
    fftShift.SetArg(3, resolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Set up low pass filter kernel
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLOG(DEBUG, "sim") << "Set up low pass filter kernel";

    // These never change, so set them here
    BandLimit.SetArg(0, clWaveFunction3, ArgumentType::InputOutput);
    BandLimit.SetArg(1, resolution);
    BandLimit.SetArg(2, resolution);
    BandLimit.SetArg(3, static_cast<T>(bandwidthkmax));
    BandLimit.SetArg(4, static_cast<T>(job->simManager->getInverseLimitFactor()));
    BandLimit.SetArg(5, clXFrequencies, ArgumentType::Input);
    BandLimit.SetArg(6, clYFrequencies, ArgumentType::Input);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Set up the kernels to calculate the atomic potentials
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLOG(DEBUG, "sim") << "Set up potential kernel";

    // Work out which blocks to load by ensuring we have the entire area around workgroup up to 5 angstroms away...
    // TODO: check this is doing what the above comment says it is doing...
    // TODO: I think the 8.0 and 3.0 should be the padding as set in the manager...

    int load_blocks_x = (int) std::ceil(8.0 / job->simManager->getBlockScaleX());
    int load_blocks_y = (int) std::ceil(8.0 / job->simManager->getBlockScaleY());

    double dz = job->simManager->getSliceThickness();
    int load_blocks_z = (int) std::ceil(3.0 / dz);

    // Set some of the arguments which dont change each iteration
    CalculateTransmissionFunction.SetArg(0, clTransmissionFunction, ArgumentType::Output);
    CalculateTransmissionFunction.SetArg(5, ClParameterisation, ArgumentType::Input);
    if (param_name == "kirkland")
        CalculateTransmissionFunction.SetArg(6, 0);
    else if (param_name == "peng")
        CalculateTransmissionFunction.SetArg(6, 1);
    else if (param_name == "lobato")
        CalculateTransmissionFunction.SetArg(6, 2);
    else
        throw std::runtime_error("Trying to use parameterisation I do not understand");
    CalculateTransmissionFunction.SetArg(8, resolution);
    CalculateTransmissionFunction.SetArg(9, resolution);
    CalculateTransmissionFunction.SetArg(13, static_cast<T>(dz));
    CalculateTransmissionFunction.SetArg(14, static_cast<T>(pixelscale)); // TODO: does this want to be different?
    CalculateTransmissionFunction.SetArg(15, job->simManager->getBlocksX());
    CalculateTransmissionFunction.SetArg(16, job->simManager->getBlocksY());
    CalculateTransmissionFunction.SetArg(17, static_cast<T>(job->simManager->getPaddedFullLimitsX()[1]));
    CalculateTransmissionFunction.SetArg(18, static_cast<T>(job->simManager->getPaddedFullLimitsX()[0]));
    CalculateTransmissionFunction.SetArg(19, static_cast<T>(job->simManager->getPaddedFullLimitsY()[1]));
    CalculateTransmissionFunction.SetArg(20, static_cast<T>(job->simManager->getPaddedFullLimitsY()[0]));
    CalculateTransmissionFunction.SetArg(21, load_blocks_x);
    CalculateTransmissionFunction.SetArg(22, load_blocks_y);
    CalculateTransmissionFunction.SetArg(23, load_blocks_z);
    CalculateTransmissionFunction.SetArg(24, static_cast<T>(sigma)); // Not sure why I am using this sigma and not commented sigma...
    CalculateTransmissionFunction.SetArg(25, static_cast<T>(startx));
    CalculateTransmissionFunction.SetArg(26, static_cast<T>(starty));
    if (isFull3D) {
        double int_shift_x = (wavevector[0] / wavevector[2]) * dz / full3dints;
        double int_shift_y = (wavevector[1] / wavevector[2]) * dz / full3dints;

        CalculateTransmissionFunction.SetArg(27, static_cast<T>(int_shift_x));
        CalculateTransmissionFunction.SetArg(28, static_cast<T>(int_shift_y));
        CalculateTransmissionFunction.SetArg(29, full3dints);
    } else {
        CalculateTransmissionFunction.SetArg(27, static_cast<T>(mParams->BeamTilt));
        CalculateTransmissionFunction.SetArg(28, static_cast<T>(mParams->BeamAzimuth));
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Set up the propagator
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLOG(DEBUG, "sim") << "Set up propagator kernel";

    GeneratePropagator.SetArg(0, clPropagator, ArgumentType::Output);
    GeneratePropagator.SetArg(1, clXFrequencies, ArgumentType::Input);
    GeneratePropagator.SetArg(2, clYFrequencies, ArgumentType::Input);
    GeneratePropagator.SetArg(3, resolution);
    GeneratePropagator.SetArg(4, resolution);
    GeneratePropagator.SetArg(5, static_cast<T>(dz)); // Is this the right dz? (Propagator needs slice thickness not spacing between atom bins)
    GeneratePropagator.SetArg(6, static_cast<T>(wavenumber));
    GeneratePropagator.SetArg(7, static_cast<T>(wavevector[0]));
    GeneratePropagator.SetArg(8, static_cast<T>(wavevector[1]));
    GeneratePropagator.SetArg(9, static_cast<T>(wavevector[2]));
    GeneratePropagator.SetArg(10, static_cast<T>(bandwidthkmax * job->simManager->getInverseLimitFactor()));

    // actually run this kernel now
    GeneratePropagator.run(WorkSize);
    ctx.WaitForQueueFinish();


    CLOG(DEBUG, "sim") << "Set up complex multiply kernel";

    ComplexMultiply.SetArg(3, resolution);
    ComplexMultiply.SetArg(4, resolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Finally, sort our atoms!
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLOG(DEBUG, "sim") << "Sorting atoms";
    sortAtoms();
}

template <class T>
void SimulationGeneral<T>::doMultiSliceStep(int slice)
{
    CLOG(DEBUG, "sim") << "Start multislice step " << slice;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create local variables for convenience
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    double dz = job->simManager->getSliceThickness();
    unsigned int resolution = job->simManager->getResolution();
    // in the current format, Tds is handled by job splitting so this is always 1??
    int n_parallel = job->simManager->getParallelPixels(); // total number of parallel pixels
    auto z_lim = job->simManager->getPaddedStructLimitsZ();

    // Didn't have MinimumZ so it wasnt correctly rescaled z-axis from 0 to SizeZ...
    double currentz = z_lim[1] - slice * dz;

    clWorkGroup Work(resolution, resolution, 1);
    clWorkGroup LocalWork(16, 16, 1);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Get our *transmission* function for the current slice
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned int numberOfSlices = job->simManager->getNumberofSlices();

    CalculateTransmissionFunction.SetArg(1, ClAtomX, ArgumentType::Input);
    CalculateTransmissionFunction.SetArg(2, ClAtomY, ArgumentType::Input);
    CalculateTransmissionFunction.SetArg(3, ClAtomZ, ArgumentType::Input);
    CalculateTransmissionFunction.SetArg(4, ClAtomA, ArgumentType::Input);
    CalculateTransmissionFunction.SetArg(7, ClBlockStartPositions, ArgumentType::Input);
    CalculateTransmissionFunction.SetArg(10, slice);
    CalculateTransmissionFunction.SetArg(11, numberOfSlices);
    CalculateTransmissionFunction.SetArg(12, static_cast<T>(currentz));

    CLOG(DEBUG, "sim") << "Calculating potentials";

    CalculateTransmissionFunction.run(Work, LocalWork);

    ctx.WaitForQueueFinish();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Apply low pass filter to transmission function
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLOG(DEBUG, "sim") << "FFT transmission function";
    FourierTrans.run(clTransmissionFunction, clWaveFunction3, Direction::Forwards);
    ctx.WaitForQueueFinish();
    CLOG(DEBUG, "sim") << "Band limit transmission function";
    BandLimit.run(Work);
    ctx.WaitForQueueFinish();
    CLOG(DEBUG, "sim") << "IFFT band limited transmission function";
    FourierTrans.run(clWaveFunction3, clTransmissionFunction, Direction::Inverse);
    ctx.WaitForQueueFinish();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Propogate slice
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (int i = 1; i <= n_parallel; i++)
    {
        CLOG(DEBUG, "sim") << "Propogating (" << i << " of " << n_parallel << " parallel)";

        // Multiply transmission function with wavefunction
        ComplexMultiply.SetArg(0, clTransmissionFunction, ArgumentType::Input);
        ComplexMultiply.SetArg(1, clWaveFunction1[i - 1], ArgumentType::Input);
        ComplexMultiply.SetArg(2, clWaveFunction2[i - 1], ArgumentType::Output);
        CLOG(DEBUG, "sim") << "Multiply wavefunction and potentials";
        ComplexMultiply.run(Work);
        ctx.WaitForQueueFinish();

        // go to reciprocal space
        CLOG(DEBUG, "sim") << "FFT to reciprocal space";
        FourierTrans.run(clWaveFunction2[i - 1], clWaveFunction3, Direction::Forwards);
        ctx.WaitForQueueFinish();

        // convolve with propagator
        ComplexMultiply.SetArg(0, clWaveFunction3, ArgumentType::Input);
        ComplexMultiply.SetArg(1, clPropagator, ArgumentType::Input);
        ComplexMultiply.SetArg(2, clWaveFunction2[i - 1], ArgumentType::Output);
        CLOG(DEBUG, "sim") << "Convolve with propagator";
        ComplexMultiply.run(Work);
        ctx.WaitForQueueFinish();

        // IFFT back to real space
        CLOG(DEBUG, "sim") << "IFFT to real space";
        FourierTrans.run(clWaveFunction2[i - 1], clWaveFunction1[i - 1], Direction::Inverse);
        ctx.WaitForQueueFinish();
    }
}

template <class T>
std::vector<double> SimulationGeneral<T>::getDiffractionImage(int parallel_ind)
{
    CLOG(DEBUG, "sim") << "Getting diffraction image";
    unsigned int resolution = job->simManager->getResolution();
    std::vector<double> data_out(resolution * resolution);

    // Original data is complex so copy complex version down first
    clWorkGroup Work(resolution, resolution, 1);

    CLOG(DEBUG, "sim") << "FFT shifting diffraction pattern";
    fftShift.SetArg(0, clWaveFunction2[parallel_ind], ArgumentType::Input);
    fftShift.run(Work);

    CLOG(DEBUG, "sim") << "Copy from buffer";
    std::vector<std::complex<T>> compdata = clWaveFunction3.CreateLocalCopy();

    // TODO: this could be done on GPU?
    CLOG(DEBUG, "sim") << "Calculating absolute squared value";
    for (int i = 0; i < resolution * resolution; i++)
        // Get absolute value for display...
        data_out[i] = std::norm(compdata[i]); // norm is square amplitude

    return data_out;
}

template <class T>
std::vector<double> SimulationGeneral<T>::getExitWaveImage(unsigned int t, unsigned int l, unsigned int b, unsigned int r) {
    CLOG(DEBUG, "sim") << "Getting exit wave image";
    unsigned int resolution = job->simManager->getResolution();
    std::vector<double> data_out(2*((resolution - t - b) * (resolution - l - r)));

    CLOG(DEBUG, "sim") << "Copy from buffer";
    std::vector<std::complex<T>> compdata = clWaveFunction1[0].CreateLocalCopy();

    CLOG(DEBUG, "sim") << "Process complex data";
    int cnt = 0;
    for (int j = 0; j < resolution; ++j)
        if (j >= b && j < (resolution - t))
            for (int i = 0; i < resolution; ++i)
                if (i >= l && i < (resolution - r)) {
                    int k = i + j * resolution;
                    data_out[cnt] = compdata[k].real();
                    data_out[cnt + 1] = compdata[k].imag();
                    cnt += 2;
                }

    return data_out;
}

template class SimulationGeneral<float>;
template class SimulationGeneral<double>;