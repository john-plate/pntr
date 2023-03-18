# This specifies the order for the single header file!
set(pntr_headers
  common.hpp
  CounterThreadSafe.hpp
  CounterThreadUnsafe.hpp
  detail/PntrTypeTraits.hpp
  detail/ControlDataStorage.hpp
  detail/ControlDataUsage.hpp
  detail/ControlDataWeak.hpp
  detail/ControlDataOffset.hpp
  detail/ControlDataSize.hpp
  detail/ControlDataAlign.hpp
  detail/ControlDataUser.hpp
  detail/AllocAdaptBase.hpp
  detail/AllocAdaptPointer.hpp
  detail/AllocAdaptTypeInfo.hpp
  detail/AllocAdaptTyped.hpp
  detail/ControlDeleter.hpp
  AllocatorMalloc.hpp
  AllocatorMemoryResource.hpp
  Deleter.hpp
  ControlData.hpp
  ControlAlloc.hpp
  ControlNew.hpp
  Intruder.hpp
  SharedPtr.hpp
  WeakPtr.hpp
  pntr.hpp)

search_unknown_files_recurse(CMakeLists.txt ${pntr_headers})

set(parent_dir ..)
cmake_path(ABSOLUTE_PATH parent_dir NORMALIZE)

add_library(pntr INTERFACE)
target_sources(pntr
  PUBLIC FILE_SET HEADERS
    BASE_DIRS ${parent_dir}
    FILES ${pntr_headers})
target_compile_features(pntr INTERFACE cxx_std_17)

install(TARGETS pntr
  EXPORT ${project_export}
  FILE_SET HEADERS
  INCLUDES DESTINATION include)
