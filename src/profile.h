enum class ProfileFormat : u32
{
  tui, /** output in format suitable for terminal displays */
  csv, /** output in csv format, suitable for an external tool */
};

using ProfileDestinationProc = void (*)(void *);
struct ProfileDestination
{
  /** data that will be passed to the `onTopScopeExit` callback */
  void *userData;

  /** function to call when a thread has finished executing its first profiled scope and all profiling data is ready to be dumped */
  ProfileDestinationProc onTopScopeExit;
};

struct ProfileConfig
{
  /** what format the output should be */
  ProfileFormat fmt;

  /** what to do when the profiler writes its data */
  ProfileDestination dst;
};

/** pass an instance of `ProfileConfig` to control the profiler output format
    and destination.

    Example Usage:
    ```
    ProfileConfig cfg = {};
    cfg.fmt = ProfileFormat::tui;
    cfg.dst.userData = &std::cerr;
    cfg.dst.onTopScopeExit = [](void *data){
        auto outStream = reinterpret_cast<std::ostream*>(data);
        *outStream << std::string_view(profileFlushLog());
    };
    ```
*/
extern void profileConfigure(ProfileConfig cfg);

/** returns a pointer to the start of the profile log for the current thread and resets the log */
extern char* profileFlushLog(void);

struct ProfileSite;
struct ProfiledScope;
struct Profiler
{
  Profiler(void);

  static ProfileSite *sites;
  static u64 siteCount;
  static u64 tscFreq;
  static size_t maxSiteLabelLength;
  static r64 tscPeriod;

  // TODO(ry): indices instead of pointers
  ProfileSite *currentParent;
  //ProfileSite *nextParent[MAX_SITES];

  u64 tscStart;
  u64 tscEnd;
};

/** a static location in code that may be hit mutliple times in different states
 * on different threads
 */
struct ProfileSite
{
  explicit ProfileSite(std::string_view _label) : label(_label)
  {
    Profiler::maxSiteLabelLength = std::max(Profiler::maxSiteLabelLength, label.size());
  }

  std::string_view label;

  /** the cumulative elapsed time stamp count accross all calls */
  u64 tscElapsed;
  /** the cumulative elapsed time stamp count of all children across all calls */
  u64 tscElapsedChildren;
  /** the elapsed time stamp count with this site as root */
  u64 tscElapsedRoot;
  /** the maximum cumulative elapsed time stamp count */
  u64 tscElapsedMax;

  u64 hitCount;

  u64 bytesAllocated;

  Profiler *usedThreadProfiler;
};

/** runtime state of profiled scopes
 */
struct ProfiledScope
{
  explicit ProfiledScope(ProfileSite *_site);
  ~ProfiledScope();

  u64 tscStart;
  u64 tscElapsedRootOld;
  ProfileSite *site;
  ProfileSite *parent;
};

static constexpr std::string_view
profileChopFullFunctionName(const char *fullNameCstr)
{
  auto view = std::string_view(fullNameCstr);
  view = view.substr(0, view.find_first_of('('));
  view = view.substr(view.find_last_of(' ') + 1);
  return view;
}

// NOTE(ry): public interface

#if defined(DISABLE_PROFILING)
#  define PROFILE_SCOPE(name)
#  define PROFILE_FUNCTION()
#else
#  define GLUE_(a, b) a##b
#  define GLUE(a, b) GLUE_(a, b)
#  define PROFILE_SCOPE(name)\
  static PROFILE_SECTION ProfileSite GLUE(profile__site__, __LINE__)(name); \
  ProfiledScope GLUE(profile__scope__, __LINE__)(&GLUE(profile__site__, __LINE__));
#  define PROFILE_FUNCTION() PROFILE_SCOPE(profileChopFullFunctionName(FULL_FUNCTION_NAME))
#endif

// NOTE(ry): internal

#if defined(DISABLE_PROFILING)
#  define PROFILE_SITE_NIL nullptr
static inline ProfileSite* profileSiteArrayBase(void) { return(nullptr); }
static inline u64 profileSiteArrayCount(void) { return(0); }
#else

#if COMPILER_MSVC
#  pragma section("dfvPROFL$a", read, write)
#  pragma section("dfvPROFL$i", read, write)
#  pragma section("dfvPROFL$z", read, write)
#  define SECTION(name) __declspec(allocate(name))
#  define PROFILE_SECTION SECTION("dfvPROFL$i")
#  define FULL_FUNCTION_NAME __FUNCSIG__
#  define PROFILE_SITE_NIL &__start_dfvPROFL;
// TODO(ry): how to avoid padding between the merged sections? don't make me parse the PE image
static SECTION("dfvPROFL$a") ProfileSite __start_dfvPROFL("start");
static SECTION("dfvPROFL$z") ProfileSite __stop_dfvPROFL("stop");
// #  pragma comment(linker, "/include:__start_dfvPROFL")
// #  pragma comment(linker, "/include:__stop_dfvPROFL")
static inline ProfileSite* profileSiteArrayBase(void) { return(&__start_dfvPROFL + 1); }
static inline u64 profileSiteArrayCount(void) { return(u64(&__stop_dfvPROFL - &__start_dfvPROFL - 1)); }

#elif COMPILER_GCC || COMPILER_CLANG

#  if OS_LINUX
#    define SECTION(name) __attribute__((section(name), used, aligned(1)))
#  elif OS_MAC
#    define SECTION(name) __attribute__((section("__DATA," name), used, aligned(1)))
#  else
#    error SECTION() not implemented for this OS
#  endif

#  define PROFILE_SECTION SECTION("dfvPROFL")
#  define FULL_FUNCTION_NAME __PRETTY_FUNCTION__
#  define PROFILE_SITE_NIL &profile__nil

static PROFILE_SECTION ProfileSite profile__nil("nil");

#  if OS_LINUX
extern ProfileSite __start_dfvPROFL[];
extern ProfileSite __stop_dfvPROFL[];
#  elif OS_MAC
extern ProfileSite __start_dfvPROFL[] asm("section$start$__DATA$dfvPROFL");
extern ProfileSite __stop_dfvPROFL[]  asm("section$end$__DATA$dfvPROFL");
#  else
#    error secition start/stop not implemented for this OS
#  endif

static inline ProfileSite* profileSiteArrayBase(void) { return(__start_dfvPROFL + 1); }
static inline u64 profileSiteArrayCount(void) { return(u64(__stop_dfvPROFL - __start_dfvPROFL) - 1); }

#else
#  error ERROR: profiling not implimented for this compiler
#endif

#endif // DISABLE_PROFILING
