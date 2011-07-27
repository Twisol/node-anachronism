#include <v8.h>
#include <node.h>
#include <node_events.h>

#include <anachronism/nvt.h>

class NVT;

namespace anachronism {
  
class Channel : public node::EventEmitter {
public:
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
  static bool HasInstance(v8::Handle<v8::Value> value);
  
  static void Init(v8::Handle<v8::Object> target);
  
  
  Channel();
  virtual ~Channel();
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  
  // Listen to a port on an NVT
  telnet_error Attach(NVT* nvt, short id);
  static v8::Handle<v8::Value> Attach(const v8::Arguments& args);
  
  // Toggle the channel's mode
  telnet_error ToggleLocal(telnet_channel_mode mode);
  telnet_error ToggleRemote(telnet_channel_mode mode);
  static v8::Handle<v8::Value> Toggle(const v8::Arguments& args);
  
private:
  telnet_channel* channel_;
  
  void OnToggle(telnet_channel_mode on, telnet_channel_provider who);
  static void OnToggle(telnet_channel* channel,
                       telnet_channel_mode on,
                       telnet_channel_provider who);
  
  void OnData(telnet_channel_event type,
              const telnet_byte* data,
              size_t length);
  static void OnData(telnet_channel* channel,
                     telnet_channel_event type,
                     const telnet_byte* data,
                     size_t length);
};

} // namespace anachronism
