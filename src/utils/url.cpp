#include <kspp/utils/url.h>
#include <regex>

namespace kspp {

/*
  file://localhost/etc/fstab
  file:///etc/fstab
*/

  url::url(std::string s, std::string default_scheme)
      : good_(true) {
    std::string::size_type pos0 = s.find("://");
    if (pos0 == std::string::npos) {
      if (default_scheme.size() == 0) {
        good_ = false;
        return;
      }
      scheme_ = default_scheme;
      std::string::size_type pos1 = s.find('/', 0);
      if (pos1 != std::string::npos) {
        authority_ = s.substr(0, pos1);
        path_ = s.substr(pos1, std::string::npos);
      } else {
        authority_ = s.substr(0, std::string::npos);
      }
    } else {
      scheme_ = s.substr(0, pos0);

      std::string::size_type pos1 = s.find('/', pos0 + 3);
      if (pos1 != std::string::npos) {
        authority_ = s.substr(pos0 + 3, pos1 - (pos0 + 3));
        path_ = s.substr(pos1, std::string::npos);
      } else {
        authority_ = s.substr(pos0 + 3, std::string::npos);
      }
    }
    std::transform(scheme_.begin(), scheme_.end(), scheme_.begin(), ::tolower);
  }
}
