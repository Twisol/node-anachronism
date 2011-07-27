#include <v8.h>
#include <node.h>
#include <node_events.h>

#include <anachronism/nvt.h>


class NVT : public node::EventEmitter {
public:
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
  static bool HasInstance(v8::Handle<v8::Value> value);
  
  static void Init(v8::Handle<v8::Object> target);
  
  
  NVT(v8::Persistent<v8::Function> eventCallback, v8::Persistent<v8::Function> teloptCallback);
  virtual ~NVT();
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  
  telnet_error InterruptParser();
  static v8::Handle<v8::Value> InterruptParser(const v8::Arguments& args);
  
  telnet_error Receive(telnet_byte* data, size_t length, size_t* bytes_used);
  static v8::Handle<v8::Value> Receive(const v8::Arguments& args);
   
  telnet_error SendText(telnet_byte* data, size_t length);
  static v8::Handle<v8::Value> SendText(const v8::Arguments& args);
  
  telnet_error SendCommand(telnet_byte command);
  static v8::Handle<v8::Value> SendCommand(const v8::Arguments& args);
  
  telnet_error SendSubnegotiation(telnet_byte telopt, telnet_byte* data, size_t length);
  static v8::Handle<v8::Value> SendSubnegotiation(const v8::Arguments& args);
  
  
  telnet_error EnableLocalTelopt(const telnet_byte telopt, unsigned char lazy);
  static v8::Handle<v8::Value> EnableLocalTelopt(const v8::Arguments& args);
  
  telnet_error EnableRemoteTelopt(const telnet_byte telopt, unsigned char lazy);
  static v8::Handle<v8::Value> EnableRemoteTelopt(const v8::Arguments& args);
  
  telnet_error DisableLocalTelopt(const telnet_byte telopt);
  static v8::Handle<v8::Value> DisableLocalTelopt(const v8::Arguments& args);
  
  telnet_error DisableRemoteTelopt(const telnet_byte telopt);
  static v8::Handle<v8::Value> DisableRemoteTelopt(const v8::Arguments& args);
  
  telnet_telopt_mode LocalTeloptStatus(const telnet_byte telopt);
  static v8::Handle<v8::Value> LocalTeloptStatus(const v8::Arguments& args);
  
  telnet_telopt_mode RemoteTeloptStatus(const telnet_byte telopt);
  static v8::Handle<v8::Value> RemoteTeloptStatus(const v8::Arguments& args);
  
  
  void OnTelnetEvent(telnet_event* event);
  static void OnTelnetEvent(telnet_nvt* nvt, telnet_event* event);
  
  void OnTeloptEvent(telnet_byte telopt, telnet_telopt_event* event);
  static void OnTeloptEvent(telnet_nvt* nvt, telnet_byte telopt, telnet_telopt_event* event);
  
private:
  telnet_nvt* nvt_;
  
  v8::Persistent<v8::Function> onEvent_;
  v8::Persistent<v8::Function> onTeloptEvent_;
};
