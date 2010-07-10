#ifndef LSST_QSERV_WORKER_CONFIG_H
#define LSST_QSERV_WORKER_CONFIG_H
#include <map>
#include <string>

namespace lsst {
namespace qserv {
namespace worker {

// The Config object provides a thin abstraction layer to shield code from
// the details of how the qserv worker is configured.  It currently
// reads configuration from environment variables, but could later use
// its own configuration file.
class Config {
public:
    Config();
    std::string const& getString(std::string const& key) const;
    bool getIsValid() const { return _isValid; }
    std::string const& getError() const { return _error; }
private:
    typedef std::map<std::string, std::string> StringMap;
    char const* _getEnvDefault(char const* varName, char const* defVal);
    void _load();
    void _validate();

    StringMap _map;
    bool _isValid;
    std::string _error;
};

Config& getConfig(); 

}}}  
#endif // LSST_QSERV_WORKER_CONFIG_H