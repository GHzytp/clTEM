//
// Created by jon on 02/08/17.
//

#ifndef CLTEM_SIMULATIONRUNNER_H
#define CLTEM_SIMULATIONRUNNER_H

#include <functional>
#include "simulationmanager.h"
#include "simulationjob.h"
#include "threadpool.h"

#include "utilities/logging.h"

class SimulationRunner
{
public:

    SimulationRunner(std::vector<std::shared_ptr<SimulationManager>> mans, std::vector<clDevice> devs, bool double_precision);

    ~SimulationRunner() {cancelSimulation();}

    void runSimulations();

    void cancelSimulation() {
        start = false;
        if (t_pool)
            t_pool->stopThreads();
    }

private:

    bool start;

    void runSingle(std::shared_ptr<SimulationManager> sim_pointer);

    std::vector<std::shared_ptr<SimulationManager>> managers;

    std::vector<clDevice> dev_list;

    std::unique_ptr<ThreadPool> t_pool;

    bool use_double_precision;

    std::vector<std::shared_ptr<SimulationJob>> SplitJobs(std::shared_ptr<SimulationManager> simManager);
};


#endif //CLTEM_SIMULATIONRUNNER_H
