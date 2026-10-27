// NOTE(ry): implementation

#if OS_WINDOWS
#  define NOMINMAX
#  include <windows.h>
#endif

#include <stdarg.h>

static ProfileConfig profileConfig{
  ProfileFormat::tui,
  {&std::cerr,
#if OS_WINDOWS
   [](void *data){
     juce::ignoreUnused(data);
     OutputDebugStringA(profileFlushLog());
     OutputDebugStringA("\n");
   }
#else
   [](void *data){
     auto outStream = reinterpret_cast<std::ostream*>(data);
     *outStream << std::string_view(profileFlushLog());
   }
#endif
  }
};

void
profileConfigure(ProfileConfig cfg)
{
  profileConfig = cfg;
}

thread_local Profiler threadProfiler = {};

struct ProfilerLog
{
  size_t logAt;
  char logBuffer[1 << 15];
};

thread_local ProfilerLog threadProfilerLog = {};

char*
profileFlushLog(void)
{
  threadProfilerLog.logBuffer[threadProfilerLog.logAt] = 0;
  threadProfilerLog.logAt = 0;
  return threadProfilerLog.logBuffer;
}

static void
profileLogFormatString(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  jassert(threadProfilerLog.logAt <= sizeof(threadProfilerLog.logBuffer));
  size_t bytesRemaining = sizeof(threadProfilerLog.logBuffer) - threadProfilerLog.logAt;
  char *buffer = &threadProfilerLog.logBuffer[0] + threadProfilerLog.logAt;
  int bytesWritten = vsnprintf(buffer, bytesRemaining, fmt, args);
  threadProfilerLog.logAt += size_t(bytesWritten);

  va_end(args);
}

static u64 tscRead(void);
static u64 tscGetFreq(void);

ProfileSite *Profiler::sites = profileSiteArrayBase();
u64 Profiler::siteCount = profileSiteArrayCount();
u64 Profiler::tscFreq = tscGetFreq();
size_t Profiler::maxSiteLabelLength = 0;
r64 Profiler::tscPeriod = 1.0 / Profiler::tscFreq;

// TODO(ry): support actual cycle PMCs, not just fixed-frequency counters
#if CPU_X86 || CPU_X64
#  if COMPILER_MSVC
#    include <intrin.h>
#  else
#    include <x86intrin.h>
#  endif
static u64
tscRead(void)
{
  return(__rdtsc());
}

static u64 osReadTimer(void);
static u64 osGetTimerFreq(void);

static u64
tscGetFreq(void)
{
  u64 const msToWait = 100;
  u64 osFreq = osGetTimerFreq();

  u64 timeToWait = osFreq * msToWait / 1000;
  u64 timeElapsed = 0;
  u64 tscStart = tscRead();
  u64 timerStart = osReadTimer();
  while(timeElapsed < timeToWait)
  {
    u64 timerEnd = osReadTimer();
    timeElapsed = timerEnd - timerStart;
  }
  u64 tscEnd = tscRead();
  u64 tscElapsed = tscEnd - tscStart;

  u64 result = 0;
  if(timeElapsed)
  {
    result = osFreq * tscElapsed / timeElapsed;
  }
  return(result);
}

#  if OS_WINDOWS
#define NOMINMAX
#include <windows.h>
static u64
osReadTimer(void)
{
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return(result.QuadPart);
}

static u64
osGetTimerFreq(void)
{
  LARGE_INTEGER result;
  QueryPerformanceFrequency(&result);
  return(result.QuadPart);
}

#  elif OS_MAC || OS_LINUX
#include <sys/time.h>
static u64
osReadTimer(void)
{
  timeval tv;
  gettimeofday(&tv, NULL);

  u64 result = osGetTimerFreq()*(u64)tv.tv_sec + (u64)tv.tv_usec;
  return(result);
}

static u64
osGetTimerFreq(void)
{
  return(1000000);
}
#  endif

#elif CPU_ARM || CPU_ARM64
static u64
tscRead(void)
{
  u64 result = 0;
#  if COMPILER_CLANG || COMPILER_GCC
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(result));
#  elif COMPILER_MSVC
  result = _ReadStatusReg(ARM64_CNTVCT_EL0);
#  endif
  return(result);
}

static u64
tscGetFreq(void)
{
  u64 result = 0;
#  if COMPILER_CLANG || COMPILER_GCC
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(result));
#  elif COMPILER_MSVC
  result = _ReadStatusReg(ARM64_CNTFRQ_EL0);
#  endif
  return(result);
}
#else
#  error ERROR: profiling not implemented for this cpu
#endif

Profiler::
Profiler(void) : currentParent(PROFILE_SITE_NIL)
{}

ProfiledScope::
ProfiledScope(ProfileSite *_site) : site(_site)
{
  // TODO(ry): we shouldn't have any branches in the profiler
  if(site->usedThreadProfiler == nullptr)
  {
    site->usedThreadProfiler = &threadProfiler;
  }
  else
  {
    // NOTE(ry); we don't support profiled sites being called by mutiple threads
    jassert(site->usedThreadProfiler == &threadProfiler);
  }

  tscElapsedRootOld = site->tscElapsedRoot;
  parent = threadProfiler.currentParent;
  threadProfiler.currentParent = site;
  ++site->hitCount;
  tscStart = tscRead();
}

ProfiledScope::
~ProfiledScope()
{
  u64 tscEnd = tscRead();
  u64 tscElapsed = tscEnd - tscStart;
  site->tscElapsed += tscElapsed;
  site->tscElapsedRoot = tscElapsedRootOld + tscElapsed;
  parent->tscElapsedChildren += tscElapsed;
  threadProfiler.currentParent = parent;

  if(threadProfiler.currentParent == PROFILE_SITE_NIL)
  {
    // NOTE(ry): top scope for thread exited, dump data for all sites this thread hit

    std::array sectionNames{
      std::string("Site Label"),
      std::string("Elapsed Exculsive (us)"),
      std::string("Elapsed With Children (us)"),
      std::string("Hit Count"),
    };
    switch(profileConfig.fmt)
    {
      case ProfileFormat::tui:
      {
        // NOTE(ry): headings
        profileLogFormatString("%-*s | %*s | %*s | %*s \n",
                               Profiler::maxSiteLabelLength, sectionNames[0].c_str(),
                               sectionNames[1].size(), sectionNames[1].c_str(),
                               sectionNames[2].size(), sectionNames[2].c_str(),
                               sectionNames[3].size(), sectionNames[3].c_str());

        // NOTE(ry): root site output
        r64 tscElapsed_us = 1000000.0 * (r64)tscElapsed * Profiler::tscPeriod;
        r64 tscElapsedExclusive_us = 1000000.0 * (r64)(site->tscElapsedRoot - site->tscElapsedChildren) * Profiler::tscPeriod;
        if(!juce::exactlyEqual(tscElapsed_us, tscElapsedExclusive_us))
        {
          profileLogFormatString("%-*.*s | %*.2f | %*.2f | %*lu \n",
                                 Profiler::maxSiteLabelLength, site->label.size(), site->label.data(),
                                 sectionNames[1].size(), tscElapsedExclusive_us,
                                 sectionNames[2].size(), tscElapsed_us,
                                 sectionNames[3].size(), site->hitCount);
        }
        else
        {
          profileLogFormatString("%-*.*s | %*.2f | %*s | %*lu \n",
                                 Profiler::maxSiteLabelLength, site->label.size(), site->label.data(),
                                 sectionNames[1].size(), tscElapsedExclusive_us,
                                 sectionNames[2].size(), "-",
                                 sectionNames[3].size(), site->hitCount);
        }

        // NOTE(ry): root site children output
        for(u32 siteIdx = 0; siteIdx < Profiler::siteCount; ++siteIdx)
        {
          auto *profSite = Profiler::sites + siteIdx;
          if(profSite->usedThreadProfiler != &threadProfiler ||
             profSite == this->site)
          { continue; }

          r64 siteTscElapsed_us = 1000000.0 * (r64)profSite->tscElapsed * Profiler::tscPeriod;
          r64 siteTscElapsedExclusive_us = 1000000.0 * (r64)(profSite->tscElapsedRoot - profSite->tscElapsedChildren) * Profiler::tscPeriod;
          if(!juce::approximatelyEqual(siteTscElapsed_us, siteTscElapsedExclusive_us))
          {
            profileLogFormatString("%-*.*s | %*.2f | %*.2f | %*lu \n",
                                   Profiler::maxSiteLabelLength, profSite->label.size(), profSite->label.data(),
                                   sectionNames[1].size(), siteTscElapsedExclusive_us,
                                   sectionNames[2].size(), siteTscElapsed_us,
                                   sectionNames[3].size(), profSite->hitCount);
          }
          else
          {
            profileLogFormatString("%-*.*s | %*.2f | %*s | %*lu \n",
                                   Profiler::maxSiteLabelLength, profSite->label.size(), profSite->label.data(),
                                   sectionNames[1].size(), siteTscElapsedExclusive_us,
                                   sectionNames[2].size(), "-",
                                   sectionNames[3].size(), profSite->hitCount);
          }

          // NOTE(ry): compute persistent statistics for root site children, clear transient data
          profSite->tscElapsedMax = std::max(profSite->tscElapsedMax, profSite->tscElapsed);
          profSite->tscElapsed = 0;
          profSite->tscElapsedChildren = 0;
          profSite->tscElapsedRoot = 0;
          profSite->hitCount = 0;
          profSite->bytesAllocated = 0;
          profSite->usedThreadProfiler = nullptr;
        }

        profileLogFormatString("\n");
      }break;

      case ProfileFormat::csv:
      {
        // NOTE(ry): header
        profileLogFormatString("%s,%s,%s,%s\r\n",
                               sectionNames[0].c_str(),
                               sectionNames[1].c_str(),
                               sectionNames[2].c_str(),
                               sectionNames[3].c_str());

        // NOTE(ry): root site output
        r64 tscElapsed_us = 1000000.0 * (r64)site->tscElapsed * Profiler::tscPeriod;
        r64 tscElapsedExclusive_us = 1000000.0 * (r64)(site->tscElapsedRoot - site->tscElapsedChildren) * Profiler::tscPeriod;
        if(!juce::approximatelyEqual(tscElapsed_us, tscElapsedExclusive_us))
        {
          profileLogFormatString("\"%.*s\",%f,%f,%lu\r\n",
                                 site->label.size(), site->label.data(),
                                 tscElapsedExclusive_us,
                                 tscElapsed_us,
                                 site->hitCount);
        }
        else
        {
          profileLogFormatString("\"%.*s\",%f,%s,%lu\r\n",
                                 site->label.size(), site->label.data(),
                                 tscElapsedExclusive_us,
                                 "-",
                                 site->hitCount);
        }

        // NOTE(ry): root site children output
        for(u32 siteIdx = 0; siteIdx < Profiler::siteCount; ++siteIdx)
        {
          auto *profSite = Profiler::sites + siteIdx;
          if(profSite->usedThreadProfiler != &threadProfiler ||
             profSite == this->site)
          { continue; }

          r64 siteTscElapsed_us = 1000000.0 * (r64)profSite->tscElapsed * Profiler::tscPeriod;
          r64 siteTscElapsedExclusive_us = 1000000.0 * (r64)(profSite->tscElapsedRoot - profSite->tscElapsedChildren) * Profiler::tscPeriod;
          if(!juce::approximatelyEqual(siteTscElapsed_us, siteTscElapsedExclusive_us))
          {
            profileLogFormatString("\"%.*s\",%f,%f,%lu\r\n",
                                   profSite->label.size(), profSite->label.data(),
                                   siteTscElapsedExclusive_us,
                                   siteTscElapsed_us,
                                   profSite->hitCount);
          }
          else
          {
            profileLogFormatString("\"%.*s\",%f,%s,%lu\r\n",
                                   profSite->label.size(), profSite->label.data(),
                                   siteTscElapsedExclusive_us,
                                   "-",
                                   profSite->hitCount);
          }

          // NOTE(ry): compute persistent statistics for root site children, clear transient data
          profSite->tscElapsedMax = std::max(profSite->tscElapsedMax, profSite->tscElapsed);
          profSite->tscElapsed = 0;
          profSite->tscElapsedChildren = 0;
          profSite->tscElapsedRoot = 0;
          profSite->hitCount = 0;
          profSite->bytesAllocated = 0;
          profSite->usedThreadProfiler = nullptr;
        }
      }break;
    }

    // NOTE(ry): dump log
    profileConfig.dst.onTopScopeExit(profileConfig.dst.userData);

    // NOTE(ry): compute persistent statistics for root site, clear transient data
    site->tscElapsedMax = std::max(site->tscElapsedMax, site->tscElapsed);
    site->tscElapsed = 0;
    site->tscElapsedChildren = 0;
    site->tscElapsedRoot = 0;
    site->hitCount = 0;
    site->bytesAllocated = 0;
    site->usedThreadProfiler = nullptr;
  }
}
