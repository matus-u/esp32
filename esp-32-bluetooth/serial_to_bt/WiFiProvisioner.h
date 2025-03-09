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

  using SuccessCallback =
      std::function<void(const char *, const char *, IpSettings*)>;

  using ApSuccessCallback =
      std::function<void(const char *, const char *)>;

  explicit WiFiProvisioner();
  ~WiFiProvisioner();

  bool startProvisioning();

  WiFiProvisioner &onSuccess(SuccessCallback callback);
  WiFiProvisioner &onApSuccess(ApSuccessCallback callback);

private:
  void loop();
  bool connect(const char *ssid, const char *password);
  void releaseResources();
  void handleRootRequest();
  void handleResetRequest();
  void handleUpdateRequest();
  void handleConfigureRequest();
  void handleApConfigureRequest();
  void sendBadRequestResponse();
  void handleSuccesfulConnection();
  void handleUnsuccessfulConnection(const char *reason);

  SuccessCallback onSuccessCallback;
  ApSuccessCallback onApSuccessCallback;

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
