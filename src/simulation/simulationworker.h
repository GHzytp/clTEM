#include <utility>

//
// Created by jon on 23/11/17.
//

#ifndef CLTEM_SIMULATIONWORKER_H
#define CLTEM_SIMULATIONWORKER_H


#include "threadworker.h"
#include "clwrapper.h"
#include "utilities/logging.h"

class SimulationWorker : public ThreadWorker
{
public:
    // initialise FourierTrans just with any values
    SimulationWorker(ThreadPool &s, unsigned int _id, std::shared_ptr<clContext> _ctx) : ThreadWorker(s, _id), ctx(std::move(_ctx)) {}

    ~SimulationWorker() = default;

    void Run(const std::shared_ptr<SimulationJob> &_job) override;

private:
    std::shared_ptr<clContext> ctx;

    std::shared_ptr<SimulationJob> job;

    void uploadParameters(std::vector<float> param);

    void sortAtoms(bool doTds = false);

    void doCtem(bool simImage = false);

    void doCbed();

    void doStem();

    void initialiseSimulation();

    void initialiseCtem();

    void initialiseCbed();

    void initialiseStem();

    void initialiseProbeWave(float posx, float posy, int n_parallel = 0);

    void doMultiSliceStep(int slice);

    void simulateCtemImage();

    void simulateCtemImage(std::vector<float> dqe_data, std::vector<float> ntf_data, int binning, float doseperpix,
                           float conversionfactor = 1);

    std::vector<float> getDiffractionImage(int parallel_ind = 0);

    std::vector<float> getExitWaveImage(unsigned int t = 0, unsigned int l = 0, unsigned int b = 0, unsigned int r = 0);

    std::vector<float> getCtemImage();

    float doSumReduction(std::shared_ptr<clMemory<float, Manual>> data, clWorkGroup globalSizeSum,
                         clWorkGroup localSizeSum, unsigned int nGroups, int totalSize);

    float getStemPixel(float inner, float outer, float xc, float yc, int parallel_ind);

    // OpenCL stuff
    std::shared_ptr<clMemory<float, Manual>> ClParameterisation;

    std::shared_ptr<clMemory<float, Manual>> ClAtomX;
    std::shared_ptr<clMemory<float, Manual>> ClAtomY;
    std::shared_ptr<clMemory<float, Manual>> ClAtomZ;
    std::shared_ptr<clMemory<int, Manual>> ClAtomA;

    std::shared_ptr<clMemory<int, Manual>> ClBlockStartPositions;
    std::shared_ptr<clMemory<int, Manual>> ClBlockIds;
    std::shared_ptr<clMemory<int, Manual>> ClZIds;

    std::vector<std::shared_ptr<clMemory<cl_float2, Manual>>> clWaveFunction1;
    std::vector<std::shared_ptr<clMemory<cl_float2, Manual>>> clWaveFunction2;
    std::shared_ptr<clMemory<cl_float2, Manual>> clWaveFunction3;
    std::vector<std::shared_ptr<clMemory<cl_float2, Manual>>> clWaveFunction4;

    std::shared_ptr<clMemory<float, Manual>> clXFrequencies;
    std::shared_ptr<clMemory<float, Manual>> clYFrequencies;
    std::shared_ptr<clMemory<cl_float2, Manual>> clPropagator;
    std::shared_ptr<clMemory<cl_float2, Manual>> clPotential;
    std::shared_ptr<clMemory<cl_float2, Manual>> clImageWaveFunction;

    // General kernels
    std::shared_ptr<clFourier> FourierTrans;
    std::shared_ptr<clKernel> BandLimit;
    std::shared_ptr<clKernel> fftShift;
    std::shared_ptr<clKernel> BinnedAtomicPotential;
    std::shared_ptr<clKernel> GeneratePropagator;
    std::shared_ptr<clKernel> ComplexMultiply;

    // CTEM
    std::shared_ptr<clKernel> InitPlaneWavefunction;
    std::shared_ptr<clKernel> ImagingKernel;
    std::shared_ptr<clKernel> ABS2;
    
    // CBED
    std::shared_ptr<clKernel> InitProbeWavefunction;
    std::shared_ptr<clMemory<float, Manual>> clTDSMaskDiff;

    // STEM
    std::shared_ptr<clKernel> TDSMaskingAbsKernel;
    std::shared_ptr<clKernel> SumReduction;
};


#endif //CLTEM_SIMULATIONWORKER_H
