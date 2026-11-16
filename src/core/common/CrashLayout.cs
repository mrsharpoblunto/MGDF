// Both C++ and C# compile these mapping offsets.
#if __cplusplus
namespace MGDF::core {
enum class CrashLayout {
#else
namespace MGDF.CrashReporter {
public enum CrashLayout {
#endif
  ThreadId = 0,
  ErrorLength = 4,
  ExceptionPointers = 8,
  ErrorText = 16,
  ErrorLimit = 2048,
  Size = ErrorText + (ErrorLimit + 1) * 2
};
}
