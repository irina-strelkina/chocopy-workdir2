#include <cstdint>
#include <cstring>
#include <list>
#include <memory>
#include <string>

extern "C" struct object {
  int32_t Tag;
  int32_t Size;
  void *DispatchTable;
};

static std::list<std::unique_ptr<char[]>> Objects;

extern "C" void __dso_handle();

extern "C" void *alloc(object *O) {
  Objects.emplace_back(std::make_unique<char[]>(O->Size));

  void *Ptr = Objects.back().get();
  std::memcpy(Ptr, O, O->Size);
  return Ptr;
}
