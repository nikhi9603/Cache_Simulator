#ifndef TRACE_H
#define TRACE_H

#include<iostream>
#include<vector>
#include<string>
#include<fstream>
using namespace std;

class Trace
{
private:
    string traceFileName;
    uint n_trace_contents;
    vector<pair<string, uint64_t>> trace_contents;

public:
    Trace(string fileName) {}
    vector<pair<string, uint64_t>> parseTraceFile();
};

#endif