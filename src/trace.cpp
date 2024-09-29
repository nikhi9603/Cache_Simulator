// #include "trace.h"
// #define TRACE_DIR_PATH "trace_files/"

// Trace::Trace(string fileName)
// {
//     this->traceFileName = fileName;
// }


// vector<TraceEntry> Trace::parseTraceFile()
// {
//     vector<TraceEntry> trace_contents;
//     string traceFilePath = TRACE_DIR_PATH + traceFileName;
//     ifstream traceFile(traceFilePath);

//     if(traceFile.is_open())
//     {
//         while(true)
//         {
//             string s1, s2;
//             traceFile >> s1 >> s2;

//             if(traceFile.fail() && !traceFile.eof())
//             {
//                 cerr << "Invalid input or formatting in trace file (Line: " << trace_contents.size()+1 << ")" << endl;
//                 exit(EXIT_FAILURE);
//             }
//             else if(traceFile.eof())
//             {
//                 break;
//             }

//             if(s1 == "r" || s1 == "w")
//             {
//                 uint64_t addr = std::stoull(s2, nullptr, 16);
//                 TraceEntry newTraceEntry = TraceEntry(s1, addr);
//                 trace_contents.push_back(newTraceEntry);
//             }
//             else
//             {
//                 cerr << "Invalid input or formatting in trace file (Line: " << trace_contents.size()+1 << ")" << endl;
//                 exit(EXIT_FAILURE);                
//             }
//         }
//     }
//     else
//     {
//         cerr << "Error in opening file - " << traceFilePath << endl;
//     }

//     return trace_contents;
// }