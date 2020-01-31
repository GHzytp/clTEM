#include <utility>

//
// Created by jon on 23/11/17.
//

#ifndef CLTEM_SIMULATIONWORKER_H
#define CLTEM_SIMULATIONWORKER_H

#include <complex>

#include "threading/threadworker.h"
#include "clwrapper.h"
#include "utilities/logging.h"

template <class GPU_Type>
class SimulationWorker : public ThreadWorker
{
private:

    SimulationMode last_mode;
    bool last_do_3d;
    bool do_initialise;

    void initialiseBuffers();
    void initialiseKernels();

public:
    // initialise FourierTrans just with any values
    SimulationWorker(ThreadPool &s, unsigned int _id, const clContext &_ctx) : ThreadWorker(s, _id), ctx(_ctx), last_mode(SimulationMode::None), last_do_3d(false), do_initialise(true) {}

    ~SimulationWorker() {ctx.WaitForQueueFinish(); ctx.WaitForIOQueueFinish();}

    void Run(const std::shared_ptr<SimulationJob> &_job) override;

private:
    clContext ctx;

    std::shared_ptr<SimulationManager> current_manager;

    std::shared_ptr<SimulationJob> job;

    void sortAtoms();

    void doCtem();

    void doCbed();

    void doStem();

    void initialiseSimulation();

    void initialiseCtem();

    void initialiseProbeWave(double posx, double posy, int n_parallel = 0);

    void doMultiSliceStep(int slice);

    void simulateCtemImage();

    void simulateCtemImagePerfect();

    void simulateCtemImageDose(std::vector<GPU_Type> dqe_data, std::vector<GPU_Type> ntf_data, int binning, double doseperpix, double conversionfactor = 1);

    std::vector<double> getDiffractionImage(int parallel_ind = 0);

    std::vector<double> getExitWaveImage(unsigned int t = 0, unsigned int l = 0, unsigned int b = 0, unsigned int r = 0);

    std::vector<double> getCtemImage();

    double doSumReduction(clMemory<GPU_Type, Manual> data, clWorkGroup globalSizeSum,
                         clWorkGroup localSizeSum, unsigned int nGroups, int totalSize);

    double getStemPixel(double inner, double outer, double xc, double yc, int parallel_ind);

    // OpenCL stuff
    clMemory<GPU_Type, Manual> ClParameterisation;

    clMemory<GPU_Type, Manual> ClAtomX;
    clMemory<GPU_Type, Manual> ClAtomY;
    clMemory<GPU_Type, Manual> ClAtomZ;
    clMemory<int, Manual> ClAtomA;

    clMemory<int, Manual> ClBlockStartPositions;
    clMemory<int, Manual> ClBlockIds;
    clMemory<int, Manual> ClZIds;

    std::vector<clMemory<std::complex<GPU_Type>, Manual>> clWaveFunction1;
    std::vector<clMemory<std::complex<GPU_Type>, Manual>> clWaveFunction2;
    clMemory<std::complex<GPU_Type>, Manual> clWaveFunction3;
    std::vector<clMemory<std::complex<GPU_Type>, Manual>> clWaveFunction4;

    clMemory<GPU_Type, Manual> clXFrequencies;
    clMemory<GPU_Type, Manual> clYFrequencies;
    clMemory<std::complex<GPU_Type>, Manual> clPropagator;
    clMemory<std::complex<GPU_Type>, Manual> clTransmissionFunction;
    clMemory<std::complex<GPU_Type>, Manual> clImageWaveFunction;

    // General kernels
    clFourier<GPU_Type> FourierTrans;
    clKernel AtomSort;
    clKernel BandLimit;
    clKernel fftShift;
    clKernel CalculateTransmissionFunction;
    clKernel GeneratePropagator;
    clKernel ComplexMultiply;

    // CTEM
    clKernel InitPlaneWavefunction;
    clKernel ImagingKernel;
    clKernel ABS2;
    clKernel NtfKernel;
    clKernel DqeKernel;
    clMemory<GPU_Type, Manual> clCcdBuffer;
    clMemory<std::complex<GPU_Type>, Manual> clTempBuffer;

    // CBED
    clKernel InitProbeWavefunction;
    clMemory<GPU_Type, Manual> clTDSMaskDiff;

    // STEM
    clKernel TDSMaskingAbsKernel;
    clKernel SumReduction;
    clMemory<GPU_Type, Manual> clReductionBuffer;
};


#endif //CLTEM_SIMULATIONWORKER_H
