#include "ftg/apiloader/APIExportsNDepsJsonLoader.h"
#include "ftg/apiloader/APIJsonLoader.h"
#include <gtest/gtest.h>

using namespace ftg;

TEST(APIJsonLoader, PublicAPIJsonLoadP) {
  std::string PublicAPIJsonPath =
      "tests/resources/apiloader_input/public_api.json";
  APIJsonLoader *Loader = new APIJsonLoader(PublicAPIJsonPath);

  ASSERT_EQ(Loader->load().size(), 3);
}

TEST(APIJsonLoaderDeathTest, NotExistPublicAPIJsonLoadN) {
  std::string PublicAPIJsonPath = "not_exists_public_api.json";
  APIJsonLoader *Loader = new APIJsonLoader(PublicAPIJsonPath);

  ASSERT_DEATH(Loader->load(), "Invalid Json");
}

TEST(APIExportsNDepsJsonLoader, ExportsNDepsJsonLoadP) {
  std::string ExportsNDepsJsonPath =
      "tests/resources/apiloader_input/exports_and_dependencies.json";
  std::string LibName = "library_test";
  APIExportsNDepsJsonLoader *Loader =
      new APIExportsNDepsJsonLoader(ExportsNDepsJsonPath, LibName);

  ASSERT_EQ(Loader->load().size(), 4);
}

TEST(APIExportsNDepsJsonLoaderDeathTest, NotExistExportsNDepsJsonLoadN) {
  std::string ExportsNDepsJsonPath = "not_exists_exports_and_dependencies.json";
  std::string LibName = "library_test";
  APIExportsNDepsJsonLoader *Loader =
      new APIExportsNDepsJsonLoader(ExportsNDepsJsonPath, LibName);

  ASSERT_DEATH(Loader->load(), "Invalid Json");
}
