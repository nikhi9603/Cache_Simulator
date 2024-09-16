#ifndef TRACE_H
#define TRACE_H

#include<iostream>
#include<vector>
#include<string>
#include<fstream>
using namespace std;

struct TraceEntry
{
    string operation;
    uint64_t addr;

    TraceEntry(string op, uint64_t addr) {operation = op; this->addr = addr;}
};


class Trace
{
private:
    string traceFileName;
    uint n_trace_contents;
    vector<TraceEntry> trace_contents;
    vector<TraceEntry> parseTraceFile();
public:
    Trace(string fileName) {}
    vector<TraceEntry> getTraceContents() {return trace_contents;}
};

#endif