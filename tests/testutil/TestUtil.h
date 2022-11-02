#ifndef FTG_TESTS_TESTUTIL_H
#define FTG_TESTS_TESTUTIL_H

#include <string>

namespace ftg {

std::string getTmpDirPath();
std::string getUniqueFilePath(std::string Dir, std::string Name,
                              std::string Ext = "");

} // namespace ftg

#endif // FTG_TESTS_TESTUTIL_H
