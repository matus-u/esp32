#ifndef WIFIPROVISIONER_H
#define WIFIPROVISIONER_H

#include <IPAddress.h>
#include <functional>

class WebServer;
class DNSServer;

struct IpSettings {
    bool dhcp_enabled;
    IPAddress ip;
    IPAddress gw;
    IPAddress netmask;
};

class WiFiProvisioner {
public:

  using ProvisionCallback = std::function<void()>;
  using InputCheckCallback = std::function<bool(const char *)>;
  using SuccessCallback =
      std::function<void(const char *, const char *, IpSettings*)>;
  using FactoryResetCallback = std::function<void()>;

  explicit WiFiProvisioner();
  ~WiFiProvisioner();

  bool startProvisioning();

  WiFiProvisioner &onProvision(ProvisionCallback callback);
  WiFiProvisioner &onSuccess(SuccessCallback callback);

private:
  void loop();
  bool connect(const char *ssid, const char *password);
  void releaseResources();
  void handleRootRequest();
  void handleResetRequest();
  void handleUpdateRequest();
  void handleConfigureRequest();
  void sendBadRequestResponse();
  void handleSuccesfulConnection();
  void handleUnsuccessfulConnection(const char *reason);

  ProvisionCallback provisionCallback;
  SuccessCallback onSuccessCallback;

  WebServer *_server;
  DNSServer *_dnsServer;
  IPAddress _apIP;
  IPAddress _netMsk;
  uint16_t _dnsPort;
  unsigned int _serverPort;
  unsigned int _wifiDelay;
  unsigned int _wifiConnectionTimeout;
  bool _serverLoopFlag;
};

#endif // WIFIPROVISIONER_H
