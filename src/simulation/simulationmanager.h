#include <utility>

#include <utility>

#include <utility>

#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include <string>
#include <mutex>
#include <map>
#include <valarray>
#include "incoherence/inelastic/phonon.h"
#include "incoherence/inelastic/plasmon.h"
#include <incoherence/incoherenteffects.h>
#include <structure/simulationcell.h>

#include "structure/structureparameters.h"
#include "utilities/commonstructs.h"
#include "utilities/enums.h"
#include "utilities/stringutils.h"
#include "utilities/logging.h"

class SimulationManager
{
public:
    // Constructors
    //
    // The copy/assignment are particularly important as we want to create our own copy of things like the structure
    // classes, not just copy pointers to the same thing

    SimulationManager();

    SimulationManager(const SimulationManager& sm);

    SimulationManager& operator=(const SimulationManager& sm);

    // getters

    std::shared_ptr<SimulationCell> simulationCell() {return simulation_cell;}

    std::shared_ptr<MicroscopeParameters> microscopeParams() {return micro_params;}

    std::shared_ptr<SimulationArea> simulationArea() {return sim_area;}

    std::vector<StemDetector>& stemDetectors() {return stem_dets;}

    std::shared_ptr<StemArea> stemArea() {return stem_sim_area;}

    std::shared_ptr<CbedPosition> cbedPosition() {return cbed_pos;}

    SimulationArea ctemArea() {return *sim_area;}

    std::shared_ptr<IncoherentEffects> incoherenceEffects() {return incoherence_effects;}

    // structure setters
    void setStructure(std::string fPath, CIF::SuperCellInfo info = CIF::SuperCellInfo(), bool fix_cif=false);
    void setStructure(CIF::CIFReader cif, CIF::SuperCellInfo info);

    // resolution
    unsigned int resolution() {return sim_resolution;}
    void setResolution(unsigned int res) { sim_resolution = res;}
    bool resolutionValid();

    // mode
    SimulationMode mode(){return simulation_mode;}
    bool isProbeSimulation() {return simulation_mode == SimulationMode::STEM || simulation_mode == SimulationMode::CBED;}
    void setMode(SimulationMode md){ simulation_mode = md;}
    std::string modeString() {return Utils::simModeToString(simulation_mode);}

    // full 3d integrals
    bool full3dEnabled(){return use_full_3d;}
    void setFull3dEnabled(bool use) { use_full_3d = use;}
    unsigned int full3dIntegrals(){return full_3d_integrals;}
    void setFull3dIntegrals(unsigned int n3d){ full_3d_integrals= n3d;}

    // scales
    /// Get the simulation scale in Angstroms per pixel
    double realScale();
    /// Get the simulation inverse scale in inverse Angstroms per pixel
    double inverseScale();
    /// GGet the max band limited simulation inverse in inverse Angstroms per pixel
    double inverseMax();
    /// Get the simulation inverse scale in mrad per pixel
    double inverseScaleAngle();
    /// Get the max band limited simulation inverse in mrad
    double inverseMaxAngle();
    /// Get the factor to band limit to avoid aliasing issues
    double inverseLimitFactor() {return max_inverse_factor;}

    //
    bool ctemImageEnabled() {return simulate_ctem_image;}
    void setCtemImageEnabled(bool val) { simulate_ctem_image = val;}

    std::string ccdName() {return ccd_name;}
    void setCcdName(std::string nm) {ccd_name = std::move(nm);}

    int ccdBinning() {return ccd_binning;}
    void setCcdBinning(int bin) {ccd_binning = bin;}

    double const ccdDose() {return ccd_dose;}
    void setCcdDose(double dose) {ccd_dose = dose;}

    //
    bool maintainAreas() {return maintain_area;}
    void setMaintainAreas(bool maintain) {maintain_area = maintain;}

    bool doublePrecisionEnabled() {return use_double_precision;}
    void setDoublePrecisionEnabled(bool ddp) {use_double_precision = ddp;}

    unsigned int intermediateSliceStep() {return intermediate_slices_enabled ? intermediate_slices : 0;}
    unsigned int storedIntermediateSliceStep() {return intermediate_slices;}
    void setIntermediateSlices(unsigned int is) {intermediate_slices = is;}

    bool intermediateSlicesEnabled() {return intermediate_slices_enabled;}
    void setIntermediateSlicesEnabled(bool ise) {intermediate_slices_enabled = ise;}

    unsigned int parallelPixels() {return (simulation_mode != SimulationMode::STEM) ? 1 : parallel_pixels;}
    void setParallelPixels(unsigned int npp) { parallel_pixels = npp;}

    unsigned long totalParts();

    int blocksX();
    int blocksY();

    double blockScaleX();
    double blockScaleY();

    // should have corrected the shift issue so this more intuitive version works!!
    // this covers only the area that is needed right now (i.e. for just this pixel)
    std::valarray<double> paddedSimLimitsX(int pixel);
    std::valarray<double> paddedSimLimitsY(int pixel);
    std::valarray<double> paddedSimLimitsZ();

    // this one covers the total sim area (even if it is not all needed for each simulation, ie area covered by all pixels)
    std::valarray<double> paddedFullLimitsX();
    std::valarray<double> paddedFullLimitsY();

    std::valarray<double> rawSimLimitsX(int pixel);
    std::valarray<double> rawSimLimitsY(int pixel);

    std::valarray<double> rawFullLimitsX();
    std::valarray<double> rawFullLimitsY();

    //
    std::tuple<double, double, double, int> simRanges();

    std::valarray<unsigned int> imageCrop();

    //
    Parameterisation structureParameters() {return StructureParameters::getParameters(structure_parameters_name);}
//    std::vector<double> structureParametersData() {return StructureParameters::getParametersData(structure_parameters_name);}
//    std::string structureParametersName() {return structure_parameters_name;}
    void setStructureParameters(std::string name) { structure_parameters_name = std::move(name); }

    // Return functions
    //

    void setImageReturnFunc(std::function<void(SimulationManager)> f) { image_return_func = std::move(f);}
    void setProgressTotalReporterFunc(std::function<void(double)> f) { report_progress_total_func = std::move(f);}
    void setProgressSliceReporterFunc(std::function<void(double)> f) { report_progress_slice_func = std::move(f);}

    std::map<std::string, Image<double>> images() { return image_container; }
    void updateImages(std::map<std::string, Image<double>> &ims, int jobCount, bool update=false);
    void failedSimulation();

    void reportTotalProgress(double prog);
    void reportSliceProgress(double prog);

    bool allPartsCompleted() {return complete_jobs == totalParts();}

    bool liveStemEnabled() {
        return live_stem;
    }
    void setLiveStemEnabled(bool enable) {
        live_stem = enable;
    }

private:
    // simulation cell contains the structure
    std::shared_ptr<SimulationCell> simulation_cell;

    // inelastic effects such as phonons and plasmons
    std::shared_ptr<IncoherentEffects> incoherence_effects;

    std::shared_ptr<MicroscopeParameters> micro_params;

    std::shared_ptr<SimulationArea> sim_area;

    std::vector<StemDetector> stem_dets;

    std::shared_ptr<StemArea> stem_sim_area;

    std::shared_ptr<CbedPosition> cbed_pos;

    //

    // full area covers the entire possible sim area (i.e. ALL pixels for STEM)
    SimulationArea fullAreaBase();

    // current area covers the current needed sim area (i.e. only the current pixel for STEM)
    SimulationArea currentAreaBase(int pixel);

    void calculateBlocks();

    //

    bool live_stem;

    //

    bool use_double_precision;

    bool maintain_area;

    bool simulate_ctem_image;

    bool use_full_3d;
    unsigned int full_3d_integrals;

    bool intermediate_slices_enabled;
    unsigned int intermediate_slices;

    std::string ccd_name;
    int ccd_binning;
    double ccd_dose;

    std::string structure_parameters_name;

    unsigned int sim_resolution;

    double max_inverse_factor;

    unsigned int parallel_pixels;

    int blocks_x, blocks_y;

    SimulationMode simulation_mode;

    std::mutex structure_mutex;

    // Data return

    unsigned int complete_jobs;

    std::mutex image_update_mutex;

    std::function<void(SimulationManager)> image_return_func;
    std::function<void(double)> report_progress_total_func;
    std::function<void(double)> report_progress_slice_func;

    std::map<std::string, Image<double>> image_container;


};

#endif // SIMULATIONMANAGER_H
