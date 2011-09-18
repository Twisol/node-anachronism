#include <v8.h>
#include <node.h>
#include <node_events.h>

#include <anachronism/nvt.h>


class NVT : public node::ObjectWrap {
public:
  static void Init(v8::Handle<v8::Object> target);
  virtual ~NVT();
  
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
  static bool HasInstance(v8::Handle<v8::Value> value);
  
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> InterruptParser(const v8::Arguments& args);
  static v8::Handle<v8::Value> Receive(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> SendText(const v8::Arguments& args);
  static v8::Handle<v8::Value> SendCommand(const v8::Arguments& args);
  static v8::Handle<v8::Value> SendSubnegotiation(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> EnableLocalTelopt(const v8::Arguments& args);
  static v8::Handle<v8::Value> EnableRemoteTelopt(const v8::Arguments& args);
  static v8::Handle<v8::Value> DisableLocalTelopt(const v8::Arguments& args);
  static v8::Handle<v8::Value> DisableRemoteTelopt(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> LocalTeloptStatus(const v8::Arguments& args);
  static v8::Handle<v8::Value> RemoteTeloptStatus(const v8::Arguments& args);
  
  static void OnTelnetEvent(telnet_nvt* nvt, telnet_event* event);
  static void OnTeloptEvent(telnet_nvt* nvt, telnet_byte telopt, telnet_telopt_event* event);
  static unsigned char OnNegotiateEvent(telnet_nvt* nvt, telnet_byte telopt, telnet_telopt_location where);
  
private:
  telnet_nvt* nvt_;
  
  v8::Persistent<v8::Function> onEvent_;
  v8::Persistent<v8::Function> onTeloptEvent_;
  v8::Persistent<v8::Function> onNegotiateEvent_;
};
