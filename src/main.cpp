#include "cacheSimulator.h"
#include<fstream>
#include<string>
#include<cstdlib>

#define TRACE_DIR_PATH "trace_files/"


int main(int argc, char* argv[])
{
    uint l1_size, l1_assoc, l1_blocksize, n_vc_blocks, l2_size, l2_assoc;
    string traceFileName;
    cout << argc << endl;

    if(argc == 8)
    {
        l1_size = atoi(argv[1]);
        l1_assoc = atoi(argv[2]);
        l1_blocksize = atoi(argv[3]);
        n_vc_blocks = atoi(argv[4]);
        l2_size = atoi(argv[5]);
        l2_assoc = atoi(argv[6]);
        traceFileName = argv[7];

        CacheSimulator cache_sim = CacheSimulator(l1_size, l1_assoc, l1_blocksize, n_vc_blocks, l2_size, l2_assoc, traceFileName);

        string traceFilePath = TRACE_DIR_PATH + traceFileName;
        ifstream traceFile(traceFilePath);

        if(traceFile.is_open())
        {
            while(true)
            {
                string s1, s2;
                traceFile >> s1 >> s2;

                if(traceFile.fail() && !traceFile.eof())
                {
                    cerr << "Invalid input or formatting in trace file" << endl;
                    exit(EXIT_FAILURE);
                }
                else if(traceFile.eof())
                {
                    break;
                }

                if(s1 == "r")
                {
                    long long int addr = std::stoll(s2, nullptr, 16);
                    // cout << "r " << hex << addr << endl;
                    cache_sim.sendReadRequest(addr);
                }
                else if(s1 == "w")
                {
                    long long int addr = std::stoll(s2, nullptr, 16);
                    cache_sim.sendWriteRequest(addr);
                }
                else
                {
                    cerr << "Invalid input or formatting in trace file" << endl;
                    exit(EXIT_FAILURE);                
                }
            }
        }
        else
        {
            cerr << "Error in opening file - " << traceFilePath << endl;
        }

        SimulationStatistics cache_sim_stats = cache_sim.getSimulationStats();

        cache_sim.printSimulatorConfiguration();
        cache_sim.printCacheContents();
        
        SimulationStatistics sim_stats = cache_sim.getSimulationStats();
        sim_stats.printStats();
    }
    return 0;
}