
# ESSS

from https://github.com/csl-ugent/ESSS



📁 ESSS
│ ├── 📁 analyzer [the ESSS tool source code]
│ │   │ ├── 📃 Makefile [adapted from Crix]
│ │   │ └── 📁 src
│ │   │     │ ├── 📃 ...
│ │   │     │ └── 📁 src
│ │   │     │     │ ├── 📃 Analyzer.{cc, h} [Entry point of the application, adapted from Crix]
│ │   │     │     │ ├── 📃 CallGraph.{cc, h} [MLTA component from Crix]
│ │   │     │     │ ├── 📃 ClOptForward.h [Forward declarations of command line options]
│ │   │     │     │ ├── 📃 Common.{cc, h} [Common utility functions, adapted from Crix]
│ │   │     │     │ ├── 📃 DataFlowAnalysis.{cc, h} [Dataflow analysis helpers]
│ │   │     │     │ ├── 📃 DebugHelpers.{cc, h} [Debugging helpers]
│ │   │     │     │ ├── 📃 EHBlockDetector.{cc, h} [Specification inference component]
│ │   │     │     │ ├── 📃 ErrorCheckViolationFinder.{cc, h} [Bug detection component]
│ │   │     │     │ ├── 📃 FunctionErrorReturnIntervals.{cc, h} [Data structure file]
│ │   │     │     │ ├── 📃 FunctionVSA.{cc, h} [Value set analysis of return values component]
│ │   │     │     │ ├── 📃 Helpers.{cc, h} [Common utility functions]
│ │   │     │     │ ├── 📃 Interval.{cc, h} [Interval data structure]
│ │   │     │     │ ├── 📃 Lazy.h [Lazy execution utility class]
│ │   │     │     │ ├── 📃 MLTA.{cc, h} [MLTA component from Crix]
│ │   │     │     │ └── 📃 PathSpan.h [Data structure to store (parts of) paths]