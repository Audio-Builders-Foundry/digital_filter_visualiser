#include "../src/JUCE/modules/juce_core/juce_core.h"
#include "RootsToCoefficientsTest.h"
#include "ProcessorChainModifierTest.h"
#include "PhaseFrequencyResponseTest.h"
#include "CoefficientsToRootsTest.h"
#include "CoefficientsToRootsDistanceTest.h"

//==============================================================================
int main (int argc, char* argv[])
{
    juce::ignoreUnused(argc);
    juce::ignoreUnused(argv);

    FILE *destFile = nullptr;
    if(argc > 1)
    {
      destFile = fopen(argv[1], "w");
    }

    ProfileConfig profCfg{};
    profCfg.fmt = ProfileFormat::csv;
    profCfg.dst.userData = destFile;
    profCfg.dst.onTopScopeExit = [](void *userData){
      auto *data = profileFlushLog();
      if(userData != nullptr)
      {
	auto *dst = reinterpret_cast<FILE *>(userData);
	auto dataSize = strlen(data);
	auto sizeWritten = fwrite(data, 1, dataSize, dst);
	jassert(sizeWritten == dataSize);
      }
    };
    profileConfigure(profCfg);

    juce::ScopedJuceInitialiser_GUI libraryInitialiser; // for proper processor initialization in test classes
    juce::UnitTestRunner runner;
    runner.runAllTests();

    std::cout << "\n===== All tests complete =====\n"<<std::endl;

    CoefficientsToRootsTest::printReport();
    std::cout<<"\n------------------------------\n"<<std::endl;
    CoefficientsToRootsDistanceTest::printReport();

    if(destFile != nullptr)
    {
      fclose(destFile);
    }

    return 0;
}
