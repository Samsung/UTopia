#ifndef FTG_TESTUTIL_SOURCEFILEMANAGER_H
#define FTG_TESTUTIL_SOURCEFILEMANAGER_H

#include <map>
#include <string>

namespace ftg {

class SourceFileManager {
public:
  SourceFileManager();
  ~SourceFileManager();
  bool addFile(std::string Path);
  bool createFile(std::string Name, std::string Content);
  std::string getBaseDirPath() const;
  std::string getFilePath(std::string Name) const;

private:
  std::string BaseDir;
  std::map<std::string, std::string> ManagedFiles;
};

} // namespace ftg

#endif // FTG_TESTUTIL_SOURCEFILEMANAGER_H
